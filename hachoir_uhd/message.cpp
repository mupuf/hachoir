#include "message.h"
#include <iostream>

void Message::printBinary(std::ostream &stream) const
{
	int i = 0;
	for (bool bit : data) {
		stream << bit;
		if (((i++) % 4) == 3)
			stream << " ";
	}
}

void Message::printHex(std::ostream &stream) const
{

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

void Message::print(std::ostream &stream, MessagePrintStyle style) const
{
	switch(style) {
	case BINARY:
		printBinary(stream);
		break;
	case HEX:
		printHex(stream);
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
