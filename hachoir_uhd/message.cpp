#include "message.h"
#include <iostream>
#include <sstream>
#include <iomanip>

std::string Message::toStringBinary() const
{
	std::stringstream ss;

	for (boost::dynamic_bitset<>::size_type i = 0; i < data.size(); ++i) {
		ss << data[i];
		if ((i % 4) == 3)
			ss << " ";
	}

	return ss.str();
}

std::string Message::toStringHex() const
{
	boost::dynamic_bitset<>::size_type i;
	std::stringstream ss;

	// print in hex all the bytes
	for (i = 0; i < (data.size() / 8) * 8; i+=8) {
		boost::dynamic_bitset<>::size_type b;
		uint8_t tmp = 0;

		for (b = i; b < i + 8; b++) {
			size_t bit_idx = 7 - b;
			tmp |= (data[b] << bit_idx);
		}

		ss << std::hex << std::setfill('0') << std::setw(2)
		   << (int) tmp << " ";
	}

	if (i == data.size())
		return ss.str();

	// print the rest in binary
	ss << "(";
	for (; i < data.size(); ++i) {
		ss << std::hex << std::setw(1) << data[i];
		if ((i % 4) == 3)
			ss << " ";
	}
	ss << ")";

	return ss.str();
}

void Message::addBit(bool b)
{
	data.push_back(b);
}

size_t Message::size() const
{
	return data.size();
}

void Message::clear()
{
	data.clear();
}

std::string Message::toString(MessagePrintStyle style) const
{
	switch(style) {
	case BINARY:
		return toStringBinary();
	case HEX:
		return toStringHex();
	default:
		std::cerr << "Message::print: unknown print style" << std::endl;
		return std::string();
	}
}

void Message::print(std::ostream &stream, MessagePrintStyle style) const
{
	switch(style) {
	case BINARY:
		stream << toStringBinary();
		break;
	case HEX:
		stream << toStringHex();
		break;
	default:
		std::cerr << "Message::print: unknown print style" << std::endl;
	}
}

Message &Message::operator<< (bool bit)
{
	addBit(bit);
}

bool Message::operator[](size_t i)
{
	return data[i];
}
