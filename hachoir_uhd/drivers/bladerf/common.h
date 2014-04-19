#ifndef COMMON_H
#define COMMON_H

#include <libbladeRF.h>
#include <iostream>
#include <complex>

#include "utils/phy_parameters.h"

#define BLADERF_CALL_EXIT(call) do { \
		int ret = call; \
		if (ret < 0) { \
			std::cerr << __FILE__ << "(" << __LINE__ << "): " << #call << " failed: " << bladerf_strerror(ret) << std::endl; \
			exit(1); \
		} \
	} while(0)

#define BLADERF_CALL(call) do { \
		int ret = call; \
		if (ret < 0) \
			std::cerr << __FILE__ << ", " << __LINE__ << ": " << #call << " failed: " << bladerf_strerror(ret) << std::endl; \
	} while(0)

struct bladerf *
brf_open_and_init(const char *device_identifier);

bool
brf_set_phy(struct bladerf *dev, bladerf_module module,
	    phy_parameters_t &phy, bool resetIQ = false);

typedef bool(*brf_stream_cb )(struct bladerf *dev, struct bladerf_stream *stream,
				struct bladerf_metadata *meta,
				std::complex<short> *samples_next, size_t len,
				phy_parameters_t &phy, void *user_data);

bool
brf_start_stream(struct bladerf *dev, bladerf_module module,
		  float buffering_min, size_t block_size,
		  phy_parameters_t &phy,
		  brf_stream_cb cb, void *user);

#endif // COMMON_H
