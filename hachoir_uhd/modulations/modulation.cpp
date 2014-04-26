#include "modulation.h"

#include <iostream>

Modulation::Modulation() :
	_freq(0.0),
	_phase(0.0),
	_amp(0.0),
	_carrier_freq(0.0),
	_sample_rate(0.0),
	_remaining_samples(0),
	_cur_index(0),
	_repeat_count(0)
{
}

void Modulation::resetState()
{
	_remaining_samples = 0.0;
	_cur_index = 0;
	_cmds.clear();
	_repeat_count = 0;
}

void Modulation::setFrequency(float freq)
{
	command cmd = { command::SET_FREQ, freq };
	_cmds.push_back(cmd);
}

void Modulation::setPhase(float phase)
{
	command cmd = { command::SET_PHASE, phase };
	_cmds.push_back(cmd);
}

void Modulation::setAmp(float amp)
{
	command cmd = { command::SET_AMP, amp };
	_cmds.push_back(cmd);
}

void Modulation::setCarrierFrequency(float freq)
{
	command cmd = { command::SET_CARRIER_FREQ, freq };
	_cmds.push_back(cmd);
}

void Modulation::setSampleRate(float sample_rate)
{
	command cmd = { command::SET_SAMPLE_RATE, sample_rate };
	_cmds.push_back(cmd);
}

void Modulation::genSymbol(float len_us)
{
	command cmd = { command::GEN_SYMBOL, len_us };
	_cmds.push_back(cmd);
}

void Modulation::endMessage(size_t repeat_count)
{
	command cmd;

	if (repeat_count > 0) {
		cmd = { command::REPEAT, (float) repeat_count };
		_cmds.push_back(cmd);
	}

	cmd = { command::STOP, 0.0 };
	_cmds.push_back(cmd);
}

bool Modulation::chechPhyParameters(const phy_parameters_t &phy) const
{
	if (centralFrequency() + channelWidth() / 2 > (phy.central_freq + phy.sample_rate / 2.0) ||
	    centralFrequency() - channelWidth() / 2 < (phy.central_freq - phy.sample_rate / 2.0))
		return false;
	return true;
}

void Modulation::getNextSamples(std::complex<short> *samples, size_t *len)
{
	size_t offset = 0;

	while (offset < *len) {
		if (_cur_index >= _cmds.size()) {
			*len = 0;
			return;
		} else if (_remaining_samples == 0) {
			// Execute the selected command
			switch(_cmds[_cur_index].action) {
			case command::SET_FREQ:
				_freq = _cmds[_cur_index].value;
				_cur_index++;
				break;
			case command::SET_PHASE:
				_phase = _cmds[_cur_index].value;
				_cur_index++;
				break;
			case command::SET_AMP:
				_amp = _cmds[_cur_index].value;
				_cur_index++;
				break;
			case command::SET_CARRIER_FREQ:
				_carrier_freq = _cmds[_cur_index].value;
				_cur_index++;
				break;
			case command::SET_SAMPLE_RATE:
				_sample_rate = _cmds[_cur_index].value;
				_cur_index++;
				break;
			case command::GEN_SYMBOL:
				_remaining_samples = ((size_t)_cmds[_cur_index].value) *
						       _sample_rate / 1000000;
				break;
			case command::REPEAT:
				if (_repeat_count < _cmds[_cur_index].value) {
					_cur_index = 0;
					_repeat_count++;
				} else
					_cur_index++;
				break;
			case command::STOP:
				*len = offset;
				return;
			}
		} else {
			// Generate the sample, starting from where we left
			size_t gen_len = _remaining_samples;
			if (offset + gen_len > *len)
				gen_len = *len - offset;

			modulate(samples + offset, gen_len);

			_remaining_samples -= gen_len;
			offset += gen_len;

			if (_remaining_samples == 0)
				_cur_index++;
		}
	}
}

void Modulation::modulate(std::complex<short> *samples, size_t len)
{
	float radian_step = 2 * M_PI * (_carrier_freq - _freq) / _sample_rate;
	short I, Q;
	FILE *f = NULL;

	//f = fopen("samples.csv", "a");

	for (size_t i = 0; i < len; i++) {
		if (_amp != 0.0) {
			float sin, cos;
			sincosf(_phase, &cos, &sin);
			I = _amp * cos;
			Q = _amp * sin;
		} else {
			I = 0;
			Q = 0;
		}
		_phase += radian_step;
		samples[i] = std::complex<short>(I, Q);

		if (f)
			fprintf(f, "%i, %i\n", I, Q);
	}

	if (f)
		fclose(f);

	return;
}
