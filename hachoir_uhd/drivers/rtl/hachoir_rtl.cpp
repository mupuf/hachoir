#include <rtl-sdr.h>

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>

#include "utils/rxtimedomain.h"

namespace po = boost::program_options;

static volatile bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

bool rtl_set_phy(rtlsdr_dev_t *dev, const phy_parameters_t &phy)
{
	//set the center frequency
	std::cout << boost::format("Setting RX Freq: %u MHz...") % (phy.central_freq/1e6) << std::endl;
	rtlsdr_set_center_freq(dev, phy.central_freq);
	std::cout << boost::format("Actual RX Freq: %u MHz...") % (rtlsdr_get_center_freq(dev) / 1e6) << std::endl << std::endl;

	//set the sample rate
	if (phy.sample_rate <= 0.0){
		std::cerr << "Please specify a valid sample rate" << std::endl;
		return false;
	}
	std::cout << boost::format("Setting RX Rate: %f Msps...") % (phy.sample_rate/1e6) << std::endl;
	rtlsdr_set_sample_rate(dev, phy.sample_rate);
	std::cout << boost::format("Actual RX Rate: %f Msps...") % (rtlsdr_get_sample_rate(dev)/1e6) << std::endl << std::endl;

	//set the rf gain
	if (phy.gain >= 0.0) {
		std::cout << boost::format("Setting RX Gain Mode: Manual...") << std::endl;
		rtlsdr_set_tuner_gain_mode(dev, false);

		std::cout << boost::format("Setting RX Gain: %f dB...") % phy.gain << std::endl;
		rtlsdr_set_tuner_gain(dev, phy.gain * 10);
		// TODO: IF gain
		std::cout << boost::format("Actual RX Gain: %f dB...") % rtlsdr_get_tuner_gain(dev) << std::endl << std::endl;
	} else {
		std::cout << boost::format("Setting RX Gain Mode: Auto...") << std::endl;
		rtlsdr_set_tuner_gain_mode(dev, true);
		std::cout << boost::format("Actual RX Gains: tuner: %f dB...") % (rtlsdr_get_tuner_gain(dev) / 10.0)  << std::endl << std::endl;
	}

	// reset the samples buffer to get rid of all the intermediate samples
	rtlsdr_reset_buffer(dev);
	return true;
}

uint64_t time_us()
{
	static struct timeval time_start = {0, 0};
	struct timeval time;
	gettimeofday(&time, NULL);

	if (time_start.tv_sec == 0)
		time_start = time;

	return (time.tv_sec * 1000000 + time.tv_usec) - (time_start.tv_sec * 1000000 + time_start.tv_usec);
}

bool RX_msg_cb(const Message &msg, phy_parameters_t &phy, void *userData)
{
	return true;
}

bool samples_read(rtlsdr_dev_t *dev, phy_parameters_t &phy, const std::string &file)
{
	std::complex<short> samples[4096];
	std::ofstream outfile;
	uint8_t buf[8192];
	uint64_t sample_count = 0;
	int len;
	bool ret = false;

	RXTimeDomain rxTimeDomain(RX_msg_cb, NULL);
	rxTimeDomain.setPhyParameters(phy);

	if (file != std::string()) {
		char filename[100];
		snprintf(filename, sizeof(filename), "%s-%.0fkHz-%.0fkSPS.dat",
			file.c_str(), phy.central_freq / 1000, phy.sample_rate / 1000);

		outfile.open(filename, std::ofstream::binary);
		if (outfile.is_open())
			std::cout << "Recording samples to '" << filename << "'." << std::endl;
		else
			std::cerr << "Failed to open '" << filename << "'." << std::endl;
	}

	do {
		if (rtlsdr_read_sync(dev, buf, sizeof(buf), &len) || (len % 2) != 0) {
			std::cerr << "rtlsdr_read_sync returned an error, len = " << len << std::endl;
			ret = false;
			break;
		}

		for (int i = 0; i < len / 2; i++) {
			samples[i] = std::complex<short>(buf[i * 2] - 127, buf[(i * 2) + 1] - 127);
		}

		if (!rxTimeDomain.processSamples(time_us(), samples, len / 2)) {
			ret = true;
			break;
		}

		if (outfile.is_open()) {
			outfile.write((const char*)&samples, (len/2) * sizeof(std::complex<unsigned short>));
			sample_count += (len / 2);
		}
	} while (!stop_signal_called);

	if (outfile.is_open()) {
		outfile.close();
		std::cout << "Wrote " << sample_count << " samples to the disk" << std::endl;
	}

	return ret;
}

int main(int argc, char *argv[])
{
	phy_parameters_t phy;
	rtlsdr_dev_t *dev;
	std::string file;

	//setup the program options
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "help message")
		("rate", po::value<float>(&phy.sample_rate)->default_value(1e6), "rate of incoming samples")
		("freq", po::value<float>(&phy.central_freq)->default_value(0.0), "RF center frequency in Hz")
		("gain", po::value<float>(&phy.gain), "gain for the RF chain")
		("file", po::value<std::string>(&file), "output all the samples to this file")
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	//print the help message
	if (vm.count("help")){
		std::cout << boost::format("Hachoir RTL %s") % desc << std::endl;
		return ~0;
	}

	phy.IF_bw = -1.0;
	if (not vm.count("gain"))
		phy.gain = -1.0;


	// find one device
	if(rtlsdr_open(&dev, 0)) {
		std::cerr << "rtlsdr_open: couldn't open the rtl-sdr device" << std::endl;
		return -1;
	}

	std::signal(SIGINT, &sig_int_handler);
	std::signal(SIGTERM, &sig_int_handler);
	std::signal(SIGQUIT, &sig_int_handler);
	std::signal(SIGABRT, &sig_int_handler);
	std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

	bool phy_ok, start_over;
	do {
		phy_ok = rtl_set_phy(dev, phy);
		if (!phy_ok)
			continue;

		/* Process samples */
		start_over = samples_read(dev, phy, file);

		//finished
		std::cout << std::endl << "Done!" << std::endl << std::endl;
	} while (phy_ok && start_over);

	rtlsdr_close(dev);

	return EXIT_SUCCESS;
}
