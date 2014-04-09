#ifndef MESSAGE_H
#define MESSAGE_H

#include <boost/dynamic_bitset.hpp>
#include <stdint.h>
#include <ostream>

class Message
{
	boost::dynamic_bitset<> data;

	std::string _modulation;
	// TODO: Central freq of the signal, bw

	std::string toStringBinary() const;
	std::string toStringHex() const;

public:
	enum MessagePrintStyle {
		BINARY = 0,
		HEX = 1
	};

	Message (const std::string &modulation = std::string());

	void addBit(bool b);
	size_t size() const;
	void clear();

	std::string modulation() const;

	std::string toString(MessagePrintStyle style) const;
	void print(std::ostream &stream, MessagePrintStyle style) const;

	Message &operator<< (bool bit);
	bool operator[](size_t i) const;
};

#endif // MESSAGE_H
