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

#include <time.h>
int64_t clock_read_us()
{
	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp))
		return 0;

	return tp.tv_sec * 1000000ULL + tp.tv_nsec / 1000;
}

struct tx_data {
	void **buffers;
	size_t buffers_count;
	size_t block_size;
	unsigned int buf_idx;

	EmissionRunTime *txRT;
	phy_parameters_t phy;
};

void* bladerf_TX_cb(struct bladerf *dev, struct bladerf_stream *stream,
		    struct bladerf_metadata *meta, void *samples,
		    size_t num_samples, void *user_data)
{
	struct tx_data *data = (struct tx_data *)user_data;

	std::complex<short> *buf = (std::complex<short> *)data->buffers[data->buf_idx];
	data->buf_idx = (data->buf_idx + 1) % data->buffers_count;

	EmissionRunTime::Command ret = data->txRT->next_block(buf, num_samples, data->phy);
	if (ret == EmissionRunTime::OK)
		return buf;
	else
		return NULL;
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
	if(bladerf_open(&dev, NULL)) {
		std::cerr << "bladerf_open: couldn't open the bladeRF device" << std::endl;
		return -1;
	}

	if (!brf_set_phy(dev, BLADERF_MODULE_TX, phy))
		return 1;

	std::signal(SIGINT, &sig_int_handler);
	std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

	struct tx_data data;
	data.block_size = 4096;
	data.buffers_count = 0;
	data.buf_idx = 0;
	data.txRT = new EmissionRunTime(10, 4096, phy, 2040);
	data.phy = phy;

	Message m({0x55, 0x2a, 0xb2});
	m.addBit(true);
	m.setRepeatCount(10);

	ModulationOOK::SymbolOOK sOn(261.2, 527.2);
	ModulationOOK::SymbolOOK sOff(270.8, 536.3);
	ModulationOOK::SymbolOOK sStop(9300);
	m.setModulation(std::shared_ptr<ModulationOOK>(new ModulationOOK(433.9e6, sOn, sOff, sStop)));

	data.txRT->addMessage(m);

	struct bladerf_stream *stream;
	float timeout = 0.02; // 20 ms
	data.buffers_count = phy.sample_rate * timeout / data.block_size;
	if (data.buffers_count < 2) data.buffers_count = 2;

	BLADERF_CALL(bladerf_init_stream(&stream, dev, bladerf_TX_cb, &data.buffers,
			    data.buffers_count, BLADERF_FORMAT_SC16_Q11,
			    data.block_size, data.buffers_count, &data));

	while (1) {

		BLADERF_CALL(bladerf_set_stream_timeout(dev, BLADERF_MODULE_TX, timeout));
		BLADERF_CALL(bladerf_enable_module(dev, BLADERF_MODULE_TX, true));

		BLADERF_CALL(bladerf_stream(stream, BLADERF_MODULE_TX));

		BLADERF_CALL(bladerf_enable_module(dev, BLADERF_MODULE_TX, false));

		if (!brf_set_phy(dev, BLADERF_MODULE_TX, data.phy))
			return 1;
	}

	bladerf_deinit_stream(stream);
	bladerf_close(dev);

	return EXIT_SUCCESS;
}
