#ifndef FSK_H
#define FSK_H

#include "demodulator.h"

class FSK : public Demodulator
{
	std::vector<size_t> _cnt_table;
	size_t _threshold;

	std::string _phy_params;
public:
	FSK();

	uint8_t likeliness(const burst_sc16_t * const burst);
	std::vector<Message> demod(const burst_sc16_t * const burst);
};

#endif // FSK_H
