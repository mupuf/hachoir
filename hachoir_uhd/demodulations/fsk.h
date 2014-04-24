#ifndef FSK_H
#define FSK_H

#include "demodulator.h"

class FSK : public Demodulator
{
	struct symbolFSK {
		float diff_phase;
		float len;
	};

	std::vector<symbolFSK> _cnt_table;
	float _TS;
	float _threshold;

	std::string _phy_params;
public:
	FSK();

	uint8_t likeliness(const burst_t * const burst);
	std::vector<Message> demod(const burst_t * const burst);
	std::string modulationString() const;
};

#endif // FSK_H
