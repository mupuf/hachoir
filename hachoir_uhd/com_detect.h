#ifndef COM_DETECT_H
#define COM_DETECT_H

#include <uhd/stream.hpp>
#include <stddef.h>

void process_samples_sc16(uhd::rx_metadata_t md, std::complex<short> *samples, size_t count);
void process_samples_fc32(uhd::rx_metadata_t md, std::complex<float> *samples, size_t count);
void process_samples_fc64(uhd::rx_metadata_t md, std::complex<double> *samples, size_t count);

template<typename samp_type> void process_samples(uhd::rx_metadata_t md,
						  const std::string &cpu_format,
						  samp_type *samples, size_t count)
{
	if (cpu_format == "sc16")
		process_samples_sc16(md, (std::complex<short> *) samples, count);
	else if (cpu_format == "fc32")
		process_samples_fc32(md, (std::complex<float> *) samples, count);
	else if (cpu_format == "fc64")
		process_samples_fc64(md, (std::complex<double> *) samples, count);
}

#endif // COM_DETECT_H
