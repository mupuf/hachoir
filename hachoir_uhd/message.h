#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <vector>
#include <ostream>

class Message
{
	std::vector<bool> data;

	void printBinary(std::ostream &stream) const;
	void printHex(std::ostream &stream) const;
public:
	enum MessagePrintStyle {
		BINARY = 0,
		HEX = 1
	};

	void addBit(bool b);
	size_t size() const;
	void clear();

	void print(std::ostream &stream, MessagePrintStyle style) const;

	Message &operator<< (bool bit);
	bool operator[](size_t i);
};

#endif // MESSAGE_H
