#include "psk.h"
#include "utils/constellation.h"

#include <fstream>

PSK::PSK()
{
}

uint8_t PSK::likeliness(const Burst &burst)
{
	float ph_last;
	Constellation cTS, cPhaseDiff;
	ConstellationPoint cpTS;

	_diff_phase.reserve(burst.len());
	_phase.reserve(burst.len());

	//FILE *f = fopen("Diff_phase", "w");

	/* compute the frequency offset */
	for (size_t i = 0; i < burst.subBursts[0].len; i++) {
		float phase = arg(std::complex<float>(burst.samples[i].real(),
						      burst.samples[i].imag()));

		_phase.push_back(ph_last);

		if (i > 1) {
			float dph = phase - ph_last;
			if (dph > M_PI)
				dph = phase - (ph_last + 2 * M_PI);
			else if (dph < -M_PI)
				dph = (phase + 2 * M_PI) - ph_last;

			//fprintf(f, "%f\n", dph);
			_diff_phase.push_back(dph);

			cPhaseDiff.addPoint(dph * 1000);
		}

		ph_last = phase;
	}

	//fclose(f);

	std::ofstream outFile("histogram.csv");
	outFile << cPhaseDiff.histogram() << std::endl;
	outFile.close();

	cPhaseDiff.clusterize(0);

	_phaseDiff = 0.62; //cPhaseDiff.mostProbabilisticPoint(0).pos / 1000.0;

	/* compute the symbol time */
	size_t last_crossing = 0;
	for (size_t i = 0; i < _diff_phase.size(); i++) {
		float diff = _phaseDiff - _diff_phase[i];
		if (diff > M_PI_4 || diff < -M_PI_4) {
			cTS.addPoint(i - last_crossing);
			last_crossing = i;
		}
	}

	// TODO
	_bps = 2;

	cTS.clusterize(1.0);
	cpTS = cTS.mostProbabilisticPoint(0);
	_TS = roundf(cpTS.pos);

	//std::cout << cTS.histogram() << std::endl;

	float freq_offset = (_phaseDiff * burst.phy().sample_rate) / (2 * M_PI); ;

	char phy_params[150];
	snprintf(phy_params, sizeof(phy_params), "%u-PSK: freq offset = %.3f kHz, symbol time = %.1f [%.1f, %.1f]",
		_bps, freq_offset / 1000.0, _TS, cpTS.posMin, cpTS.posMax);
	_phy_params = phy_params;

	return 255;
}

std::vector<Message> PSK::demod(const Burst &burst)
{
	std::vector<Message> msg;
	Message m;

	//size_t last_crossing = 0;
	for (size_t i = 0; i < _diff_phase.size(); i++) {
		float diff = _phaseDiff - _diff_phase[i];
		if (diff > M_PI_4) {
			m.addBit(true);
		} else if (diff < -M_PI_4) {
			m.addBit(false);
		}
	}

	msg.push_back(m);

	return msg;
}

std::string PSK::modulationString() const
{
	return _phy_params;
}
