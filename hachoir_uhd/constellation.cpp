#include "constellation.h"

#include <limits.h>
#include <algorithm>

Constellation::Constellation() : hist_pos(1000, 0), hist_neg(1000, 0),
				 pos_min(INT32_MAX), pos_max(INT32_MIN),
				 pos_count(0)
{

}

size_t Constellation::getHistAt(int32_t position)
{
	if (position >= 0) {
		return hist_pos[position];
	} else {
		if (position < pos_min)
			pos_min = position;
		if (position > pos_max)
			pos_max = position;

		return hist_neg[-position - 1];
	}
}

void Constellation::addPoint(int32_t position)
{
	if (position >= 0) {
		if (position < pos_min)
			pos_min = position;
		if (position > pos_max)
			pos_max = position;

		if ((int32_t)hist_pos.size() < position)
			hist_pos.resize(position * 2, 0);

		hist_pos[position] = hist_pos[position] + 1;
	} else {
		if (position < pos_min)
			pos_min = position;
		if (position > pos_max)
			pos_max = position;

		position = -position - 1;
		if ((int32_t)hist_neg.size() < position)
			hist_neg.resize(position * 2, 0);

		hist_neg[position] = hist_neg[position] + 1;
	}

	pos_count++;
}

bool Constellation::clusterize(float sampleDistMax)
{
	size_t sampleDistMaxReal = sampleDistMax * (pos_max - pos_min);
	size_t cluster_pos_avr = 0, cluster_sum = 0;
	size_t val_at_zero_count = 0;

	for (int32_t i = pos_min; i < pos_max; i++) {
		size_t count = getHistAt(i);
		if (count > 0) {
			cluster_pos_avr += i * count;
			cluster_sum += count;
			val_at_zero_count = 0;
		} else if (cluster_sum > 0 && val_at_zero_count == sampleDistMaxReal) {
			prio.push_back(ConstellationPoint(1.0 * cluster_pos_avr / cluster_sum,
							  1.0 * cluster_sum / pos_count));
			cluster_pos_avr = 0;
			cluster_sum = 0;
		} else
			val_at_zero_count++;
	}

	if (cluster_sum > 0)
		prio.push_back(ConstellationPoint(1.0 * cluster_pos_avr / cluster_sum,
						  1.0 * cluster_sum / pos_count));

	std::sort(prio.begin(), prio.end(), std::greater<ConstellationPoint>());

	return true;
}

ConstellationPoint Constellation::mostProbabilisticPoint(size_t n) const
{
	if (n < prio.size())
		return prio[n];
	else
		return ConstellationPoint();
}
