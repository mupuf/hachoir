#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>

#include "utils/com_detect.h"

namespace po = boost::program_options;

static bool stop_signal_called = false;
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

void samples_read(std::string file, phy_parameters_t &phy)
{
	std::complex<unsigned short> samples[4096];
	bool stop = false;
	uint64_t sample_count = 0;

	std::ifstream infile(file.c_str(), std::ifstream::binary);
	if (!infile.is_open()) {
		std::cerr << "Cannot open file '" << file << "'." << std::endl;
		return;
	}

	do {
		infile.read((char*)&samples, 4096 * sizeof(std::complex<unsigned short>));
		size_t num_tx_samps = infile.gcount()/sizeof(std::complex<unsigned short>);

		if (num_tx_samps == 0) {
			std::cout << "Replayed " << sample_count << " samples. Exiting..." << std::endl;
			return;
		}

		process_samples(phy, sample_count * 1000000 / phy.sample_rate,
				    "sc16", samples, num_tx_samps);
		sample_count += num_tx_samps;
	} while (!stop && !stop_signal_called);
}

int main(int argc, char *argv[])
{
	phy_parameters_t phy;
	std::string file;

	//setup the program options
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "help message")
		("rate", po::value<float>(&phy.sample_rate)->default_value(1e6), "rate of incoming samples")
		("freq", po::value<float>(&phy.central_freq)->default_value(0.0), "RF center frequency in Hz")
		("gain", po::value<float>(&phy.gain), "gain for the RF chain")
		("file", po::value<std::string>(&file), "file to replay, sc16 format")
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	//print the help message
	if (vm.count("help")){
		std::cout << boost::format("UHD RX samples to file %s") % desc << std::endl;
		return ~0;
	}

	if (file == std::string()) {
		std::cout << "please specify a file with '--file <file>'" << std::endl;
		return ~0;
	}

	phy.IF_bw = -1.0;
	if (not vm.count("gain"))
		phy.gain = -1.0;

	std::signal(SIGINT, &sig_int_handler);
	std::signal(SIGTERM, &sig_int_handler);
	std::signal(SIGQUIT, &sig_int_handler);
	std::signal(SIGABRT, &sig_int_handler);
	std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

	/* Process samples */
	samples_read(file, phy);

	return EXIT_SUCCESS;
}
