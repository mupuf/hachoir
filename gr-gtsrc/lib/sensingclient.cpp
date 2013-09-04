#include "sensingclient.h"

#include <stdint.h>
#include <stdio.h>
#include <iostream>

SensingClient::SensingClient(boost::asio::io_service &ios) : _socket(ios)
{
}

bool SensingClient::readNBytes(char *buf, size_t size)
{
	size_t len = 0;

	while (len < size) {
		size_t subLen = _socket.read_some(boost::asio::buffer(buf+size, size - len));
		if (subLen == 0)
			return false;
		len += subLen;
	}

	return true;
}

#include "message_utils.h"
void SensingClient::socketIsConnected()
{
	try {
		uint8_t msgType = 0;
		do {
			size_t len = _socket.read_some(boost::asio::buffer(&msgType, 1));
			if (len != 1)
				return;

			switch (msgType)
			{
			case 0x0:
				/* End, nothing to do */
				break;
			case 0x1:
				_fullSensing = true;
				break;
			case 0x2:
				freqInterest f;

				_socket.read_some(boost::asio::buffer(&f.centralFreq, 8));
				_socket.read_some(boost::asio::buffer(&f.bandwidth, 4));
				_socket.read_some(boost::asio::buffer(&f.error, 1));

				_frequencies.push_back(f);

				fprintf(stderr, "cf = %llu, bandwidth = %u, error = %u\n",
					f.centralFreq, f.bandwidth, f.error);
				break;
			default:
				fprintf(stderr, "Unknown message type %u\n", msgType);
				break;
			}
		} while (msgType > 0);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

bool SensingClient::send(const char *buf, size_t len)
{
	try {
		size_t sentLen = _socket.send(boost::asio::buffer(buf, len));
		if (sentLen != len) {
			fprintf(stderr, "SensingClient::send: sentLen != len\n");
			return false;
		} else
			return true;
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return false;
	}
}
