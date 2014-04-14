#include <libbladeRF.h>

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>

#include "modulations/modulationOOK.h"
#include "utils/com_detect.h"
#include "utils/message.h"

namespace po = boost::program_options;

static volatile bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

bool brf_set_phy(struct bladerf *dev, const phy_parameters_t &phy)
{
	unsigned int actual;
	int ret;

	//set the center frequency
	std::cout << boost::format("Setting RX Freq: %u MHz...") % (phy.central_freq/1e6) << std::endl;
	ret = bladerf_set_frequency(dev, BLADERF_MODULE_TX, phy.central_freq);
	if (ret)
		std::cerr << "bladerf_set_frequency: " << bladerf_strerror(ret) << std::endl;
	bladerf_get_frequency(dev, BLADERF_MODULE_TX, &actual);
	std::cout << boost::format("Actual RX Freq: %u MHz...") % (actual / 1.0e6) << std::endl << std::endl;

	//set the sample rate
	if (phy.sample_rate <= 0.0){
		std::cerr << "Please specify a valid sample rate" << std::endl;
		return false;
	}
	std::cout << boost::format("Setting TX Rate: %f Msps...") % (phy.sample_rate/1e6) << std::endl;
	ret = bladerf_set_sample_rate(dev, BLADERF_MODULE_TX, phy.sample_rate, &actual);
	if (ret)
		std::cerr << "bladerf_set_sample_rate: " << bladerf_strerror(ret) << std::endl;
	std::cout << boost::format("Actual TX Rate: %f Msps...") % (actual/1.0e6) << std::endl << std::endl;

	// set the bandwidth
	std::cout << boost::format("Setting TX Bandwidth: %f MHz...") % (phy.sample_rate/1e6) << std::endl;
	ret = bladerf_set_bandwidth(dev, BLADERF_MODULE_TX, phy.sample_rate, &actual);
	if (ret)
		std::cerr << "bladerf_set_bandwidth: " << bladerf_strerror(ret) << std::endl;
	std::cout << boost::format("Actual TX Bandwidth: %f MHz...") % (actual/1.0e6) << std::endl << std::endl;

	//set the rf gain
	if (phy.gain >= 0.0) {
		/*std::cout << boost::format("Setting RX Gain Mode: Manual...") << std::endl;
		rtlsdr_set_tuner_gain_mode(dev, false);

		std::cout << boost::format("Setting RX Gain: %f dB...") % phy.gain << std::endl;
		rtlsdr_set_tuner_gain(dev, phy.gain * 10);
		// TODO: IF gain
		std::cout << boost::format("Actual RX Gain: %f dB...") % rtlsdr_get_tuner_gain(dev) << std::endl << std::endl;*/
	} else {
		int txvga1 = BLADERF_TXVGA1_GAIN_MIN + (BLADERF_TXVGA1_GAIN_MAX - BLADERF_TXVGA1_GAIN_MIN) / 2;
		int txvga2 = BLADERF_TXVGA2_GAIN_MIN + (BLADERF_TXVGA2_GAIN_MAX - BLADERF_TXVGA2_GAIN_MIN) / 2;
		bladerf_lna_gain lna_gain = BLADERF_LNA_GAIN_MID;

		std::cout << boost::format("Setting RX VGA1 Gain: %i dB ...") % (txvga1) << std::endl;
		ret = bladerf_set_txvga1(dev, txvga1);
		if (ret)
			std::cerr << "bladerf_set_txvga1: " << bladerf_strerror(ret) << std::endl;
		bladerf_get_txvga1(dev, &txvga1);
		std::cout << boost::format("Actual TX VGA1 Gain: %f dB...") % (txvga1) << std::endl << std::endl;

		std::cout << boost::format("Setting TX VGA2 Gain: %i dB ...") % (txvga2) << std::endl;
		ret = bladerf_set_txvga2(dev, txvga2);
		if (ret)
			std::cerr << "bladerf_set_txvga2: " << bladerf_strerror(ret) << std::endl;
		bladerf_get_txvga2(dev, &txvga2);
		std::cout << boost::format("Actual TX VGA2 Gain: %f dB...") % (txvga2) << std::endl << std::endl;

		std::cout << boost::format("Setting LNA Gain: %i dB ...") % (lna_gain) << std::endl;
		ret = bladerf_set_lna_gain(dev, BLADERF_LNA_GAIN_MID);
		if (ret)
			std::cerr << "bladerf_get_lna_gain: " << bladerf_strerror(ret) << std::endl;
		bladerf_get_lna_gain(dev, &lna_gain);
		std::cout << boost::format("Actual LNA Gain: %f dB...") % (lna_gain) << std::endl << std::endl;
	}

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

void sample_send(struct bladerf *dev, std::complex<short> *samples, size_t len)
{
	std::complex<short> padding[4096] = { std::complex<short>(0, 0) };
	size_t padding_len = 0;
	int ret;

	if (len % 4096)
		padding_len = len - ((len / 4096) * 4096);

	// enable the sync TX mode
	ret = bladerf_sync_config(dev, BLADERF_MODULE_TX, BLADERF_FORMAT_SC16_Q11,
				  64, 4096, 16, 0);
	if (ret)
		std::cerr << "bladerf_sync_config: " << bladerf_strerror(ret) << std::endl;

	// enable the TX module
	ret = bladerf_enable_module(dev, BLADERF_MODULE_TX, true);
	if (ret)
		std::cerr << "bladerf_enable_module: " << bladerf_strerror(ret) << std::endl;

	for (int i = 0; i < 10; i++) {

		ret = bladerf_sync_tx(dev, samples, len, NULL, 1000);
		if (ret)
			std::cerr << "bladerf_sync_tx: " << bladerf_strerror(ret) << std::endl;
		usleep(20000);
	}

	ret = bladerf_sync_tx(dev, padding, padding_len, NULL, 1000);
	if (ret)
		std::cerr << "bladerf_sync_tx: " << bladerf_strerror(ret) << std::endl;

	usleep(5000);
	bladerf_enable_module(dev, BLADERF_MODULE_TX, false);
}

int main(int argc, char *argv[])
{
	phy_parameters_t phy;
	struct bladerf *dev;
	std::string file;
	FILE *f = NULL;

	//setup the program options
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "help message")
		("rate", po::value<float>(&phy.sample_rate)->default_value(1e6), "rate of incoming samples")
		("freq", po::value<float>(&phy.central_freq)->default_value(0.0), "RF center frequency in Hz")
		("gain", po::value<float>(&phy.gain), "gain for the RF chain")
		("file", po::value<std::string>(&file), "input all the samples from this file")
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

	if (file.size() > 0) {
		f = fopen(file.c_str(), "rb");
		if (!f) {
			std::cerr << "Couldn't open file " << file << "! Bailing out..." << std::endl;
			return 1;
		}
		std::cout << "Replaying file " << file << "..." << std::endl;
	}


	// find one device
	if(bladerf_open(&dev, NULL)) {
		std::cerr << "bladerf_open: couldn't open the bladeRF device" << std::endl;
		return -1;
	}

	std::signal(SIGINT, &sig_int_handler);
	std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

	if (!brf_set_phy(dev, phy))
		return 1;

	if (f) {
		std::complex<short> samples[4096];
		int ret;

		// enable the sync TX mode
		ret = bladerf_sync_config(dev, BLADERF_MODULE_TX, BLADERF_FORMAT_SC16_Q11,
					  64, 1024, 16, 0);
		if (ret)
			std::cerr << "bladerf_sync_config: " << bladerf_strerror(ret) << std::endl;

		// enable the TX module
		ret = bladerf_enable_module(dev, BLADERF_MODULE_TX, true);
		if (ret)
			std::cerr << "bladerf_enable_module: " << bladerf_strerror(ret) << std::endl;

		size_t len;

		do {
			len = fread(samples, sizeof(std::complex<short>), 4096, f);

			ret = bladerf_sync_tx(dev, samples, len, NULL, 1000);
			if (ret)
				std::cerr << "bladerf_sync_tx: " << bladerf_strerror(ret) << std::endl;
		} while(len == 4096);

		// disable the TX module
		ret = bladerf_enable_module(dev, BLADERF_MODULE_TX, false);
		if (ret)
			std::cerr << "bladerf_enable_module: " << bladerf_strerror(ret) << std::endl;

	} else {
		Message m({0x55, 0x2a, 0xb2});
		m.addBit(true);

		//for (float freq = 433.80e6; freq < 434.10e6; freq += 0.01e6) {
		//for (float freq = 868.60e6; freq < 868.90e6; freq += 0.01e6) {
		float freq = 433.90e6;
			ModulationOOK::SymbolOOK sOn(261.2, 527.2);
			ModulationOOK::SymbolOOK sOff(270.8, 536.3);
			ModulationOOK::SymbolOOK sStop(9300);
			m.setModulation(std::shared_ptr<ModulationOOK>(new ModulationOOK(freq, sOn, sOff, sStop)));

			std::cout << freq / 1e6 << " Mhz: " << m.toString(Message::HEX) << std::endl;

			std::complex<short> *samples;
			size_t len_samples;
			m.modulation()->genSamples(&samples, &len_samples, m, phy.central_freq,
						   phy.sample_rate, 2000.0);

			sample_send(dev, samples, len_samples);

			delete[] samples;
		//}
	}

	bladerf_close(dev);

	return EXIT_SUCCESS;
}
