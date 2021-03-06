#include "constellation.h"

#include <sstream>
#include <limits.h>
#include <algorithm>

ConstellationPoint::ConstellationPoint(float pos, float posMin,
			   float posMax, float proba) :
			   pos(pos), posMin(posMin), posMax(posMax),
			   proba(proba) {
	//std::cout << "Add point at pos " << pos << ", prio = " << prio << std::endl;
}

std::string ConstellationPoint::toString() {
	std::stringstream ss;

	ss << "[ " << pos << "(-" << pos - posMin << ", +" << posMax - pos
	   << "), p=" << proba << " ]";

	return ss.str();
}

bool ConstellationPoint::operator > (const ConstellationPoint &c) const
{
	return this->proba > c.proba;
}

bool ConstellationPoint::operator < (const ConstellationPoint &c) const
{
	return this->proba < c.proba;
}


Constellation::Constellation() : hist_pos(1000, 0), hist_neg(1000, 0),
				 pos_min(INT32_MAX), pos_max(INT32_MIN),
				 pos_count(0)
{

}

size_t Constellation::getHistAt(int32_t position) const
{
	if (position >= 0) {
		if ((size_t)position < hist_pos.size())
			return hist_pos[position];
		else
			return 0;
	} else {
		position = -position - 1;
		if ((size_t)position < hist_neg.size())
			return hist_neg[position];
		else
			return 0;
	}
}

int32_t Constellation::histMinVal() const
{
	return pos_min;
}

int32_t Constellation::histMaxVal() const
{
	return pos_max;
}

int32_t Constellation::histValCount() const
{
	return pos_count;
}

void Constellation::addPoint(int32_t position)
{
	if (position >= 0) {
		if (position < pos_min)
			pos_min = position;
		if (position >= pos_max)
			pos_max = position + 1;

		if ((int32_t)hist_pos.size() < position)
			hist_pos.resize(position * 2, 0);

		hist_pos[position] = hist_pos[position] + 1;
	} else {
		if (position < pos_min)
			pos_min = position;
		if (position >= pos_max)
			pos_max = position + 1;

		position = -position - 1;
		if ((int32_t)hist_neg.size() < position)
			hist_neg.resize(position * 2, 0);

		hist_neg[position] = hist_neg[position] + 1;
	}

	pos_count++;
}

void Constellation::createClusters(float sampleDistMax, float ignoreProbaUnder)
{
	size_t sampleDistMaxRealAbs = sampleDistMax * pos_min;
	size_t sampleDistMaxRealRange = sampleDistMax * (pos_max - pos_min);
	size_t sampleDistMaxReal = sampleDistMaxRealAbs > sampleDistMaxRealRange ? sampleDistMaxRealAbs : sampleDistMaxRealRange;
	size_t cluster_pos_avr = 0, cluster_sum = 0;
	int32_t cluster_start = -1, cluster_end = -1;
	size_t val_at_zero_count = 0;

	for (int32_t i = pos_min; i < pos_max; i++) {
		size_t count = getHistAt(i);

		if (count * 1.0 / pos_count < ignoreProbaUnder)
			count = 0;

		if (count > 0) {
			cluster_pos_avr += i * count;
			cluster_sum += count;
			if (cluster_start < 0)
				cluster_start = i;
			cluster_end = i + 1;
			val_at_zero_count = 0;
		} else if (cluster_sum > 0 && val_at_zero_count == sampleDistMaxReal) {
			prio.push_back(ConstellationPoint(1.0 * cluster_pos_avr / cluster_sum,
							  cluster_start, cluster_end,
							  1.0 * cluster_sum / pos_count));
			cluster_pos_avr = 0;
			cluster_sum = 0;
			cluster_start = -1;
			cluster_end = -1;
		} else
			val_at_zero_count++;
	}

	if (cluster_sum > 0) {
		prio.push_back(ConstellationPoint(1.0 * cluster_pos_avr / cluster_sum,
						  cluster_start, cluster_end,
						  1.0 * cluster_sum / pos_count));
	}
}

bool Constellation::clusterize(float sampleDistMax, float ignoreProbaUnder)
{
	createClusters(sampleDistMax, ignoreProbaUnder);

	std::sort(prio.begin(), prio.end(), std::greater<ConstellationPoint>());

	return true;
}

ConstellationPoint Constellation::findGreatestCommonDivisor()
{
	std::vector<ConstellationPoint> clusters;
	Constellation cStride;

	createClusters(0.0, 0.0);

	clusters = prio;

	// sort from bigger to smaller
	std::sort(clusters.begin(), clusters.end(), [](ConstellationPoint a, ConstellationPoint b) {
		return a.pos > b.pos;
	});

	for (size_t i = 1; i < clusters.size(); i++) {
		float diff = clusters[i - 1].pos - clusters[i].pos;
		cStride.addPoint(diff);
	}

	cStride.clusterize(0, 0);

	//std::cout << cStride.histogram() << std::endl;

	int i = 0;
	//std::cout << std::endl;
	ConstellationPoint cpFinal = { 0.0, 0.0 }, cp;
	do {
		cp = cStride.mostProbabilisticPoint(i);
		if (cp.proba > 0.0) {
			if (cpFinal.proba == 0) {
				cpFinal.proba = cp.proba;
				cpFinal.pos = cp.pos;
			} else if (cp.pos > cpFinal.pos){
				float factor = cpFinal.proba / cp.proba;
				float val = cp.pos / roundf(cp.pos / cpFinal.pos);
				cpFinal.pos = ((cpFinal.pos * factor) + val) / (factor + 1);
				cpFinal.proba += cp.proba;
			}
			std::cout << cp.toString() << " ";
		}
		i++;
	} while (cp.proba > 0.0);
	std::cout << std::endl; // << std::endl;

	return cpFinal;
}

ConstellationPoint Constellation::mostProbabilisticPoint(size_t n) const
{
	if (n < prio.size())
		return prio[n];
	else
		return ConstellationPoint();
}

std::string Constellation::histogram() const
{
	std::stringstream ss;
	float last = 1.0;

	ss << "value, proba (n=" << pos_count << ")" << std::endl;
	for (int32_t i = pos_min; i < pos_max; i++) {
		float val = getHistAt(i) * 1.0 / pos_count;
		if (val > 0)
			ss << i << "," << val << std::endl;
		else if (last > 0)
			ss << "[...]" << std::endl;

		last = val;
	}
	ss << std::endl;

	return ss.str();
}
