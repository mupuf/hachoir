#include <libbladeRF.h>

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>

#include "utils/com_detect.h"

namespace po = boost::program_options;

static volatile bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

bool brf_set_phy(struct bladerf *dev, const phy_parameters_t &phy)
{
	unsigned int actual;
	int ret;

	//set the center frequency
	std::cout << boost::format("Setting RX Freq: %u MHz...") % (phy.central_freq/1e6) << std::endl;
	ret = bladerf_set_frequency(dev, BLADERF_MODULE_RX, phy.central_freq);
	if (ret)
		std::cerr << "bladerf_set_frequency: " << bladerf_strerror(ret) << std::endl;
	bladerf_get_frequency(dev, BLADERF_MODULE_RX, &actual);
	std::cout << boost::format("Actual RX Freq: %u MHz...") % (actual / 1.0e6) << std::endl << std::endl;

	//set the sample rate
	if (phy.sample_rate <= 0.0){
		std::cerr << "Please specify a valid sample rate" << std::endl;
		return false;
	}
	std::cout << boost::format("Setting RX Rate: %f Msps...") % (phy.sample_rate/1e6) << std::endl;
	ret = bladerf_set_sample_rate(dev, BLADERF_MODULE_RX, phy.sample_rate, &actual);
	if (ret)
		std::cerr << "bladerf_set_sample_rate: " << bladerf_strerror(ret) << std::endl;
	std::cout << boost::format("Actual RX Rate: %f Msps...") % (actual/1.0e6) << std::endl << std::endl;

	// set the bandwidth
	std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (phy.sample_rate/1e6) << std::endl;
	ret = bladerf_set_bandwidth(dev, BLADERF_MODULE_RX, phy.sample_rate, &actual);
	if (ret)
		std::cerr << "bladerf_set_bandwidth: " << bladerf_strerror(ret) << std::endl;
	std::cout << boost::format("Actual RX Bandwidth: %f MHz...") % (actual/1.0e6) << std::endl << std::endl;

	//set the rf gain
	if (phy.gain >= 0.0) {
		/*std::cout << boost::format("Setting RX Gain Mode: Manual...") << std::endl;
		rtlsdr_set_tuner_gain_mode(dev, false);

		std::cout << boost::format("Setting RX Gain: %f dB...") % phy.gain << std::endl;
		rtlsdr_set_tuner_gain(dev, phy.gain * 10);
		// TODO: IF gain
		std::cout << boost::format("Actual RX Gain: %f dB...") % rtlsdr_get_tuner_gain(dev) << std::endl << std::endl;*/
	} else {
		int rxvga1 = BLADERF_RXVGA1_GAIN_MIN + (BLADERF_RXVGA1_GAIN_MAX - BLADERF_RXVGA1_GAIN_MIN) / 2;
		int rxvga2 = BLADERF_RXVGA2_GAIN_MIN + (BLADERF_RXVGA2_GAIN_MAX - BLADERF_RXVGA2_GAIN_MIN) / 2;

		std::cout << boost::format("Setting RX VGA1 Gain: %i dB ...") % (rxvga1) << std::endl;
		ret = bladerf_set_rxvga1(dev, rxvga1);
		if (ret)
			std::cerr << "bladerf_set_rxvga1: " << bladerf_strerror(ret) << std::endl;
		bladerf_get_rxvga1(dev, &rxvga1);
		std::cout << boost::format("Actual RX VGA1 Gain: %f dB...") % (rxvga1) << std::endl << std::endl;

		std::cout << boost::format("Setting RX VGA2 Gain: %i dB ...") % (rxvga2) << std::endl;
		ret = bladerf_set_rxvga2(dev, rxvga2);
		if (ret)
			std::cerr << "bladerf_set_rxvga2: " << bladerf_strerror(ret) << std::endl;
		bladerf_get_rxvga2(dev, &rxvga2);
		std::cout << boost::format("Actual RX VGA2 Gain: %f dB...") % (rxvga2) << std::endl << std::endl;
	}

	// enable the sync RX mode
	ret = bladerf_sync_config(dev, BLADERF_MODULE_RX, BLADERF_FORMAT_SC16_Q11,
				  64, 16384, 16, 0);
	if (ret)
		std::cerr << "bladerf_set_frequency: " << bladerf_strerror(ret) << std::endl;

	// enable the RX module
	ret = bladerf_enable_module(dev, BLADERF_MODULE_RX, true);
	if (ret)
		std::cerr << "bladerf_set_frequency: " << bladerf_strerror(ret) << std::endl;

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

bool samples_read(struct bladerf *dev, phy_parameters_t &phy, const std::string &file)
{
	std::complex<unsigned short> samples[4096];
	struct bladerf_metadata meta;
	std::ofstream outfile;
	uint64_t sample_count = 0;
	int ret, len = 4096;
	bool exit = false;

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
		ret = bladerf_sync_rx(dev, samples, len, &meta, 1000);
		if (ret) {
			std::cerr << "bladerf_sync_rx: " << bladerf_strerror(ret) << std::endl;
			exit = false;
			break;
		}

		if (process_samples(phy, time_us(), "sc16", samples, len)) {
			exit = true;
			break;
		}

		if (outfile.is_open()) {
			outfile.write((const char*)&samples, len * sizeof(std::complex<unsigned short>));
			sample_count += len;
		}
	} while (!stop_signal_called || exit);

	if (outfile.is_open()) {
		outfile.close();
		std::cout << "Wrote " << sample_count << " samples to the disk" << std::endl;
	}

	return exit;
}

int main(int argc, char *argv[])
{
	phy_parameters_t phy;
	struct bladerf *dev;
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
	if(bladerf_open(&dev, NULL)) {
		std::cerr << "bladerf_open: couldn't open the bladeRF device" << std::endl;
		return -1;
	}

	std::signal(SIGINT, &sig_int_handler);
	std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

	bool phy_ok, start_over;
	do {
		phy_ok = brf_set_phy(dev, phy);
		if (!phy_ok)
			continue;

		/* Process samples */
		start_over = samples_read(dev, phy, file);

		//finished
		std::cout << std::endl << "Done!" << std::endl << std::endl;
	} while (phy_ok && start_over);

	bladerf_close(dev);

	return EXIT_SUCCESS;
}
