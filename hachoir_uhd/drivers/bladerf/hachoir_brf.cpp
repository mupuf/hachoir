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

#include "utils/rxtimedomain.h"
#include "utils/tapinterface.h"
#include "utils/emissionruntime.h"
#include "modulations/modulationOOK.h"
#include "modulations/modulationFSK.h"
#include "modulations/modulationPSK.h"

#include "common.h"

namespace po = boost::program_options;
float txFreq;

uint64_t last_message;

static volatile bool stop_signal_called = false;
void sig_int_handler(int){
	stop_signal_called = true;
	std::cout << "Exiting..." << std::endl;
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

static uint64_t time_abs()
{
	struct timeval time;
	gettimeofday(&time, NULL);

	return (time.tv_sec * 1000000 + time.tv_usec);
}

struct rx_data {
	FILE *outfile;
	uint64_t sample_count;

	RXTimeDomain *rxTimeDomain;
	TapInterface *tapInterface;
};

bool brf_RX_stream_cb(struct bladerf *dev, struct bladerf_stream *stream,
				struct bladerf_metadata *meta,
				std::complex<short> *samples, size_t len,
				phy_parameters_t &phy, void *user_data)
{
	struct rx_data *data = (struct rx_data *)user_data;

	if (data->outfile) {
		fwrite(samples, sizeof(std::complex<short>), len, data->outfile);
		data->sample_count += len;
	}

	if (stop_signal_called)
		return false;

	return data->rxTimeDomain->processSamples(time_abs(), samples, len);
}

bool RX_msg_cb(const Message &msg, phy_parameters_t &phy, void *userData)
{
	struct rx_data *data = (struct rx_data *)userData;

	std::cerr << "Full Trip time = " << (time_abs() - last_message) / 1000 << " ms" << std::endl;

	data->tapInterface->sendMessage(msg);

	return true;
}

void thread_rx(struct bladerf *dev, std::mutex *mutex_conf, phy_parameters_t phy,
	       TapInterface *tapInterface = NULL,
	       const std::string &file = std::string())
{
	struct rx_data data;

	if (file != std::string()) {
		char filename[100];
		snprintf(filename, sizeof(filename), "%s-%.0fkHz-%.0fkSPS.dat",
			file.c_str(), phy.central_freq / 1000, phy.sample_rate / 1000);

		data.outfile = fopen(filename, "wb");
		if (data.outfile)
			std::cout << "RX: Recording samples to '" << filename << "'." << std::endl;
		else
			std::cerr << "RX: Failed to open '" << filename << "'." << std::endl;
		data.sample_count = 0;
	}

	data.rxTimeDomain = new RXTimeDomain(RX_msg_cb, &data);
	data.rxTimeDomain->setPhyParameters(phy);

	data.tapInterface = tapInterface;

	bool phy_ok;
	do {
		if (brf_start_stream(dev, BLADERF_MODULE_RX, 0.001, 4096, phy,
				 brf_RX_stream_cb, &data) && !stop_signal_called) {
			mutex_conf->lock();
			phy_ok = brf_set_phy(dev, BLADERF_MODULE_RX, phy);
			data.rxTimeDomain->setPhyParameters(phy);
			mutex_conf->unlock();
		}
	} while (phy_ok && !stop_signal_called);

	delete data.rxTimeDomain;

	if (data.outfile) {
		fclose(data.outfile);
		std::cerr << "RX: Wrote " << data.sample_count << " samples to the disk" << std::endl;
	}
}

struct tx_data {
	FILE *outfile;
	uint64_t sample_count;

	EmissionRunTime *txRT;
};

bool brf_TX_stream_cb(struct bladerf *dev, struct bladerf_stream *stream,
			struct bladerf_metadata *meta,
			std::complex<short> *samples_next, size_t len,
			phy_parameters_t &phy, void *user_data)
{
	struct tx_data *data = (struct tx_data *)user_data;

	if (stop_signal_called)
		return false;

	EmissionRunTime::Command ret = data->txRT->next_block(samples_next, len,
							      phy);

	if (ret == EmissionRunTime::OK && data->outfile) {
		fwrite(samples_next, sizeof(std::complex<short>), len, data->outfile);
		data->sample_count += len;
	}

	return ret == EmissionRunTime::OK;
}

void thread_tx(struct bladerf *dev, std::mutex *mutex_conf, phy_parameters_t phy,
	       EmissionRunTime *txRT, std::string file)
{
	struct tx_data data;
	data.txRT = txRT;

	if (file != std::string()) {
		char filename[100];
		snprintf(filename, sizeof(filename), "%s-%.0fkHz-%.0fkSPS.dat",
			file.c_str(), phy.central_freq / 1000, phy.sample_rate / 1000);

		data.outfile = fopen(filename, "wb");
		if (data.outfile)
			std::cout << "TX: Recording samples to '" << filename << "'." << std::endl;
		else
			std::cerr << "TX: Failed to open '" << filename << "'." << std::endl;
		data.sample_count = 0;
	}

	bool phy_ok = true;
	do {
		if (brf_start_stream(dev, BLADERF_MODULE_TX, 0.001, 4096, phy,
				 brf_TX_stream_cb, &data) && !stop_signal_called) {

			mutex_conf->lock();
			phy_ok = brf_set_phy(dev, BLADERF_MODULE_TX, phy);
			mutex_conf->unlock();
		}
	} while (phy_ok && !stop_signal_called);

	if (data.outfile) {
		fclose(data.outfile);
		std::cerr << "TX: Wrote " << data.sample_count << " samples to the disk" << std::endl;
	}
}

void ringBell(EmissionRunTime *txRT, size_t channel, size_t music)
{
	uint8_t channels[][2] = {
		{ 0x2a, 0xaa },
		{ 0x52, 0xaa },
		{ 0x32, 0xaa },
		{ 0x55, 0x2a },
		{ 0x35, 0x2a },
		{ 0x4d, 0x2a },
		{ 0x2d, 0x2a },
		{ 0x4c, 0xaa },
		{ 0x2c, 0xaa },
		{ 0x54, 0xaa },
		{ 0x34, 0xaa },
		{ 0x53, 0x2a },
		{ 0x33, 0x2a },
		{ 0x4b, 0x2a },
		{ 0x2b, 0x2a },
		{ 0x4a, 0xaa },
	};
	uint8_t musics[] = { 0xb2, 0xcc, 0xca };

	if (channel > 15 || music > 2)
		return;

	Message m;
	m.addByte(channels[channel][0]);
	m.addByte(channels[channel][1]);
	m.addByte(musics[music]);
	m.addBit(true);
	m.setRepeatCount(5);

	ModulationOOK::SymbolOOK sOn(261.2, 527.2);
	ModulationOOK::SymbolOOK sOff(270.8, 536.3);
	ModulationOOK::SymbolOOK sStop(4000);
	m.setModulation(std::shared_ptr<ModulationOOK>(new ModulationOOK(433.9e6,
									 sOn, sOff,
									 sStop)));
	txRT->addMessage(m);
}

bool sendMessage(EmissionRunTime *txRT, Message &m)
{
	/*ModulationOOK::SymbolOOK sOn(100, 200);
	ModulationOOK::SymbolOOK sOff(100, 200);
	ModulationOOK::SymbolOOK sStop(500);
	m.setModulation(std::shared_ptr<ModulationOOK>(new ModulationOOK(txFreq + 1e5,
									 sOn, sOff,
									 sStop)));*/
	/*m.setModulation(std::shared_ptr<Modulation>(new ModulationFSK(txFreq + 1e5,
								      100.0e3,
								      100e3, 1)));*/

	m.setModulation(std::shared_ptr<Modulation>(new ModulationPSK(txFreq + 1e5,
								      10e3, 1)));

	return txRT->addMessage(m);
}

int main(int argc, char *argv[])
{
	phy_parameters_t phyRX, phyTX;
	struct bladerf *dev;
	std::string rxFile, txFile;

	TapInterface tapInterface("tap_brf");
	std::thread tRx, tTx;
	std::mutex mutex_conf;
	EmissionRunTime *txRT = NULL;
	bool defaultRXGain = false, defaultTXGain = false;

	//setup the program options
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "help message")
		("rx-rate", po::value<float>(&phyRX.sample_rate)->default_value(1e6), "rate of incoming samples")
		("rx-freq", po::value<float>(&phyRX.central_freq)->default_value(0.0), "RX RF center frequency in Hz")
		("rx-gain", po::value<float>(&phyRX.gain), "gain for the RX RF chain")
		("rx-bw", po::value<float>(&phyRX.gain), "bandwidth of the RF RX filter")
		("rx-file", po::value<std::string>(&rxFile), "output all the samples to this file")
		("tx-rate", po::value<float>(&phyTX.sample_rate)->default_value(1e6), "rate of outgoing samples")
		("tx-freq", po::value<float>(&phyTX.central_freq)->default_value(0.0), "TX RF center frequency in Hz")
		("tx-gain", po::value<float>(&phyTX.gain), "gain for the TX RF chain")
		("tx-bw", po::value<float>(&phyTX.gain), "bandwidth of the RF TX filter")
		("tx-file", po::value<std::string>(&txFile), "output all the samples to this file")
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	//print the help message
	if (vm.count("help")){
		std::cout << boost::format("Hachoir BRF %s") % desc << std::endl;
		return ~0;
	}

	if (!vm.count("rx-freq") || !vm.count("rx-rate") ||
	    !vm.count("tx-freq") || !vm.count("tx-rate")) {
		std::cerr << "The RX-TX frequency and rate need to be set!" << std::endl;
		return 1;
	}

	if (not vm.count("rx-bw"))
		phyRX.IF_bw = phyRX.sample_rate;
	if (not vm.count("tx-bw"))
		phyTX.IF_bw = phyTX.sample_rate;

	if (not vm.count("rx-gain")) {
		defaultRXGain = true;
		phyRX.gain = -999.0;
	}
	if (not vm.count("rx-gain")) {
		defaultTXGain = true;
		phyTX.gain = -999.0;
	}

	txFreq = phyTX.central_freq;

	// find one device
	dev = brf_open_and_init(NULL);
	if(!dev)
		return 1;

	brf_set_phy(dev, BLADERF_MODULE_RX, phyRX, defaultRXGain);
	brf_set_phy(dev, BLADERF_MODULE_TX, phyTX, defaultTXGain);

	std::signal(SIGINT, &sig_int_handler);
	std::signal(SIGTERM, &sig_int_handler);
	std::signal(SIGQUIT, &sig_int_handler);
	std::signal(SIGABRT, &sig_int_handler);
	std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

	txRT = new EmissionRunTime(30, 4096, 2040);

	tRx = std::thread(thread_rx, dev, &mutex_conf, phyRX, &tapInterface, rxFile);
	tTx = std::thread(thread_tx, dev, &mutex_conf, phyTX, txRT, txFile);

	system("rm samples.csv");

	do {
		/*sleep(2 + rand() % 20);

		size_t music = (rand() % 300) / 100;
		std::cout << "DING DONG (music " << music << ")" << std::endl;
		for (size_t i = 0; i < 16; i++)
			ringBell(txRT, i, music);*/

		Message m = tapInterface.readNextMessage();
		std::cout << time_abs() << ": " << m.toString(Message::HEX) << std::endl;
		sendMessage(txRT, m);
		last_message = time_abs();

	} while (!stop_signal_called);

	tRx.join();
	tTx.join();

	delete txRT;

	bladerf_close(dev);

	std::cout << "Done!" << std::endl;

	return EXIT_SUCCESS;
}
