#include "emissionruntime.h"

#include <iostream>
#include <string.h>

#include "modulations/modulation.h"

EmissionRunTime::EmissionRunTime(size_t messageCountMax, size_t block_size,
				 phy_parameters_t phy, float amp) :
	_thread(worker, this), _phy(phy), _amp(amp), _heap(messageCountMax),
	_block_size(block_size), _cacheBlocks(0), _quit(false), _nextBuf(NULL)
{
	samples[0] = new std::complex<short>[_block_size];
	samples[1] = new std::complex<short>[_block_size];
}

bool EmissionRunTime::addMessage(const Message& msg)
{
	return _heap.addMessage(msg);
}

// this thread will be terminated when the object gets destroyed
/*void EmissionRunTime::worker(EmissionRunTime *_this)
{
	std::vector<Message> _active;
	std::complex<short> *msgSamples = new std::complex<short>[_this->_block_size];
	std::complex<short> *nullSamples = new std::complex<short>[_this->_block_size];
	std::complex<short> *nextBuf = _this->samples[0];

	memset(nullSamples, 0, sizeof(std::complex<short>) * _this->_block_size);

	while (1) {
		// add messages to the "active" list
		_this->_heap.lock();
		for (size_t i = 0; i < _this->_heap.size(); i++) {
			if (_this->_heap.at(i).modulation()->prepareMessage(_this->_heap.at(i),
									    _this->_phy,
									    _this->_amp))
			{
				Message m = _this->_heap.extract(i);
				_active.push_back(m);
			}
		}
		_this->_heap.unlock();

		for (size_t i = 0; i < _active.size(); i++) {
			std::complex<short> *dst = msgSamples;
			size_t len = _this->_block_size;

			if (i == 0)
				dst = nextBuf;

			_active[i].modulation()->getNextSamples(dst, &len);

			// we didn't fill the buffer, fill the rest with 0's
			if (i == 0 && len != _this->_block_size) {
				memset(dst + len, 0,
				       (_this->_block_size - len) * sizeof(std::complex<short>));
			} else if (i > 0) {
				// mix all the signals
				for (size_t e = 0; e < len; e++) {
					nextBuf[e] += dst[e];
				}
			}
		}

		// We are done producing the buffer, expose it!
		_this->_cv_m.lock();
		if (_active.size() > 0) {
			_this->_nextBuf = nextBuf;

			// select another buffer
			if (nextBuf == _this->samples[0])
				nextBuf = _this->samples[1];
			else
				nextBuf = _this->samples[0];
		} else {
			std::cerr << "Cannot send any message!" << std::endl;
			_this->_nextBuf = nullSamples;
		}
		_this->_cv_m.unlock();

		// wait for the consummer to use our buffer
		std::unique_lock<std::mutex> lk(_this->_cv_m);
		_this->_cv.wait(lk);

	}
}

EmissionRunTime::Command
EmissionRunTime::next_block(std::complex<short> **samples,
			    size_t *len, phy_parameters_t &phy)
{
	EmissionRunTime::Command ret = OK;

	// Check the requested size is equal to the block size
	if (*len != _block_size)
		return EmissionRunTime::ERROR;

	// Ensure the synchronization with the worker
	_cv_m.lock();

	if (!_nextBuf) {
		*samples = NULL;
		*len = 0;
		std::cerr << "Samples weren't ready. The computer is too slow!" << std::endl;
		ret = EmissionRunTime::ERROR;
	} else {
		*samples = _nextBuf;
		_nextBuf = NULL;
	}

	_cv_m.unlock();

	// Wake up the worker
	_cv.notify_all();

	return ret;
}*/

void EmissionRunTime::worker(EmissionRunTime *_this)
{
}

EmissionRunTime::Command
EmissionRunTime::next_block(std::complex<short> *samples,
			    size_t len, phy_parameters_t &phy)
{
	memset(samples, 0, sizeof(std::complex<short>) * len);

	if (!cur_mod.get() && _heap.size() > 0) {
		Message m = _heap.extract(0);
		cur_mod = m.modulation();
		cur_mod->prepareMessage(m, phy, _amp);
		// change phy if needed
	}

	if (!cur_mod.get())
		return OK;

	size_t msg_len = len;
	cur_mod->getNextSamples(samples, &msg_len);

	if (msg_len < len)
		cur_mod.reset();

	return OK;
}

