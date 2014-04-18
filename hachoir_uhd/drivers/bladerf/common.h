#ifndef COMMON_H
#define COMMON_H

#include <libbladeRF.h>
#include <iostream>

#include "utils/phy_parameters.h"

#define BLADERF_CALL_EXIT(call) do { \
		int ret = call; \
		if (ret < 0) { \
			std::cerr << "call failed: " << bladerf_strerror(ret) << std::endl; \
			exit(1); \
		} \
	} while(0)

#define BLADERF_CALL(call) do { \
		int ret = call; \
		if (ret < 0) \
			std::cerr << "call failed: " << bladerf_strerror(ret) << std::endl; \
	} while(0)

struct bladerf *
brf_open_and_init(const char *device_identifier);


bool
brf_set_phy(struct bladerf *dev, bladerf_module module,
	    const phy_parameters_t &phy, bool resetIQ = false);

#endif // COMMON_H
