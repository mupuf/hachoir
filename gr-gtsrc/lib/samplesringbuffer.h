/**
 * \file      samplesringbuffer.h
 * \author    MuPuF (Martin Peres <martin.peres@labri.fr>)
 * \version   1.0
 * \date      08 Avril 2013
 */

#ifndef SAMPLESRINGBUFFER_H
#define SAMPLESRINGBUFFER_H

#include "ringbuffer.h"

/// The #SamplesRingBuffer's markers that annotate the samples stream
struct RBMarker
{
	uint64_t freq; ///< The central frequency of the samples' packet
	uint64_t sampleRate; ///< The sample rate of the samples' packet
	uint64_t time; ///< The time in nanoseconds of the first sample (relative to 01/01/1970)
};

/**
 * \class     SamplesRingBuffer
 * \brief     A gr_complex #RingBuffer that adds information about frequency,
 *            sample rate and time to the samples.
 *
 * \details   **Thread-safety:** There can be only one producer without locking.
 *            Multiple consumers are ok.
 */
class SamplesRingBuffer : public RingBuffer<gr_complex, RBMarker>
{
public:
	/**
	 * \brief    Construct a new Sample ring buffer
	 *
	 * \param    size   The maximum number of elements to be stored in the
	 *                  ring buffer.
	 * \return   Nothing.
	 */
	SamplesRingBuffer(size_t size) : RingBuffer(size)
	{

	}

	/**
	 * \brief    Get the time from a marker and a specific positive offset
	 *
	 * \param[in]  marker      The marker from which time should be taken
	 * \param[in]  offset      The positive sample offset that should be
	 *                         added to the marker's time.
	 *
	 * \return   The number of nanoseconds elapsed since the first of
	 *           January 1970 when the sample at \a marker.pos + \a offset
	 *           arrived.
	 *
	 * \sa #timeAt, #timeAtMarker
	 */
	uint64_t timeAtWithMarker(const RBMarker &marker, size_t offset)
	{
		uint64_t samplePeriod = 1000000000/marker.sampleRate;
		return marker.time + offset * samplePeriod;
	}

	/**
	 * \brief    Get the time from a specific marker
	 *
	 * \param[in]  markerPos   The position of the marker from which the
	 *                         time should be taken.
	 *
	 * \return   The number of nanoseconds elapsed since the first of
	 *           January 1970 when the sample at \a markerPos arrived. 0 if
	 *           the marker doesn't exist.
	 *
	 * \sa #timeAt, #timeAtWithMarker
	 */
	uint64_t timeAtMarker(uint64_t markerPos)
	{
		RBMarker marker;

		if (getMarker(markerPos, &marker))
			return timeAtWithMarker(marker, 0);
		else
			return 0;
	}

	/**
	 * \brief    Get the time at a specific position in the sample stream
	 *
	 * \param[in]  pos         The position at which the time should be
	 *                         retrieved.
	 *
	 * \return   The number of nanoseconds elapsed since the first of
	 *           January 1970 when the sample at \a markerPos arrived. 0 if
	 *           no marker has been found.
	 *
	 * \sa #timeAtMarker, #timeAtWithMarker
	 */
	uint64_t timeAt(uint64_t pos)
	{
		RBMarker marker;
		uint64_t markerPos;

		if (findMarker(pos, &marker, &markerPos))
			return timeAtWithMarker(marker, pos - markerPos);
		else
			return 0;
	}
};

#endif // SAMPLESRINGBUFFER_H
