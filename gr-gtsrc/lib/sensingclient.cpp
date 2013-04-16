#include "sensingclient.h"

#include <stdio.h>
#include <iostream>

SensingClient::SensingClient(boost::asio::io_service &ios) : _socket(ios)
{
}

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
