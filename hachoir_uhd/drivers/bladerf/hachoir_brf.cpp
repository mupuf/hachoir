#include <libbladeRF.h>

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>
#include <thread>
#include <mutex>
#include <time.h>

#include "utils/com_detect.h"

#include "common.h"

namespace po = boost::program_options;

static volatile bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

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
	std::complex<short> samples[4096];
	struct bladerf_metadata meta;
	std::ofstream outfile;
	uint64_t sample_count = 0;
	int ret, len = 4096;
	bool exit = false;

	// enable the sync RX mode
	BLADERF_CALL(bladerf_sync_config(dev, BLADERF_MODULE_RX, BLADERF_FORMAT_SC16_Q11,
				  64, 16384, 16, 0));

	// enable the RX module
	BLADERF_CALL(bladerf_enable_module(dev, BLADERF_MODULE_RX, true));

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
	dev = brf_open_and_init(NULL);

	std::signal(SIGINT, &sig_int_handler);
	std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

	bool phy_ok, start_over;
	do {
		phy_ok = brf_set_phy(dev, BLADERF_MODULE_RX, phy);
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
