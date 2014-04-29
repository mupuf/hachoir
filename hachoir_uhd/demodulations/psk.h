#ifndef PSK_H
#define PSK_H

#include "demodulator.h"

class PSK : public Demodulator
{
	struct symbolPSK {
		float diff_phase;
		float len;
	};

	std::vector<symbolPSK> _cnt_table;
	float _TS;
	float _threshold;

	std::string _phy_params;
public:
	PSK();

	uint8_t likeliness(const Burst &burst);
	std::vector<Message> demod(const Burst &burst);
	std::string modulationString() const;
};

#endif // PSK_H
