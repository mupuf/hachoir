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

	for(size_t i = 0; i < 1000; i++) {
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

static void brf_init(struct bladerf *dev)
{
	char serial[BLADERF_SERIAL_LENGTH];
	bladerf_fpga_size size;
	const char *usbSpeed;
	int fpga_loaded;

	BLADERF_CALL_EXIT(bladerf_get_serial(dev, serial));

	bladerf_dev_speed speed = bladerf_device_speed(dev);
	switch (speed) {
	case BLADERF_DEVICE_SPEED_UNKNOWN:
		usbSpeed = "Unknown";
		break;
	case BLADERF_DEVICE_SPEED_HIGH:
		usbSpeed = "high (USB 2.0)";
		break;
	case BLADERF_DEVICE_SPEED_SUPER:
		usbSpeed = "super (USB 3.0)";
		break;
	}

	fpga_loaded = bladerf_is_fpga_configured(dev);

	BLADERF_CALL_EXIT(bladerf_get_fpga_size(dev, &size));

	std::cout << "BladeRF opened: " << std::endl
		  << "	Serial:		" << serial << std::endl
		  << "	USB speed:	" << usbSpeed << std::endl
		  << "	FPGA size:	" << size << " KLE" << std::endl
		  << "	FPGA loaded:	" << (fpga_loaded ? "Yes" : "No") << std::endl
		  << std::endl;

	if (!fpga_loaded) {
		std::string path = boost::str(boost::format("/usr/share/bladerf/fpga/hostedx%i.rbf") % size);

		std::cout << "Load FPGA image '" << path << "': ";
		BLADERF_CALL_EXIT(bladerf_load_fpga(dev, path.c_str()));
		std::cout << " OK" << std::endl;
	}

	//correctRXIQ(dev);
}

struct bladerf *brf_open_and_init(const char *device_identifier, bladerf_backend backend)
{
	struct bladerf *dev = NULL;
	struct bladerf_devinfo *devices;
	int device_count, ret;

	// List devices
	device_count = bladerf_get_device_list(&devices);
	std::cout << "Found " << device_count << " bladeRF:" << std::endl;
	for (int i = 0; i < device_count; i++) {
		std::cout << "	" << devices[i].instance
			  << ": serial = " << devices[i].serial << std::endl;
	}
	std::cout << std::endl;

	if (device_identifier)
		BLADERF_CALL_EXIT(bladerf_open(&dev, device_identifier));
	else {
		for (int i = 0; i < device_count; i++) {
			devices[i].backend = backend;
			ret = bladerf_open_with_devinfo(&dev, &devices[i]);
			if (ret) {
				std::cerr << "Cannot open bladeRF id " << devices[i].instance
					  << " (err = '" << bladerf_strerror(ret)
					  << "). Trying the next one..."
					  << std::endl;
				dev = NULL;
			}
		}
	}

	if (dev)
		brf_init(dev);
	else
		std::cerr << std::endl << "Failed to open any bladeRF device" << std::endl;

	return dev;
}

bool brf_set_phy(struct bladerf *dev, bladerf_module module,
		 phy_parameters_t &phy, bool defaultGain, bool resetIQ)
{
	const char *mod = (module == BLADERF_MODULE_RX ? "RX":"TX");
	unsigned int actual;
	int gainMin, gainMax;

	std::cout << boost::format("%s PHY change: %.3f MHz, %.2f MS/s, BW %.2f MHz")
		     % mod % (phy.central_freq / 1e6)
		     % (phy.sample_rate / 1e6)
		     % (phy.IF_bw / 1e6);

	if (defaultGain)
		std::cout << ", default gain" << std::endl;
	else
		std::cout << boost::format(", gain %.02f dB") % phy.gain << std::endl;

	//set the center frequency
	BLADERF_CALL(bladerf_set_frequency(dev, module, phy.central_freq));
	BLADERF_CALL(bladerf_get_frequency(dev, module, &actual));
	phy.central_freq = 1.0 * actual;

	//set the sample rate
	if (phy.sample_rate <= 0.0){
		std::cerr << "Please specify a valid sample rate" << std::endl;
		return false;
	}
	BLADERF_CALL(bladerf_set_sample_rate(dev, module, phy.sample_rate, &actual));
	phy.sample_rate = 1.0 * actual;

	// set the bandwidth
	BLADERF_CALL(bladerf_set_bandwidth(dev, module, phy.IF_bw, &actual));
	phy.IF_bw = 1.0 * actual;

	//set the rf gain
	BLADERF_CALL(bladerf_set_lna_gain(dev, BLADERF_LNA_GAIN_MID));
	if (module == BLADERF_MODULE_RX) {
		int rxvga1, rxvga2;

		if (defaultGain) {
			rxvga1 = BLADERF_RXVGA1_GAIN_MIN + (BLADERF_RXVGA1_GAIN_MAX - BLADERF_RXVGA1_GAIN_MIN) / 2;
			rxvga2 = BLADERF_RXVGA2_GAIN_MIN + (BLADERF_RXVGA2_GAIN_MAX - BLADERF_RXVGA2_GAIN_MIN) / 2;
		} else {
			rxvga1 = phy.gain / 2.0;
			rxvga2 = phy.gain / 2.0;
		}

		BLADERF_CALL(bladerf_set_rxvga1(dev, rxvga1));
		BLADERF_CALL(bladerf_get_rxvga1(dev, &rxvga1));

		BLADERF_CALL(bladerf_set_rxvga2(dev, rxvga2));
		BLADERF_CALL(bladerf_get_rxvga2(dev, &rxvga2));

		phy.gain = rxvga1 + rxvga2;
		gainMin = BLADERF_RXVGA1_GAIN_MIN + BLADERF_RXVGA2_GAIN_MIN;
		gainMax = BLADERF_RXVGA1_GAIN_MAX + BLADERF_RXVGA2_GAIN_MAX;
	} else {
		int txvga1, txvga2;

		if (defaultGain) {
			txvga1 = BLADERF_TXVGA1_GAIN_MIN + (BLADERF_TXVGA1_GAIN_MAX - BLADERF_TXVGA1_GAIN_MIN) / 2;
			txvga2 = BLADERF_TXVGA2_GAIN_MIN + (BLADERF_TXVGA2_GAIN_MAX - BLADERF_TXVGA2_GAIN_MIN) / 2;
		} else {
			txvga1 = phy.gain / 2.0;
			txvga2 = phy.gain / 2.0;
		}

		BLADERF_CALL(bladerf_set_txvga1(dev, txvga1));
		BLADERF_CALL(bladerf_get_txvga1(dev, &txvga1));

		BLADERF_CALL(bladerf_set_txvga2(dev, txvga2));
		BLADERF_CALL(bladerf_get_txvga2(dev, &txvga2));

		phy.gain = txvga1 + txvga2;
		gainMin = BLADERF_TXVGA1_GAIN_MIN + BLADERF_TXVGA2_GAIN_MIN;
		gainMax = BLADERF_TXVGA1_GAIN_MAX + BLADERF_TXVGA2_GAIN_MAX;
	}

	std::cout <<
		boost::format("%s actual PHY: %.3f MHz, %.2f MS/s, BW %.2f MHz, gain %.0f dB [%.0f, %.0f]")
		     % mod % (phy.central_freq / 1e6)
		     % (phy.sample_rate / 1e6)
		     % (phy.IF_bw / 1e6)
		     % phy.gain % gainMin % gainMax
		  << std::endl << std::endl;

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
	size_t num_transfers;
	size_t block_size;
	unsigned int buf_idx;
	int exit_in;

	bladerf_module module;
	phy_parameters_t phy;
	brf_stream_cb user_cb;
	void *user_data;

	int lastError;
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
	const char *mod_s;
	bool ret = true;

	if (module == BLADERF_MODULE_TX) {
		mod_s = "TX";
		internal_cb = bladerf_TX_cb;
	} else {
		mod_s = "RX";
		internal_cb = bladerf_RX_cb;
	}

	data.num_transfers = ceilf(1.1 * phy.sample_rate * buffering_min / block_size);
	if (data.num_transfers < 1)
		data.num_transfers = 1;
	data.buffers_count = data.num_transfers * 2;

	data.block_size = block_size;
	data.buf_idx = 0;
	data.module = module;
	data.phy = phy;
	data.exit_in = -1;
	data.user_cb = cb;
	data.user_data = user;

	std::cout << "val = " << (data.buffers_count / (buffering_min)) * block_size << std::endl;

	//BLADERF_CALL(bladerf_set_stream_timeout(dev, module, buffering_min * 1000));

	BLADERF_CALL(bladerf_init_stream(&stream, dev, internal_cb, &data.buffers,
			    data.buffers_count, BLADERF_FORMAT_SC16_Q11,
			    data.block_size, data.buffers_count, &data));

	BLADERF_CALL(bladerf_enable_module(dev, module, true));

	int error = bladerf_stream(stream, module);
	if (error < 0) {
		std::cerr << mod_s << " streaming error: " << bladerf_strerror(error) << std::endl;
		ret = false;
	}

	BLADERF_CALL(bladerf_enable_module(dev, module, false));

	bladerf_deinit_stream(stream);

	return ret;

}
