#include "sensingclient.h"

#include <stdio.h>
#include <iostream>

SensingClient::SensingClient(boost::asio::io_service &ios) : _socket(ios)
{

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
