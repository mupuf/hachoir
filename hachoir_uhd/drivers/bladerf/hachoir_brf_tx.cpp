#include <libbladeRF.h>

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>

#include "modulations/modulationOOK.h"
#include "utils/emissionruntime.h"
#include "utils/com_detect.h"
#include "utils/message.h"
#include "common.h"
#include <time.h>

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

int64_t clock_read_us()
{
	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp))
		return 0;

	return tp.tv_sec * 1000000ULL + tp.tv_nsec / 1000;
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

	EmissionRunTime::Command ret = data->txRT->next_block(samples_next, len,
							      phy);
	return ret == EmissionRunTime::OK;
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

	if (!brf_set_phy(dev, BLADERF_MODULE_TX, phy))
		return 1;

	std::signal(SIGINT, &sig_int_handler);
	std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

	struct tx_data data;
	data.txRT = new EmissionRunTime(10, 4096, 2040);

	Message m({0x4a, 0xaa, 0xb2});
	m.addBit(true);
	m.setRepeatCount(10);

	ModulationOOK::SymbolOOK sOn(261.2, 527.2);
	ModulationOOK::SymbolOOK sOff(270.8, 536.3);
	ModulationOOK::SymbolOOK sStop(9300);
	m.setModulation(std::shared_ptr<ModulationOOK>(new ModulationOOK(433.9e6,
									 sOn, sOff,
									 sStop)));
	data.txRT->addMessage(m);


	Message m2({0x55, 0x2a, 0xb2});
	m2.addBit(true);
	m2.setRepeatCount(10);
	m2.setModulation(std::shared_ptr<ModulationOOK>(new ModulationOOK(869.9e6,
									  sOn, sOff,
									  sStop)));
	data.txRT->addMessage(m2);


	Message m3({0x55, 0x2a, 0xb2});
	m3.addBit(true);
	m3.setRepeatCount(10);
	m3.setModulation(std::shared_ptr<ModulationOOK>(new ModulationOOK(915.9e6,
									  sOn, sOff,
									  sStop)));
	data.txRT->addMessage(m3);


	while (1) {
		brf_start_stream(dev, BLADERF_MODULE_TX, 0.05, 4096, phy,
				 brf_TX_stream_cb, &data);

		if (!brf_set_phy(dev, BLADERF_MODULE_TX, phy))
			return 1;
	}

	bladerf_close(dev);

	return EXIT_SUCCESS;
}
