#ifndef SAMPLESRINGBUFFER_H
#define SAMPLESRINGBUFFER_H

#include <atomic>

#include <stddef.h>
#include <memory>
#include <list>

#include "stdio.h"

typedef uint64_t SamplesRingBufferMarkerId;

template <class Sample, class Marker>
class SamplesRingBuffer
{
	size_t _ring_length;
	std::auto_ptr<Sample> _ring;
	std::atomic<size_t> _cur;

	struct MarkerInternal
	{
		SamplesRingBufferMarkerId id;
		size_t pos;
		Marker m;
	};
	uint64_t _marker_id;
	std::list<MarkerInternal> _markers;

public:
	SamplesRingBuffer(size_t size) : _ring_length(size),
		_ring(new Sample[size * sizeof(Sample)]), _cur(0), _marker_id(1)
	{

	}

	size_t stringLength() const { return _ring_length; }

	/** request the address of the N'th Sample before the end
	 *
	 * WARNING: This function returns the number of bytes readable that
	 * can be lower than $length (due to the wrapping).
	 */
	size_t requestReadLastN(size_t n, Sample **samples)
	{
		int64_t wantedPos = (_ring_length + _cur.load() - n) % _ring_length;
		size_t packetSize = n;

		/* check that we won't be wrapping around  */
		if (wantedPos + n >= _ring_length)
			packetSize = (_ring_length - wantedPos);

		*samples = (_ring.get() + wantedPos);

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
		size_t cur = _cur.load();
		size_t packetSize = requestRead(cur, length, samples);
		cur = (cur + packetSize) % _ring_length;

		/* delete the markers that have been overridden */
		typename std::list<MarkerInternal>::iterator it =_markers.begin();
		while (it != _markers.end() && (*it).pos < cur)
			it = _markers.erase(it);

		return packetSize;
	}

	/** please try not to use this function unless necessary as it involves
	 * doing a deep copy of the input buffer. Use requestWrite instead.
	 *
	 * It returns the location of the data that has been copied
	 */
	size_t addSamples(Sample *samples, size_t length)
	{
		size_t begin = _cur.load();;

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

	/// This function adds a marker $offset samples before the current position
	SamplesRingBufferMarkerId addMarker(Marker marker, size_t offset)
	{
		int64_t wantedPos = (_ring_length + _cur.load() - offset) % _ring_length;
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

#endif // SAMPLESRINGBUFFER_H
