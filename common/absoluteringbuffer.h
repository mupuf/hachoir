#ifndef ABSOLUTERINGBUFFER_H
#define ABSOLUTERINGBUFFER_H

#include <stdint.h>

#include <vector>
#include <memory>

template <class T>
class AbsoluteRingBuffer
{
	size_t _ringSize;
	std::vector< std::shared_ptr<T> > _ring;

	uint64_t _head;
	uint64_t _tail;

public:
	AbsoluteRingBuffer(size_t ringSize) : _ringSize(ringSize), _head(0), _tail(0)
	{
		_ring.reserve(ringSize);
		for (size_t i = 0; i < ringSize; i++)
			_ring.push_back(std::shared_ptr<T>());
	}

	size_t ringSize() const { return _ringSize; }
	size_t size() const { return _head - _tail; }

	uint64_t head() const { return _head;}
	uint64_t tail() const { return _tail;}

	bool isPositionValid(uint64_t i)
	{
		return i >= _tail && i < _head;
	}

	uint64_t pushBack(T *e)
	{
		/* reserve some space */
		if (size() >= ringSize())
			_tail++;

		/* replace the element */
		_ring[_head % _ringSize].reset(e);

		return _head++;
	}

	std::vector< std::shared_ptr<T> > getRange(uint64_t &start, uint64_t &end)
	{
		std::vector< std::shared_ptr<T> > v;

		/* check that we at least have a length of 1 */
		if (end < start + 1)
			return v;
		size_t length = end - start - 1;

		/* make sure the window is in the available range */
		if (start < _tail) {
			start = _tail;
			end = start + length + 1;
		}
		if (end >= _head)
			end = _head;

		v.reserve(length);

		for (uint64_t i = start; i < end; i++)
			v.push_back(_ring.at(i % _ringSize));

		return v;
	}

	std::shared_ptr<T> at(uint64_t i)
	{
		if (isPositionValid(i))
			return _ring.at(i % _ringSize);
		else
			return std::shared_ptr<T>();
	}

	T * at_unsafe(uint64_t i)
	{
		if (isPositionValid(i))
			return _ring.at(i % _ringSize).get();
		else
			return NULL;
	}

};

#endif // ABSOLUTERINGBUFFER_H
