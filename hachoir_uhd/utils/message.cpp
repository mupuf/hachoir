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
		uint8_t tmp = byteAt(i / 8);

		ss << std::hex << std::setfill('0') << std::setw(2)
		   << (int) tmp << " ";
	}

	if (i == data.size())
		return ss.str();

	// print the rest in binary
	ss << "(";
	for (; i < data.size(); ++i) {
		ss << std::hex << std::setw(1) << data[i];
		if ((i % 4) == 3 && i < data.size() - 1)
			ss << " ";
	}
	ss << ")";

	return ss.str();
}

Message::Message () : _repeat_count(0)
{
}

Message::Message(std::initializer_list<uint8_t> bytes) : _repeat_count(0)
{
	addBytes(bytes);
}

void Message::addBit(bool b)
{
	data.push_back(b);
}

void Message::addByte(uint8_t byte)
{
	for (int i = 7; i >= 0; i--)
		data.push_back((byte >> i) & 1);
}

void Message::addBytes(std::initializer_list<uint8_t> bytes)
{
	for (auto it = bytes.begin(); it != bytes.end(); ++it)
		addByte(*it);
}

void Message::addBytes(const uint8_t *bytes, size_t len)
{
	for (size_t i = 0; i < len; i++)
		addByte(bytes[i]);
}

size_t Message::size() const
{
	return data.size();
}

void Message::clear()
{
	data.clear();
}

bool Message::bitAt(size_t i) const
{
	return data[i];
}

uint8_t Message::byteAt(size_t i) const
{
	return symbolAt(i, 8);
}

size_t Message::symbolAt(size_t i, size_t bps) const
{
	boost::dynamic_bitset<>::size_type b;
	size_t tmp = 0;

	for (b = 0; b < bps && (i * bps + b) < data.size(); b++) {
		size_t bit_idx = bps - b - 1;
		tmp |= (data[i * bps + b] << bit_idx);
	}

	return tmp;
}

size_t Message::symbolCount(size_t bps) const
{
	if (data.size() % bps == 0)
		return data.size() / bps;
	else
		return (data.size() / bps) + 1;
}

size_t Message::repeatCount() const
{
	return _repeat_count;
}

void Message::setRepeatCount(size_t repeat_count)
{
	_repeat_count = repeat_count;
}

std::shared_ptr<Modulation> Message::modulation() const
{
	return _modulation;
}

void Message::setModulation(std::shared_ptr<Modulation> mod)
{
	_modulation = mod;
}

bool Message::toBuffer(uint8_t *buf, size_t *len) const
{
	if (*len < data.size() / 8)
		return false;

	for (size_t i = 0; i < (data.size() / 8) * 8; i+=8) {
		buf[i / 8] = byteAt(i / 8);
	}

	*len = data.size() / 8;

	return true;
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

Message& Message::operator<< (bool bit)
{
	addBit(bit);
	return *this;
}

Message& Message::operator<< (uint8_t byte)
{
	addByte(byte);
	return *this;
}

Message& Message::operator<< (std::initializer_list<uint8_t> bytes)
{
	addBytes(bytes);
	return *this;
}

bool Message::operator[](size_t i) const
{
	return bitAt(i);
}
