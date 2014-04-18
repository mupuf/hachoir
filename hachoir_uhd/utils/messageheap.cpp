#include "messageheap.h"

MessageHeap::MessageHeap(size_t sizeLimit) : _sizeLimit(sizeLimit)
{
}

void MessageHeap::lock()
{
	_mutex.lock();
}

void MessageHeap::unlock()
{
	_mutex.unlock();
}

const Message& MessageHeap::at(size_t i) const
{
	auto it = _msgs.begin();
	std::advance(it, i);
	return *it;
}

bool MessageHeap::addMessage_unsafe(const Message& msg)
{
	if (_msgs.size() + 1 < sizeLimit()) {
		_msgs.push_back(msg);
		return true;
	} else
		return false;
}

bool MessageHeap::addMessage(const Message& msg)
{
	bool ret;

	lock();
	ret = addMessage_unsafe(msg);
	unlock();

	return ret;
}

Message MessageHeap::extract(size_t i)
{
	auto it = _msgs.begin();
	std::advance(it, i);

	Message m = *it;
	_msgs.erase(it);

	return m;
}
