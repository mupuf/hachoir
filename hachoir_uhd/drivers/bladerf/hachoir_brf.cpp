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
#include "utils/tapinterface.h"
#include "utils/emissionruntime.h"
#include "modulations/modulationOOK.h"
#include "modulations/modulationFSK.h"

#include "common.h"

namespace po = boost::program_options;

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
	std::ofstream outfile;
	uint64_t sample_count;
};

bool brf_RX_stream_cb(struct bladerf *dev, struct bladerf_stream *stream,
				struct bladerf_metadata *meta,
				std::complex<short> *samples, size_t len,
				phy_parameters_t &phy, void *user_data)
{
	struct rx_data *data = (struct rx_data *)user_data;

	if (data->outfile.is_open()) {
		data->outfile.write((const char*)&samples,
				    len * sizeof(std::complex<unsigned short>));
		data->sample_count += len;
	}

	if (stop_signal_called)
		return false;

	return !process_samples(phy, time_us(), "sc16", samples, len);
}

void thread_rx(struct bladerf *dev, std::mutex *mutex_conf, phy_parameters_t phy,
	       const std::string &file = std::string())
{
	struct rx_data data;

	if (file != std::string()) {
		char filename[100];
		snprintf(filename, sizeof(filename), "%s-%.0fkHz-%.0fkSPS.dat",
			file.c_str(), phy.central_freq / 1000, phy.sample_rate / 1000);

		data.outfile.open(filename, std::ofstream::binary);
		if (data.outfile.is_open())
			std::cout << "Recording samples to '" << filename << "'." << std::endl;
		else
			std::cerr << "Failed to open '" << filename << "'." << std::endl;
		data.sample_count = 0;
	}

	bool phy_ok;
	do {
		mutex_conf->lock();
		phy_ok = brf_set_phy(dev, BLADERF_MODULE_RX, phy);
		sleep(1);
		mutex_conf->unlock();
		if (!phy_ok)
			continue;

		brf_start_stream(dev, BLADERF_MODULE_RX, 0.01, 4096, phy,
				 brf_RX_stream_cb, &data);
	} while (phy_ok && !stop_signal_called);

	if (data.outfile.is_open()) {
		data.outfile.close();
		std::cout << "Wrote " << data.sample_count << " samples to the disk" << std::endl;
	}
}

struct tx_data {
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
	return ret == EmissionRunTime::OK;
}

void thread_tx(struct bladerf *dev, std::mutex *mutex_conf, phy_parameters_t phy,
	       EmissionRunTime *txRT)
{
	struct tx_data data;
	data.txRT = txRT;

	bool phy_ok;
	do {
		mutex_conf->lock();
		phy_ok = brf_set_phy(dev, BLADERF_MODULE_TX, phy);
		sleep(1);
		mutex_conf->unlock();
		if (!phy_ok)
			continue;

		brf_start_stream(dev, BLADERF_MODULE_TX, 0.01, 4096, phy,
				 brf_TX_stream_cb, &data);
	} while (phy_ok && !stop_signal_called);
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
	m.setModulation(std::shared_ptr<ModulationOOK>(new ModulationOOK(433.9e6,
									 sOn, sOff,
									 sStop)));*/
	m.setModulation(std::shared_ptr<Modulation>(new ModulationFSK(433.6e6,
								      200.0e3,
								      100e3, 1)));

	return txRT->addMessage(m);
}

int main(int argc, char *argv[])
{
	phy_parameters_t phy;
	struct bladerf *dev;
	std::string file;

	TapInterface tapInterface("tap_brf");
	std::thread tRx, tTx;
	std::mutex mutex_conf;
	EmissionRunTime *txRT = NULL;

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
		std::cout << boost::format("Hachoir BRF %s") % desc << std::endl;
		return ~0;
	}

	phy.IF_bw = -1.0;
	if (not vm.count("gain"))
		phy.gain = -1.0;

	// find one device
	dev = brf_open_and_init(NULL);

	std::signal(SIGINT, &sig_int_handler);
	std::signal(SIGTERM, &sig_int_handler);
	std::signal(SIGQUIT, &sig_int_handler);
	std::signal(SIGABRT, &sig_int_handler);
	std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

	txRT = new EmissionRunTime(30, 4096, 2040);

	tRx = std::thread(thread_rx, dev, &mutex_conf, phy, file);
	tTx = std::thread(thread_tx, dev, &mutex_conf, phy, txRT);

	system("rm samples.csv");
	/*Message m({0xaa, 0x00, 0xff});
	m.setModulation(std::shared_ptr<Modulation>(new ModulationFSK(433.6e6, 250.0e3, 50e3, 1)));
	txRT->addMessage(m);*/

	sleep(4);

	do {
		/*sleep(2 + rand() % 20);

		size_t music = (rand() % 300) / 100;
		std::cout << "DING DONG (music " << music << ")" << std::endl;
		for (size_t i = 0; i < 16; i++)
			ringBell(txRT, i, music);*/

		Message m = tapInterface.readNextMessage();
		std::cout << time_abs() << ": " << m.toString(Message::HEX) << std::endl;
		sendMessage(txRT, m);

	} while (!stop_signal_called);

	tRx.join();
	tTx.join();

	bladerf_close(dev);

	std::cout << "Done!" << std::endl;

	return EXIT_SUCCESS;
}
