#ifndef MESSAGEHEAP_H
#define MESSAGEHEAP_H

#include <list>
#include <mutex>

#include "message.h"

class MessageHeap
{
	std::list<Message> _msgs;
	std::mutex _mutex;

	size_t _sizeLimit;
public:
	MessageHeap(size_t sizeLimit = 10);
	size_t sizeLimit() const { return _sizeLimit; }
	size_t size() const { return _msgs.size(); }

	void lock();
	void unlock();

	const Message& at(size_t i) const;

	bool addMessage(const Message& msg);
	bool addMessage_unsafe(const Message& msg);
	Message extract(size_t i);
};

#endif // MESSAGEHEAP_H
