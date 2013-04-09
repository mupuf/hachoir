#ifndef POWERSPECTRUM_H
#define POWERSPECTRUM_H

#include <QObject>
#include <QSharedPointer>

class PowerSpectrum : public QObject
{
	Q_OBJECT

	qreal _freqStart;
	qreal _freqEnd;
	qreal _pwrMin;
	qreal _pwrMax;
	qreal _timeNs;

	size_t _steps;
	QSharedPointer<char> _pwr;
public:
	PowerSpectrum(qreal freqStart, qreal freqEnd, uint64_t timeNs,
		      uint16_t steps, const char *data);

	void reset(qreal freqStart, qreal freqEnd, uint64_t timeNs,
	      uint16_t steps, const char *data);

	Q_INVOKABLE qreal frequencyMin() const { return _freqStart; }
	Q_INVOKABLE qreal frequencyMax() const { return _freqEnd; }
	Q_INVOKABLE qreal powerMin() const { return _pwrMin; }
	Q_INVOKABLE qreal powerMax() const { return _pwrMax; }

	Q_INVOKABLE qreal timeNs() const { return _timeNs; }

	Q_INVOKABLE int samplesCount() const { return _steps; }
	Q_INVOKABLE qreal sampleFrequency(int i) const
	{
		return _freqStart + i * (_freqEnd-_freqStart) / _steps;
	}
	Q_INVOKABLE qreal sampleDbm(int i) const  { return _pwr.data()[i]; }
};

#endif // POWERSPECTRUM_H
