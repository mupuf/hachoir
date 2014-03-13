#ifndef CONSTELLATION_H
#define CONSTELLATION_H

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>


#include <iostream>

struct ConstellationPoint {
	float pos;
	float posMin;
	float posMax;
	// TODO: Standard deviation
	float proba;

	ConstellationPoint(float pos = 0.0, float posMin = 0.0,
			   float posMax = 0.0, float proba = 0.0);

	std::string toString();
	bool operator > (const ConstellationPoint &c) const;
	bool operator < (const ConstellationPoint &c) const;
};

class Constellation
{
	std::vector<ConstellationPoint> prio;
	std::vector<size_t> hist_pos, hist_neg;

	int32_t pos_min, pos_max;
	size_t pos_count;

	size_t getHistAt(int32_t position) const;
public:
	Constellation();

	void addPoint(int32_t position);
	bool clusterize(float sampleDistMax = 0.01, float ignoreValuesUnder = 0.01);
	ConstellationPoint mostProbabilisticPoint(size_t n) const;

	std::string histogram() const;
};

#endif // CONSTELLATION_H
