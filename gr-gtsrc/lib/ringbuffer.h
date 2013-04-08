/**
 * \file      ringbuffer.h
 * \author    MuPuF (Martin Peres <martin.peres@labri.fr>)
 * \version   1.0
 * \date      07 Avril 2013
 */

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <boost/atomic.hpp>
#include <boost/thread/mutex.hpp>

#include <stddef.h>
#include <list>

/**
 * \class     RingBuffer
 * \brief     Defines a template-based ring buffer container that never blocks
 *            and also allows to annotate elements with meta-data.
 *
 * \details   The first property is important for applications that want a cache
 *            for the latest n elements of a stream regardless of who is going
 *            to read them. The point is that the producer should never be
 *            blocked. The problem with that property is that consumers cannot
 *            lock the content they are reading so it can be overriden by the
 *            producer at any time. This means that the consummer should be
 *            able to check if the data it read is correct. This results in
 *            multiple calls for reading.
 *
 *            The second property is important when some meta-data need to be
 *            attached to samples such as the time at which they have been taken
 *            in scenarios where the sampling rate isn't fixed. This increases
 *            the complexity of the ring buffer as both the meta data and the
 *            data cannot be added at the same time. For this reason, a staging
 *            area is introduced by this class that allows
 *
 *            **Thread-safety:** There can be only one producer without locking.
 *            Multiple consumers are ok.
 *
 *            ### Examples ###
 *
 *            #### Writing data ####
 *
 *            Here is a function that would add samples to a #RingBuffer.
 *
 *                void writeSamples(RingBuffer<int, bool> &rb, const int *samples,
 *                                size_t length, bool marker)
 *                {
 *                    uint64_t startPos = _head.load();
 *
 *                    while (length > 0)
 *                    {
 *                        int *rbData;
 *                        size_t writeLen = requestWrite(length, &r);
 *
 *                        memcpy(rbData, samples, writeLen * sizeof(Sample));
 *
 *                        // prepare for the next loop
 *                        samples += writeLen;
 *                        length -= writeLen;
 *                    }
 *                    rb.addMarker(marker, startPos);
 *                    rb.validateWrite();
 *                }
 *
 *            #### Reading data ####
 *
 *            Here is a function that would read samples from a #RingBuffer.
 *
 *                bool readSamples(RingBuffer<int, bool> &rb, uint64_t pos,
 *                                 Sample *samples, size_t length)
 *                {
 *                    uint64_t startPos = pos;
 *                    while (length > 0)
 *                    {
 *                        int *rbData;
 *                        size_t wantedLength = length;
 *
 *                        size_t readLen = requestRead(pos, &wantedLength, &rbData);
 *                        memcpy(samples, wantedLength, rbData);
 *
 *                        // prepare for the next loop
 *                        samples += readLen;
 *                        pos += readLen;
 *                        length -= readLen;
 *                    }
 *                    return ringBuffer.isPositionValid(startPos);
 *                }
 *
 */
template <class Sample, class Marker>
class RingBuffer
{
protected:
	size_t _ring_length; ///< The length of the ring buffer
	std::auto_ptr<Sample> _ring; ///< The ring buffer

	boost::atomic<uint64_t> _newHead; ///< The head of the staging area
	boost::atomic<uint64_t> _head; ///< The head of the public area
	boost::atomic<uint64_t> _tail; ///< The tail of the ring buffer

	/// Structure to associate markers to their positions
	struct MarkerInternal
	{
		uint64_t pos; ///< Position of the marker
		Marker m; ///< The marker
	};
	boost::mutex _markersMutex; ///< Mutex to protect the markers' list
	std::list<MarkerInternal> _markers; ///< The markers' list

	/// Same as #requestRead, but without boundary checks
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

	/**
	 * \brief    Construct a new ring buffer
	 *
	 * \param    size The maximum number of elements to be stored in the ring buffer.
	 * \return   Nothing.
	 */
	RingBuffer(size_t size) : _ring_length(size),
		_ring(new Sample[size * sizeof(Sample)]), _newHead(0), _head(0),
		_tail(0)
	{
	}

	/**
	 * \brief    Returns the maximum number of elements that can be stored in the ring buffer.
	 * \return   The maximum number of elements that can be stored in the ring buffer.
	 */
	size_t ringLength() const { return _ring_length; }

	/**
	 * \brief    Returns the number of elements currently stored in the ring buffer.
	 * \return   The number of elements currently stored in the ring buffer.
	 */
	size_t size() const
	{
		return _head.load() - _tail.load();
	}

	/**
	 * \brief    Checks if a position in the ring buffer is available or not.
	 * \return   True if the position is available, false otherwise.
	 */
	bool isPositionValid(uint64_t pos) const
	{
		return pos >= _tail.load() && pos < _head.load();
	}

	/**
	 * \brief    Returns head's position
	 * \return   Returns the position of the head
	 */
	uint64_t head() const
	{
		return _head.load();
	}

	/**
	 * \brief    Returns tail's position
	 * \return   Returns the position of the tail
	 */
	uint64_t tail() const
	{
		return _tail.load();
	}

	/**
	 * \brief    Empties the ring buffer and markers.
	 *
	 * \return   Nothing.
	 */
	void clear()
	{
		_markersMutex.lock();

		_newHead = 0;
		 _head = 0;
		_tail = 0;

		_markers.clear();

		_markersMutex.unlock();
	}

	/**
	 * \brief    Request reading the N last elements of the ring buffer
	 *
	 * \warning  The data may be changed by the producer while another
	 *           thread is reading data. To ensure that data hasn't been
	 *           overriden, the reader must call #isPositionValid with
	 *           \a pos as a parameter. If the function returns true,
	 *           then the data is safe to use. Otherwise, the data must be
	 *           discarded.
	 *
	 * \param[in]  n           The number of elements you want to read
	 * \param[out] startPos    Stores the position of the first element in
	 *                         the ring buffer.
	 * \param[out] restartPos  Stores the restart position in the case where
	 *                         not all element can be read in a row.
	 * \param[out] samples     Stores the pointer at which data is available.
	 *                         Make sure to only read up to the number
	 *                         returned by this function which can be lower
	 *                         than \a n.
	 *
	 * \return   The samples of elements that can be read. The number can be
	 * lower than what has been requested due to the ring buffer's wrap
	 * around. In this case, one should call #requestRead with \a restartPos as
	 * a starting position.
	 *
	 * \sa #requestRead, #isPositionValid, #addSamples, #requestWrite
	 */
	size_t requestReadLastN(size_t n, uint64_t *startPos, uint64_t *restartPos, Sample **samples)
	{
		int64_t wantedPos = _head.load() - n;
		size_t packetSize = n;

		/* check there are at least n elements */
		if (size() < n) {
			packetSize = 0;
			if (startPos)
				*startPos = wantedPos;
			if (restartPos)
				*restartPos = wantedPos;
			if (samples)
				*samples = NULL;
		} else {
			/* check that we won't be wrapping around */
			if ((wantedPos % _ring_length) + n >= _ring_length)
				packetSize = (_ring_length - (wantedPos % _ring_length));

			if (startPos)
				*startPos = wantedPos;
			if (restartPos)
				*restartPos = wantedPos + packetSize;
			if (samples)
				*samples = (_ring.get() + (wantedPos % _ring_length));
		}

		return packetSize;
	}

	/**
	 * \brief    Request reading elements from the ring buffer
	 *
	 * \warning  The data may be changed by the producer while another
	 *           thread is reading data. To ensure that data hasn't been
	 *           overriden, the reader must call #isPositionValid with
	 *           \a pos as a parameter. If the function returns true,
	 *           then the data is safe to use. Otherwise, the data must be
	 *           discarded.
	 *
	 * \param[in]  pos         The starting position in the ring buffer from
	 *                         which data should be read.
	 * \param[in,out] length   Specifies how many elements should be read.
	 *                         It also returns the number of elements that
	 *                         are actually readable.
	 * \param[out] samples     Stores the pointer at which data is available.
	 *                         Make sure to only read up to \a length
	 *                         elements. Warning, \a length can be lower
	 *                         than the requested value.
	 *
	 * \return   True if the position is available and data can be read
	 *           from it. False otherwise.
	 *
	 * \sa #requestReadLastN, #isPositionValid, #addSamples, #requestWrite
	 */
	bool requestRead(uint64_t pos, size_t *length, Sample **samples)
	{
		size_t packetSize = *length;

		if (!isPositionValid(pos)) {
			*length = 0;
			return false;
		}

		requestRead_unsafe(pos, length, samples);
		return true;
	}

	/**
	 * \brief    Request writing new elements to the ring buffer
	 *
	 * \details  This function should be prefered over #addSamples because
	 *           it avoids an uncessary copy.
	 *
	 * \warning  The data added will be staged but not made public until
	 *           #validateWrite is called. This staging area is meant to let
	 *           developers add both data and markers before making
	 *           them public.
	 *
	 * \param[in]  length      The number of elements to be written.
	 * \param[out] samples     Stores the pointer at which data should be
	 *                         written.
	 *                         Make sure to only write up to the number of
	 *                         elements returned by the function.
	 *
	 * \return   The number of elements that can be written at \a samples.
	 *           WARNING: This number may be lower than \a length.
	 *
	 * \sa #addSamples, #validateWrite, #requestRead, #requestReadLastN
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

	/**
	 * \brief    Flush the staging area and make the newly-added data public
	 *
	 * \details  When data is added, it is staged and not made public until
	 *           #validateWrite is called. This staging area is meant to let
	 *           developers add both data and markers before making
	 *           them public.
	 *
	 * \return   Nothing.
	 *
	 * \sa #requestWrite, #addSamples
	 */
	void validateWrite()
	{
		_head.store(_newHead.load());
	}

	/**
	 * \brief    Write new elements to the ring buffer
	 *
	 * \warning  This function should be avoided as possible. Please prefer
	 *           avoiding a copy by using #requestWrite.
	 *
	 * \warning  The data added will be staged but not made public until
	 *           #validateWrite is called. This staging area is meant to let
	 *           developers add both data and markers before making
	 *           them public.
	 *
	 * \param[in] samples      The samples to be written to the ring buffer
	 * \param[in] length       The number of elements from samples to be
	 *                         added to the ring buffer.
	 *
	 * \return   The address of the first element added to the ring buffer.
	 *
	 * \sa #requestWrite, #validateWrite, #requestRead, #requestReadLastN
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

	/**
	 * \brief    Add a marker at a specific position
	 *
	 * \param[in] marker      The marker to be added
	 * \param[in] pos         The position at which the marker \a marker
	 *                        should be added. If a marker already exists at
	 *                        this position. It will be replaced by
	 *                        the new marker \a marker.
	 *
	 * \return   True if the position was valid and the marker has been
	 *           added. False if the position was invalid.
	 *
	 * \sa #getMarker, #findMarker
	 */
	bool addMarker(Marker marker, uint64_t pos)
	{
		if (!isPositionValid(pos))
			return false;

		_markersMutex.lock();

		MarkerInternal m = {pos, marker};

		typename std::list<MarkerInternal>::reverse_iterator rit;
		for (rit =_markers.rbegin(); rit!=_markers.rend(); ++rit) {
			if ((*rit).pos == pos) {
				(*rit).m = marker;
			} else if ((*rit).pos < pos) {
				_markers.insert(rit.base(), m);
				goto exit;
			}
		}

		_markers.push_front(m);

exit:
		_markersMutex.unlock();
		return true;
	}

	/**
	 * \brief    Get a marker at a known position
	 *
	 * \param[in] markerPos   The position at which the marker should be
	 *                        retrieved.
	 * \param[out] marker     Stores the retrieved marker.
	 *
	 * \return   True if the marker has been retrieved, false otherwise.
	 *
	 * \sa #findMarker, #addMarker
	 */
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

	/**
	 * \brief    Find the marker right before a position
	 *
	 * \param[in] pos         The position from which the marker should be
	 *                        searched for.
	 * \param[out] marker     Stores the retrieved marker.
	 * \param[out] markerPos  Stores the marker's position
	 *
	 * \return   True if a marker has been found, false otherwise.
	 *
	 * \sa #getMarker, #addMarker
	 */
	bool findMarker(uint64_t pos, Marker *marker, uint64_t *markerPos)
	{
		bool ret = false;

		if (!isPositionValid(pos))
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
};

#endif // RINGBUFFER_H
