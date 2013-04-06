#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <atomic>

#include <stddef.h>
#include <memory>
#include <list>
#include <boost/thread/mutex.hpp>

#include "stdio.h"

typedef uint64_t RingBufferMarkerId;

template <class Sample, class Marker>
class RingBuffer
{
protected:
	size_t _ring_length;
	std::auto_ptr<Sample> _ring;

	std::atomic<uint64_t> _newHead;
	std::atomic<uint64_t> _head;
	std::atomic<uint64_t> _tail;

	/// All this needs locking (rw-lock)
	struct MarkerInternal
	{
		uint64_t pos;
		Marker m;
	};
	boost::mutex _markersMutex;
	uint64_t _marker_id;
	std::list<MarkerInternal> _markers;


	void requestRead_unsafe(uint64_t pos, size_t *length, Sample **samples)
	{
		size_t packetSize = *length;

		/* check that we won't be wrapping around  */
		if ((pos % _ring_length) + packetSize >= _ring_length)
			packetSize = (_ring_length - (pos % _ring_length));

		*samples = (_ring.get() + (pos % _ring_length));
		*length = packetSize;
	}

public:
	RingBuffer(size_t size) : _ring_length(size),
		_ring(new Sample[size * sizeof(Sample)]), _newHead(0), _head(0),
		_tail(0)
	{

	}

	size_t ringLength() const { return _ring_length; }

	size_t size() const
	{
		return _head.load() - _tail.load();
	}

	bool positionExists(uint64_t pos) const
	{
		return pos >= _tail.load() && pos < _head.load();
	}

	void clear()
	{
		_markersMutex.lock();

		_newHead = 0;
		 _head = 0;
		_tail = 0;

		_markers.clear();

		_markersMutex.unlock();
	}

	/** request the address of the N'th Sample before the end
	 *
	 * WARNING: This function returns the number of bytes readable that
	* can be lower than $length (due to the wrapping). In this case,
	* restartPos will contain the position at which you should read.
	 */
	size_t requestReadLastN(size_t n, uint64_t *startPos, uint64_t *restartPos, Sample **samples)
	{
		int64_t wantedPos = _head.load() - n;
		size_t packetSize = n;

		/* check there are at least n elements */
		if (size() < n) {
			packetSize = 0;
			*startPos = wantedPos;
			*restartPos = wantedPos;
			*samples = NULL;
		} else {
			/* check that we won't be wrapping around */
			if ((wantedPos % _ring_length) + n >= _ring_length)
				packetSize = (_ring_length - (wantedPos % _ring_length));

			*startPos = wantedPos;
			*restartPos = wantedPos + packetSize;
			*samples = (_ring.get() + (wantedPos % _ring_length));
		}

		return packetSize;
	}

	/** request reading data from the ringbuffer
	 *
	 * WARNING: This function returns the number of bytes readable that
	 * can be lower than $length (due to the wrapping).
	 */
	bool requestRead(uint64_t pos, size_t *length, Sample **samples)
	{
		size_t packetSize = *length;

		if (!positionExists(pos)) {
			*length = 0;
			return false;
		}

		requestRead_unsafe(pos, length, samples);
		return true;
	}

	/** request some space in the ringbuffer for writing new data
	 *
	 * Do not forget to validate when you are done adding the markers!
	 *
	 * WARNING: This function returns the number of bytes allocated that
	 * can be lower than $length (due to the wrapping).
	 */
	size_t requestWrite(size_t length, Sample **samples)
	{
		uint64_t tail = _tail.load();
		uint64_t newHead = _newHead.load();
		size_t packetSize = length;
		requestRead_unsafe(newHead, &packetSize, samples);
		uint64_t wantedNewHead = newHead + packetSize;

		if ((_newHead.load() - _tail.load()) + packetSize >= _ring_length) {
			// reserve the space in the ring buffer
			uint64_t wantedTail = wantedNewHead - _ring_length;
			if (wantedTail > _head.load())
				_head.store(wantedTail);
			_tail.store(wantedTail);

			/* delete the markers that have been overridden */
			typename std::list<MarkerInternal>::iterator it =_markers.begin();
			while (it != _markers.end() && (*it).pos >= tail && (*it).pos < wantedTail)
				it = _markers.erase(it);
		}

		_newHead.store(wantedNewHead);

		return packetSize;
	}

	/// call this function when you are done adding the new data and markers
	void validateWrite()
	{
		_head.store(_newHead.load());
	}

	/** please try not to use this function unless necessary as it involves
	 * doing a deep copy of the input buffer. Use requestWrite instead.
	 *
	 * Do not forget to validate when you are done adding the markers!
	 *
	 * It returns the location of the data that has been copied
	 */
	uint64_t addSamples(const Sample *samples, size_t length)
	{
		uint64_t begin = _head.load();

		while (length > 0)
		{
			Sample *r;
			size_t packetSize = requestWrite(length, &r);

			memcpy(r, samples, packetSize * sizeof(Sample));

			// prepare for the next loop
			samples += packetSize;
			length -= packetSize;
		}

		return begin;
	}

	/// This function adds a marker at the specified location
	void addMarker(Marker marker, uint64_t wantedPos)
	{
		_markersMutex.lock();

		MarkerInternal m = {wantedPos, marker};

		typename std::list<MarkerInternal>::reverse_iterator rit;
		for (rit =_markers.rbegin(); rit!=_markers.rend(); ++rit) {
			if ((*rit).pos == wantedPos) {
				(*rit).m = marker;
			} else if ((*rit).pos < wantedPos) {
				_markers.insert(rit.base(), m);
				goto exit;
			}
		}

		_markers.push_front(m);

exit:
		_markersMutex.unlock();
	}

	bool getMarker(uint64_t markerPos, Marker *marker)
	{
		typename std::list<MarkerInternal>::reverse_iterator rit;
		bool ret = false;

		_markersMutex.lock();
		for (rit =_markers.rbegin(); rit!=_markers.rend(); ++rit) {
			if ((*rit).pos == markerPos) {
				*marker = (*rit).m;
				ret = true;
				break;
			}
		}
		_markersMutex.unlock();

		return ret;
	}

	bool findMarker(uint64_t pos, Marker *marker, uint64_t *markerPos)
	{
		bool ret = false;

		if (!positionExists(pos))
			return false;

		_markersMutex.lock();
		typename std::list<MarkerInternal>::reverse_iterator rit = _markers.rbegin();

		while (rit != _markers.rend() && (*rit).pos > pos)
			++rit;

		if (rit != _markers.rend()) {
			*marker = (*rit).m;
			*markerPos = (*rit).pos;
			ret = true;
		}

		_markersMutex.unlock();

		return ret;
	}

	void debugContent()
	{
		float * r = _ring.get();
		typename std::list<MarkerInternal>::iterator it;
		std::cerr << "len = " << size() << ": ";
		for (size_t i = 0; i < _ring_length; i++)
			std::cerr << " " << (float)r[i];
		std::cerr << std::endl;
	}

	void debugMarkers()
	{
		typename std::list<MarkerInternal>::iterator it;
		_markersMutex.lock();
		for (it=_markers.begin(); it != _markers.end(); ++it)
			fprintf(stderr, "(%llu, %u), ", (*it).id, (*it).pos);
		fprintf(stderr, "\n");
		_markersMutex.unlock();
	}
};

#endif // RINGBUFFER_H
