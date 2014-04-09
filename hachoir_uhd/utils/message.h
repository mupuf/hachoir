#ifndef MESSAGE_H
#define MESSAGE_H

#include <boost/dynamic_bitset.hpp>
#include <stdint.h>
#include <ostream>
#include <memory>

#include "modulations/modulation.h"

class Message
{
	boost::dynamic_bitset<> data;

	std::shared_ptr<Modulation> _modulation;

	std::string toStringBinary() const;
	std::string toStringHex() const;

public:
	enum MessagePrintStyle {
		BINARY = 0,
		HEX = 1
	};

	Message();

	void addBit(bool b);
	size_t size() const;
	void clear();

	std::shared_ptr<Modulation> modulation() const;
	void setModulation(std::shared_ptr<Modulation> mod);

	std::string toString(MessagePrintStyle style) const;
	void print(std::ostream &stream, MessagePrintStyle style) const;

	Message &operator<< (bool bit);
	bool operator[](size_t i) const;
};

#endif // MESSAGE_H
