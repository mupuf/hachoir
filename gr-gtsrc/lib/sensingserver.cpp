#include "sensingserver.h"
#include "message_utils.h"

void SensingServer::io_service()
{
	startListening();
	_ios.run();
}

void SensingServer::incomingConnection(SensingClient *client,
				       const boost::system::error_code& error)
{
	startListening();

	if (!error) {
		client->socketIsConnected();

		_clientsMutex.lock();
		_clients.push_back(client);
		_clientsMutex.unlock();
	}
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
		if ((*it)->wantsFullSensing()) {
			if (!(*it)->send(buf, len)) {
				delete (*it);
				it = _clients.erase(it);
			}
		}
	}

	_clientsMutex.unlock();
}

void SensingServer::sendDetection(std::list<SensingClient *>::iterator &client, const SensingClient::freqInterest &f)
{
	char buffer[20];
	size_t offset = 0;

	write_and_update_offset(offset, buffer, (char)2);
	write_and_update_offset(offset, buffer, f.centralFreq);
	write_and_update_offset(offset, buffer, f.bandwidth);
	write_and_update_offset(offset, buffer, f.error);

	if (!(*client)->send(buffer, offset)) {
		/*delete (*client);
		client = _clients.erase(client);*/
	}
}

void SensingServer::matchActiveCommunications(RadioEventTable &ret)
{

	/* for all clients */
	std::list<SensingClient *>::iterator itClient;
	for (itClient = _clients.begin(); itClient != _clients.end(); ++itClient) {
		if ((*itClient)->wantsFullSensing())
			continue;

		/* for all wanted communications */
		std::list< SensingClient::freqInterest >::iterator itWanted;
		for (itWanted = (*itClient)->frequencies().begin(); itWanted != (*itClient)->frequencies().end(); ++itWanted) {
			SensingClient::freqInterest f = (*itWanted);

			uint32_t fStart = f.centralFreq - f.bandwidth / 2;
			uint32_t fEnd = f.centralFreq + f.bandwidth / 2;
			RetEntry *entry = ret.findMatchInActiveCommunications(fStart, fEnd, 0);

			if (entry) {
				fprintf(stderr, "Found a matching communication: fc = %llu kHz\n",
					f.centralFreq);
				sendDetection(itClient, f);
			}
		}
	}
}
