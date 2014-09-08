#include <iostream>

#include "cognode.h"
#include "random.h"
#include <unistd.h>
#include <time.h>
#include <math.h>

uint64_t scenario1_run(uint64_t hoppingPeriod, uint64_t beaconningPeriod,
		       size_t beaconCount, float senseBW)
{
	float band1 = 325e6 + (rand_cmwc() % 2650) * 1e6;
	float band2 = 325e6 + (rand_cmwc() % 2650) * 1e6;

	/*std::cout << "Band1: " << band1 / 1e6;
	std::cout << ", Band2: " << band2 / 1e6 << std::endl;*/

	HoppingPattern hp1(1000e3, Band(300e6, 3e9), 25e6, HoppingPattern::LINEAR, 1e3);
	hp1.addAvailableEntry(Band(band1, band1+25e6), 0.0, 0.4);
	hp1.addAvailableEntry(Band(band2, band2+25e6), 0.5, 0.4);
	CogNode n1(42, 0, 0, beaconningPeriod, hp1, beaconCount);

	HoppingPattern hp2(60e6, Band(300e6, 3e9), senseBW, HoppingPattern::RANDOM, hoppingPeriod);
	CogNode n2(43, 0, 0, 10e3, hp2, beaconCount);

	CogNode *nodes[] = { &n1, &n2 };
	size_t nodesCount = sizeof(nodes) / sizeof(CogNode *);

	while (true) {
		uint64_t next = -1;

		for (size_t i = 0; i < nodesCount; i++) {
			uint64_t nodeNext = nodes[i]->usUntilNextOperation();
			if (nodeNext < next)
				next = nodeNext;
		}

		//std::cout << "next = " << next << std::endl;

		// FIXME? If a node jumps to a frequency, it won't be able to
		// receive a message until the next tick change. That's pretty
		// much what we get in real life though :s
		for (size_t i = 0; i < nodesCount; i++) {
			nodes[i]->addTicks(next);
			if (n2.hasNeighbour(42)) {
				return n2.ticksSinceStart();
			}
		}
	}

	//std::cout << std::endl;
}

void scenario1_periods_vs_avgtime() {
	std::cout << "Hopping Period (µs), Beaconning Period (µs), average time (s), std, min, max" << std::endl;

	for (size_t hp = 0; hp < 5; hp ++) {
		for (size_t bp = 0; bp < 5; bp ++) {
			size_t hp_us = pow(10, hp) * 1e3;
			size_t bp_us = pow(10, bp) * 1e3;

			uint64_t sum = 0, sum_sq = 0, count = 10000, min = -1, max = 0;
			for (size_t i = 0; i < count; i++) {
				uint64_t res = scenario1_run(hp_us, bp_us, 1, 25e6);
				sum_sq += (res * res);
				sum += res;
				if (res < min)
					min = res;
				if (res > max)
					max = res;
			}
			std::cout << hp_us << ", " << bp_us << ", "
				  << sum / count / 1.0e6 << ", "
				  << sqrtf(sum_sq - (sum * sum)) << ", "
				  << min << ", " << max
				  << std::endl;
		}
	}
}

void scenario1_beaconCount_vs_time() {
	std::cout << "Beacon Count, Hopping Period (µs), Beaconning Period (µs), average time (s), std, min (s), max (s)" << std::endl;

	size_t hp_us = pow(10, 1) * 1e3;
	size_t bp_us = pow(10, 1) * 1e3;
	for (size_t bc = 1; bc <= 10; bc++) {
		uint64_t sum = 0, sum_sq = 0, count = 100000, min = -1, max = 0;
		for (size_t i = 0; i < count; i++) {
			uint64_t res = scenario1_run(hp_us, bp_us, bc, 25e6);
			sum_sq += (res * res);
			sum += res;
			if (res < min)
				min = res;
			if (res > max)
				max = res;
		}
		std::cout << bc << ", " << hp_us << ", " << bp_us << ", "
			  << sum / count / 1.0e6 << ", "
			  << sqrtf(sum_sq - (sum * sum)) << ", "
			  << min / 1.0e6 << ", " << max / 1.0e6
			  << std::endl;
	}
}

void scenario1_bandwidth_vs_time() {
	std::cout << "Bandwidth, Beacon Count, Hopping Period (µs), Beaconning Period (µs), average time (s), std, min (s), max (s)" << std::endl;

	size_t hp_us = pow(10, 1) * 1e3;
	size_t bp_us = pow(10, 1) * 1e3;
	size_t bc = 2;

	for (float bandwidth = 4e6; bandwidth <= 100e6; bandwidth+=2e6) {
		uint64_t sum = 0, sum_sq = 0, count = 1000000, min = -1, max = 0;
		for (size_t i = 0; i < count; i++) {
			uint64_t res = scenario1_run(hp_us, bp_us, bc, bandwidth);
			sum_sq += (res * res);
			sum += res;
			if (res < min)
				min = res;
			if (res > max)
				max = res;
		}
		std::cout << bandwidth << ", " << bc << ", " << hp_us << ", "
			  << bp_us << ", " << sum / count / 1.0e6 << ", "
			  << sqrtf(sum_sq - (sum * sum)) << ", "
			  << min / 1.0e6 << ", " << max / 1.0e6
			  << std::endl;
	}
}

int main(int argc, char **argv)
{
	init_rand(time(NULL));

	//scenario1_periods_vs_avgtime();
	//scenario1_beaconCount_vs_time();
	scenario1_bandwidth_vs_time();

	return 0;
}

