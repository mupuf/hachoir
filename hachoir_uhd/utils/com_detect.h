#ifndef COM_DETECT_H
#define COM_DETECT_H

#include <stddef.h>
#include <complex>
#include <string>

#include "phy_parameters.h"

enum process_return_val_t {
	RET_NOP = 0,
	RET_CH_PHY
};

process_return_val_t process_samples(phy_parameters_t &phy, uint64_t time_us,
			  std::complex<short> *samples, size_t count);

#endif // COM_DETECT_H
