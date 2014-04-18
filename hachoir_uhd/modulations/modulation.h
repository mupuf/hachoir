#ifndef MODULATION_H
#define MODULATION_H

#include <string>
#include <vector>
#include <complex>

#include "utils/message.h"
#include "utils/phy_parameters.h"

class Modulation
{
private:
	float _freq;
	float _phase;
	float _amp;
	float _carrier_freq;
	float _sample_rate;

	struct command {
		enum action_t {
			SET_FREQ, // in Hz
			SET_PHASE, // In radians
			SET_AMP, // scaling factor
			SET_CARRIER_FREQ, // in Hz
			SET_SAMPLE_RATE, // in samples per seconds
			GEN_SYMBOL, // in µs
			REPEAT,
			STOP,
		} action;

		float value;
	};

	std::vector<command> _cmds;
	uint64_t _remaining_samples;
	size_t _cur_index;
	size_t _repeat_count;

	void modulate(std::complex<short> *samples, size_t len);

protected:
	void resetState();
	void setFrequency(float freq);
	void setPhase(float phase);
	void setAmp(float amp);
	void setCarrierFrequency(float freq);
	void setSampleRate(float sample_rate);
	void genSymbol(float len_us);
	void endMessage(size_t repeat_count);

public:
	enum ModulationType {
		UNKNOWN	= 0,
		OOK	= 1,
		FSK	= 2,
		PSK	= 4,
		DPSK	= 8,
		QAM	= 16
	};

	Modulation();

	virtual std::string toString() const = 0;

	virtual float centralFrequency() const = 0;
	virtual float channelWidth() const = 0;

	virtual bool chechPhyParameters(const phy_parameters_t &phy) const;

	virtual bool prepareMessage(const Message &m, const phy_parameters_t &phy,
				float amp) = 0;
	void getNextSamples(std::complex<short> *samples, size_t *len);
};

#endif // MODULATION_H
