#ifndef COM_DETECT_H
#define COM_DETECT_H

#include <uhd/stream.hpp>
#include <stddef.h>
#include <string>

struct phy_parameters_t {
	float central_freq;
	float sample_rate;
	float IF_bw;
	float gain;
};

enum process_return_val_t {
	RET_NOP = 0,
	RET_CH_PHY
};

process_return_val_t process_samples_sc16(phy_parameters_t &phy, uhd::rx_metadata_t md,
			  std::complex<short> *samples, size_t count);
process_return_val_t process_samples_fc32(phy_parameters_t &phy, uhd::rx_metadata_t md,
				      std::complex<float> *samples, size_t count);
process_return_val_t process_samples_fc64(phy_parameters_t &phy, uhd::rx_metadata_t md,
				      std::complex<double> *samples, size_t count);

template<typename samp_type> process_return_val_t
process_samples(phy_parameters_t &phy, uhd::rx_metadata_t md,
		const std::string &cpu_format, samp_type *samples, size_t count)
{
	if (cpu_format == "sc16")
		return process_samples_sc16(phy, md, (std::complex<short> *) samples, count);
	else if (cpu_format == "fc32")
		return process_samples_fc32(phy, md, (std::complex<float> *) samples, count);
	else if (cpu_format == "fc64")
		return process_samples_fc64(phy, md, (std::complex<double> *) samples, count);

	return RET_NOP;
}

#endif // COM_DETECT_H
