#include "sensingserver.h"

void SensingServer::io_service()
{
	startListening();
	_ios.run();
}

void SensingServer::incomingConnection(SensingClient *client,
				       const boost::system::error_code& error)
{
	if (!error) {
		_clientsMutex.lock();
		_clients.push_back(client);
		_clientsMutex.unlock();
	}

	startListening();
}

SensingServer::SensingServer(uint16_t port) : _port(port),
	_endpoint(boost::asio::ip::tcp::v4(), _port), _acceptor(_ios, _endpoint)
{
	_io_thread = boost::thread(&SensingServer::io_service, this);
}

void SensingServer::startListening()
{
	SensingClient *client = new SensingClient(_ios);

	_acceptor.async_accept(client->socket(),
		boost::bind(&SensingServer::incomingConnection, this,
			    client, boost::asio::placeholders::error));
}

void SensingServer::stopListening()
{
	_acceptor.cancel();
}

void SensingServer::sendToAll(const char *buf, size_t len)
{
	_clientsMutex.lock();

	std::list<SensingClient *>::iterator it;
	for (it = _clients.begin(); it != _clients.end(); ++it) {
		if (!(*it)->send(buf, len)) {
			delete (*it);
			it = _clients.erase(it);
		}
	}

	_clientsMutex.unlock();
}
