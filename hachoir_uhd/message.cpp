#include "message.h"
#include <iostream>
#include <sstream>

std::string Message::toStringBinary() const
{
	std::stringstream ss;

	int i = 0;
	for (bool bit : data) {
		ss << bit;
		if (((i++) % 4) == 3)
			ss << " ";
	}

	return ss.str();
}

std::string Message::toStringHex() const
{
	// TODO
	return std::string();
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
