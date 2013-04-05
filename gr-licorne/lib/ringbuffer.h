#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <atomic>

#include <stddef.h>
#include <memory>
#include <list>

#include "stdio.h"

typedef uint64_t RingBufferMarkerId;

template <class Sample, class Marker>
class RingBuffer
{
protected:
	size_t _ring_length;
	std::auto_ptr<Sample> _ring;
	std::atomic<size_t> _cur;
	std::atomic<size_t> _size;

	/// All this needs locking (rw-lock)
	struct MarkerInternal
	{
		RingBufferMarkerId id;
		size_t pos;
		Marker m;
	};
	uint64_t _marker_id;
	std::list<MarkerInternal> _markers;

public:
	RingBuffer(size_t size) : _ring_length(size),
		_ring(new Sample[size * sizeof(Sample)]), _cur(0),
		_marker_id(1), _size(0)
	{

	}

	size_t ringLength() const { return _ring_length; }
	size_t currentSize() const { return _size; }

	/** request the address of the N'th Sample before the end
	 *
	 * WARNING: This function returns the number of bytes readable that
	* can be lower than $length (due to the wrapping). In this case,
	* restartPos will contain the position at which you should read.
	 */
	size_t requestReadLastN(size_t n, size_t *startPos, size_t *restartPos, Sample **samples)
	{
		int64_t wantedPos = (_ring_length + _cur.load() - n) % _ring_length;
		size_t packetSize = n;

		/* check there are at least n elements */
		if (_size.load() < n) {
			packetSize = 0;
			*startPos = wantedPos;
			*restartPos = wantedPos;
			*samples = NULL;
		} else {
			/* check that we won't be wrapping around */
			if (wantedPos + n >= _ring_length)
				packetSize = (_ring_length - wantedPos);

			*startPos = wantedPos;
			*restartPos = 0; // restart pos will always be 0
			*samples = (_ring.get() + wantedPos);
		}

		return packetSize;
	}

	/** request some space in the ringbuffer for reading new data
	 *
	 * WARNING: This function returns the number of bytes readable that
	 * can be lower than $length (due to the wrapping).
	 */
	size_t requestRead(size_t pos, size_t length, Sample **samples)
	{
		size_t packetSize = length;

		/* check that we won't be wrapping around  */
		if (pos + length >= _ring_length)
			packetSize = (_ring_length - pos);

		*samples = (_ring.get() + pos);

		return packetSize;
	}

	/** request some space in the ringbuffer for writing new data
	 *
	 * WARNING: This function returns the number of bytes allocated that
	 * can be lower than $length (due to the wrapping).
	 */
	size_t requestWrite(size_t length, Sample **samples)
	{
		// reserve the space in the ring buffer
		size_t cur = _cur.load(), oldCur = cur;
		size_t packetSize = requestRead(cur, length, samples);
		cur = (cur + packetSize) % _ring_length;
		_cur.store(cur);

		size_t size = _size.load();
		size = (size + packetSize) % _ring_length;
		_size.store(size);

		/* delete the markers that have been overridden */
		typename std::list<MarkerInternal>::iterator it =_markers.begin();
		while (it != _markers.end() && (*it).pos >= oldCur && (*it).pos < cur)
			it = _markers.erase(it);

		return packetSize;
	}

	/** please try not to use this function unless necessary as it involves
	 * doing a deep copy of the input buffer. Use requestWrite instead.
	 *
	 * It returns the location of the data that has been copied
	 */
	size_t addSamples(const Sample *samples, size_t length)
	{
		size_t begin = _cur.load();

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
	RingBufferMarkerId addMarkerAt(Marker marker, size_t wantedPos)
	{
		wantedPos = (_ring_length + wantedPos) % _ring_length;
		MarkerInternal m = {_marker_id++, wantedPos, marker};

		typename std::list<MarkerInternal>::reverse_iterator rit;
		for (rit =_markers.rbegin(); rit!=_markers.rend(); ++rit) {
			if ((*rit).pos < wantedPos) {
				_markers.insert(rit.base(), m);
				goto exit;
			}
		}

		_markers.push_front(m);

exit:
		return m.id;
	}

	/// This function adds a marker $offset samples before the current position
	RingBufferMarkerId addMarker(Marker marker, size_t offset)
	{
		size_t wantedPos = (_ring_length + _cur.load() - offset) % _ring_length;
		return addMarkerAt(marker, wantedPos);
	}

	bool getMarker(uint64_t id, Marker &marker, size_t &pos)
	{
		typename std::list<MarkerInternal>::reverse_iterator rit;
		for (rit =_markers.rbegin(); rit!=_markers.rend(); ++rit) {
			if ((*rit).id == id) {
				marker = (*rit).m;
				pos = (*rit).pos;
				return true;
			}
		}
		return false;
	}

	bool findMarker(size_t pos, Marker &marker, size_t &markerPos)
	{
		typename std::list<MarkerInternal>::reverse_iterator rit = _markers.rbegin();

		/* go back in the markers until the marker's pos is
		 * higher than the wanted pos
		 */
		while (rit != _markers.rend() && (*rit).pos < pos)
			++rit;

		while (rit != _markers.rend() && (*rit).pos > pos)
			++rit;

		if (rit == _markers.rend())
			return false;
		else {
			marker = (*rit).m;
			markerPos = (*rit).pos;
			return true;
		}
	}

	void debugContent()
	{
		float * r = _ring.get();
		typename std::list<MarkerInternal>::iterator it;
		for (size_t i = 0; i < _ring_length; i++)
			std::cerr << " " << (float)r[i];
		std::cerr << std::endl;
	}

	void debugMarkers()
	{
		typename std::list<MarkerInternal>::iterator it;
		for (it=_markers.begin(); it != _markers.end(); ++it)
			fprintf(stderr, "(%llu, %u), ", (*it).id, (*it).pos);
		fprintf(stderr, "\n");
	}
};

#endif // RINGBUFFER_H
