#ifndef EMISSIONRUNTIME_H
#define EMISSIONRUNTIME_H

#include <condition_variable>
#include <complex>
#include <thread>
#include <mutex>

#include "phy_parameters.h"
#include "messageheap.h"

class EmissionRunTime
{
	std::thread _thread;

	phy_parameters_t _phy;
	float _amp;

	MessageHeap _heap;
	size_t _block_size;

	std::condition_variable _cv;
	std::mutex _cv_m;
	size_t _cacheBlocks;
	bool _quit;

	std::complex<short> *samples[2], *_nextBuf;


	static void worker(EmissionRunTime *_this);

	std::shared_ptr<Modulation> cur_mod;

public:
	enum Command {
		OK = 1,
		ERROR = 2,
		END_OF_BLOCK = 4,
		CHANGE_PHY = 8
	};

	EmissionRunTime(size_t messageCountMax, size_t block_size,
			phy_parameters_t phy, float amp);

	bool addMessage(const Message& msg);

	Command next_block(std::complex<short> *samples, size_t len, phy_parameters_t &phy);
};

#endif // EMISSIONRUNTIME_H
