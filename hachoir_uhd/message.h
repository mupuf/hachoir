#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <vector>
#include <ostream>

class Message
{
	std::vector<bool> data;

	std::string toStringBinary() const;
	std::string toStringHex() const;
public:
	enum MessagePrintStyle {
		BINARY = 0,
		HEX = 1
	};

	void addBit(bool b);
	size_t size() const;
	void clear();

	std::string toString(MessagePrintStyle style) const;
	void print(std::ostream &stream, MessagePrintStyle style) const;

	Message &operator<< (bool bit);
	bool operator[](size_t i);
};

#endif // MESSAGE_H
