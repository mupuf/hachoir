#include "spectrumsample.h"

SpectrumSample::SpectrumSample() : _freq(-1), _dbm(-1)
{
}

SpectrumSample::SpectrumSample(qreal freq, qreal dbm) : _freq(freq), _dbm(dbm)
{
}
