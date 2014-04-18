#include "common.h"

#include <string.h>
#include <complex>
#include <string>

#include <boost/format.hpp>

static bool correctRXIQ(struct bladerf *dev)
{
	std::complex<short> samples[4096];
	size_t len = 4096;
	int64_t I_sum = 0, Q_sum = 0;
	size_t sum = 0;
	int ret;

	// reset the I/Q coff
	BLADERF_CALL(bladerf_set_correction(dev, BLADERF_MODULE_RX,
					    BLADERF_CORR_LMS_DCOFF_I, 0));
	BLADERF_CALL(bladerf_set_correction(dev, BLADERF_MODULE_RX,
					    BLADERF_CORR_LMS_DCOFF_Q, 0));

	// enable the sync RX mode
	BLADERF_CALL(bladerf_sync_config(dev, BLADERF_MODULE_RX,
					 BLADERF_FORMAT_SC16_Q11,
					 64, 16384, 16, 0));

	// enable the RX module
	BLADERF_CALL(bladerf_enable_module(dev, BLADERF_MODULE_RX, true));

	for(size_t i = 0; i < 100; i++) {
		ret = bladerf_sync_rx(dev, samples, len, NULL, 1000);
		if (ret) {
			std::cerr << "bladerf_sync_rx: " << bladerf_strerror(ret) << std::endl;
			//return false;
		}

		for (size_t e = 0; e < len; e++) {
			I_sum += samples[e].real();
			Q_sum += samples[e].imag();
		}
		sum += len;
	}

	float I_avr = 1.0 * I_sum / sum;
	float Q_avr = 1.0 * Q_sum / sum;

	short I_coff = I_avr / 2.0, Q_coff = Q_avr / 2.0;

	std::cout << boost::format("Found RX DC-Offset: I = %f, Q = %f")
		     % I_avr % Q_avr << std::endl << std::endl;

	BLADERF_CALL(bladerf_set_correction(dev, BLADERF_MODULE_RX,
					    BLADERF_CORR_LMS_DCOFF_I, I_coff));

	BLADERF_CALL(bladerf_set_correction(dev, BLADERF_MODULE_RX,
					    BLADERF_CORR_LMS_DCOFF_Q, Q_coff));

	// disable the RX module
	BLADERF_CALL(bladerf_enable_module(dev, BLADERF_MODULE_RX, false));

	return true;
}

struct bladerf *brf_open_and_init(const char *device_identifier)
{
	struct bladerf *dev;
	char serial[BLADERF_SERIAL_LENGTH];
	int ret;

	BLADERF_CALL_EXIT(bladerf_open(&dev, device_identifier));
	BLADERF_CALL_EXIT(bladerf_get_serial(dev, serial));

	std::cout << "BladeRF opened: " << std::endl
		  << "	Serial = " << serial << std::endl
		  << std::endl;

	ret = bladerf_is_fpga_configured(dev);
	if (!ret) {
		bladerf_fpga_size size;
		BLADERF_CALL_EXIT(bladerf_get_fpga_size(dev, &size));

		std::string path = boost::str(boost::format("/usr/share/bladerf/fpga/hostedx%i.rbf") % size);
		BLADERF_CALL_EXIT(bladerf_load_fpga(dev, path.c_str()));

		std::cout << "FPGA is not loaded:" << std::endl
			  << "	Found size " << size << " KLE" << std::endl
			  << "	Loaded FPGA image '" << path << "'" << std::endl
			  << std::endl;
	}

	correctRXIQ(dev);

	return dev;
}

bool brf_set_phy(struct bladerf *dev, bladerf_module module,
		 const phy_parameters_t &phy, bool resetIQ)
{
	const char *mod = (module == BLADERF_MODULE_RX ? "RX":"TX");
	bladerf_lna_gain lna_gain = BLADERF_LNA_GAIN_MID;
	unsigned int actual;

	//set the center frequency
	std::cout << boost::format("Setting %s Freq: %u MHz...") % mod
		     % (phy.central_freq / 1e6) << std::endl;
	BLADERF_CALL(bladerf_set_frequency(dev, module, phy.central_freq));
	BLADERF_CALL(bladerf_get_frequency(dev, module, &actual));
	std::cout << boost::format("Actual %s Freq: %u MHz...") % mod
		     % (actual / 1.0e6) << std::endl << std::endl;

	//set the sample rate
	if (phy.sample_rate <= 0.0){
		std::cerr << "Please specify a valid sample rate" << std::endl;
		return false;
	}
	std::cout << boost::format("Setting %s Rate: %f Msps...") % mod
		     % (phy.sample_rate/1e6) << std::endl;
	BLADERF_CALL(bladerf_set_sample_rate(dev, module, phy.sample_rate, &actual));
	std::cout << boost::format("Actual %s Rate: %f Msps...") % mod
		     % (actual/1.0e6) << std::endl << std::endl;

	// set the bandwidth
	std::cout << boost::format("Setting %s Bandwidth: %f MHz...") % mod
		     % (phy.sample_rate/1e6) << std::endl;
	BLADERF_CALL(bladerf_set_bandwidth(dev, module, phy.sample_rate, &actual));
	std::cout << boost::format("Actual %s Bandwidth: %f MHz...") % mod
		     % (actual/1.0e6) << std::endl << std::endl;

	//set the rf gain
	if (module == BLADERF_MODULE_RX) {
		int rxvga1 = BLADERF_RXVGA1_GAIN_MIN + (BLADERF_RXVGA1_GAIN_MAX - BLADERF_RXVGA1_GAIN_MIN) / 2;
		int rxvga2 = BLADERF_RXVGA2_GAIN_MIN + (BLADERF_RXVGA2_GAIN_MAX - BLADERF_RXVGA2_GAIN_MIN) / 2;

		std::cout << boost::format("Setting RX VGA1 Gain: %i dB [%i, %i]...")
			     % rxvga1 % BLADERF_RXVGA1_GAIN_MIN
			     % BLADERF_RXVGA1_GAIN_MAX << std::endl;
		BLADERF_CALL(bladerf_set_rxvga1(dev, rxvga1));
		BLADERF_CALL(bladerf_get_rxvga1(dev, &rxvga1));
		std::cout << boost::format("Actual RX VGA1 Gain: %f dB...")
			     % (rxvga1) << std::endl << std::endl;

		std::cout << boost::format("Setting RX VGA2 Gain: %i dB [%i, %i] ...")
			     % rxvga2 % BLADERF_RXVGA2_GAIN_MIN
			     % BLADERF_RXVGA2_GAIN_MAX << std::endl;
		BLADERF_CALL(bladerf_set_rxvga2(dev, rxvga2));
		BLADERF_CALL(bladerf_get_rxvga2(dev, &rxvga2));
		std::cout << boost::format("Actual RX VGA2 Gain: %f dB...")
			     % rxvga2 << std::endl << std::endl;
	} else {
		int txvga1 = BLADERF_TXVGA1_GAIN_MIN + (BLADERF_TXVGA1_GAIN_MAX - BLADERF_TXVGA1_GAIN_MIN) / 2;
		int txvga2 = BLADERF_TXVGA2_GAIN_MIN + (BLADERF_TXVGA2_GAIN_MAX - BLADERF_TXVGA2_GAIN_MIN) / 2;

		std::cout << boost::format("Setting TX VGA1 Gain: %i dB ...")
			     % (txvga1) << std::endl;
		BLADERF_CALL(bladerf_set_txvga1(dev, txvga1));
		BLADERF_CALL(bladerf_get_txvga1(dev, &txvga1));
		std::cout << boost::format("Actual TX VGA1 Gain: %f dB...")
			     % (txvga1) << std::endl << std::endl;

		std::cout << boost::format("Setting TX VGA2 Gain: %i dB ...")
			     % (txvga2) << std::endl;
		BLADERF_CALL(bladerf_set_txvga2(dev, txvga2));
		BLADERF_CALL(bladerf_get_txvga2(dev, &txvga2));
		std::cout << boost::format("Actual TX VGA2 Gain: %f dB...")
			     % (txvga2) << std::endl << std::endl;
	}

	std::cout << boost::format("Setting LNA Gain: %i dB [%i, %i] ...")
		     % lna_gain % BLADERF_LNA_GAIN_BYPASS
		     % BLADERF_LNA_GAIN_MAX << std::endl;
	BLADERF_CALL(bladerf_set_lna_gain(dev, BLADERF_LNA_GAIN_MID));
	BLADERF_CALL(bladerf_get_lna_gain(dev, &lna_gain));
	std::cout << boost::format("Actual LNA Gain: %f dB...")
		     % (lna_gain) << std::endl << std::endl;

	// reset the I/Q coff
	if (resetIQ) {
		BLADERF_CALL(bladerf_set_correction(dev, module,
						    BLADERF_CORR_LMS_DCOFF_I, 0));
		BLADERF_CALL(bladerf_set_correction(dev, module,
						    BLADERF_CORR_LMS_DCOFF_Q, 0));
	}

	return true;
}

struct stream_data {
	void **buffers;
	size_t buffers_count;
	size_t block_size;
	unsigned int buf_idx;
	int exit_in;

	bladerf_module module;
	phy_parameters_t phy;
	brf_stream_cb user_cb;
	void *user_data;
};

static void* bladerf_RX_cb(struct bladerf *dev, struct bladerf_stream *stream,
		    struct bladerf_metadata *meta, void *samples,
		    size_t num_samples, void *user_data)
{
	struct stream_data *data = (struct stream_data *)user_data;

	if (!data->user_cb(dev, stream, meta, (std::complex<short> *)samples,
			   num_samples, data->phy, data->user_data))
		return NULL;

	std::complex<short> *buf = (std::complex<short> *)data->buffers[data->buf_idx];
	data->buf_idx = (data->buf_idx + 1) % data->buffers_count;

	return buf;
}

static void* bladerf_TX_cb(struct bladerf *dev, struct bladerf_stream *stream,
		    struct bladerf_metadata *meta, void *samples,
		    size_t num_samples, void *user_data)
{
	struct stream_data *data = (struct stream_data *)user_data;

	std::complex<short> *buf = (std::complex<short> *)data->buffers[data->buf_idx];
	data->buf_idx = (data->buf_idx + 1) % data->buffers_count;

	if (data->exit_in < 0) {
		if (data->user_cb(dev, stream, meta, buf, num_samples,
				  data->phy, data->user_data))
			return buf;
		else
			data->exit_in = data->buffers_count + 1;
	} else if (data->exit_in == 0)
		return NULL;

	/* we need to change frequency, send enough samples for the radio
	 * to get all the good samples before stopping the stream
	 */

	// fill the buffer with 0's
	memset(buf, 0, sizeof(std::complex<short>) * data->block_size);

	data->exit_in--;

	return buf;
}

bool brf_start_stream(struct bladerf *dev, bladerf_module module,
		      float buffering_min, size_t block_size,
		      phy_parameters_t &phy,
		      brf_stream_cb cb, void *user)
{
	struct bladerf_stream *stream;
	bladerf_stream_cb internal_cb;
	struct stream_data data;

	if (module == BLADERF_MODULE_TX) {
		internal_cb = bladerf_TX_cb;
		data.buffers_count = phy.sample_rate * buffering_min / data.block_size;
		if (data.buffers_count < 2)
			data.buffers_count = 2;
	} else {
		internal_cb = bladerf_RX_cb;
		data.buffers_count = 2;
	}

	data.block_size = block_size;
	data.buf_idx = 0;
	data.module = module;
	data.phy = phy;
	data.exit_in = -1;
	data.user_cb = cb;
	data.user_data = user;

	BLADERF_CALL(bladerf_init_stream(&stream, dev, internal_cb, &data.buffers,
			    data.buffers_count, BLADERF_FORMAT_SC16_Q11,
			    data.block_size, data.buffers_count, &data));

	BLADERF_CALL(bladerf_enable_module(dev, module, true));

	BLADERF_CALL(bladerf_stream(stream, module));

	BLADERF_CALL(bladerf_enable_module(dev, module, false));

	bladerf_deinit_stream(stream);

	phy = data.phy;

	return true;

}
