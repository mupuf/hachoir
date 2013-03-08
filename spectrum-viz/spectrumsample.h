#ifndef SPECTRUMSAMPLE_H
#define SPECTRUMSAMPLE_H

#include <QtGlobal>

class SpectrumSample
{
private:
	qreal _freq;
	qreal _dbm;

public:
	SpectrumSample();
	explicit SpectrumSample(qreal freq, qreal dbm);

	qreal freq() const { return _freq; }
	qreal dBM() const { return _dbm; }

	void setFreq(qreal freq) { _freq = freq; }
	void setDBM(qreal dBM) { _dbm = dBM; }
};

#endif // SPECTRUMSAMPLE_H
