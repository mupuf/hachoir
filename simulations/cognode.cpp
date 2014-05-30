#include "cognode.h"
#include <algorithm>
#include <iostream>

std::list<CogNode*> CogNode::_nodes;
std::map<uint64_t, CogNode::frame_t> CogNode::_frames;

#define BEACON_EMIT_AFTER 5000
#define BEACON_BANDWIDTH 500e3
#define BEACON_BITRATE 1e6

CogNode::CogNode(uint32_t id, float x, float y, uint64_t beaconPeriod, HoppingPattern hp) :
	_id(id), _x(x), _y(y), _beaconPeriod(beaconPeriod), _hp(hp), _ticks_count(0)
{
	_nodes.push_back(this);

	for (size_t i = 0; i < hp.bandsCount(); i++)
		_frameIDs.push_back(0);
}

CogNode::~CogNode()
{
	_nodes.remove(this);
}

uint64_t CogNode::usUntilNextOperation()
{
	uint64_t next = -1, nextAvailBand = -1;

	uint64_t beacon_tx_time_us = _hp.beaconSize() * 8 * 1e6 / BEACON_BITRATE;

	uint64_t sensingFor, sensingSince, sensingNextJump;
	if (_hp.isSensing(sensingSince, sensingFor, sensingNextJump))
		return sensingNextJump < sensingFor ? sensingNextJump : sensingFor;

	for (size_t i = 0; i < _hp.bandsCount(); i++) {
		uint64_t since_us, for_us, band_next = -1;
		if (!_hp.bandInfo(i, since_us, for_us)) {
			if (nextAvailBand > for_us)
				nextAvailBand = for_us;
			//std::cout << i << ": Will be available in " << for_us << std::endl;
			continue;
		}

		if (since_us < BEACON_EMIT_AFTER)
			band_next = BEACON_EMIT_AFTER - since_us;
		else if (since_us < BEACON_EMIT_AFTER + beacon_tx_time_us)
			band_next = BEACON_EMIT_AFTER - since_us + beacon_tx_time_us;
		else {
			uint64_t curPos = ((since_us - BEACON_EMIT_AFTER) % _beaconPeriod);
			if (curPos >= beacon_tx_time_us)
				band_next = _beaconPeriod - curPos;
			else
				band_next = beacon_tx_time_us - curPos;
		}

		//std::cout << i << ": won't be available in " << for_us << std::endl;

		/* if the next jump is closer than the next beacon
		 * we need to send, move to it
		 */
		if (band_next > for_us)
			band_next = for_us;

		if (band_next < next)
			next = band_next;
	}

	if (next != -1)
		return next;
	else
		return nextAvailBand;
}

void CogNode::addTicks(uint64_t us)
{
	/*uint64_t next = usUntilNextOperation();
	if (us > next) {
		std::cerr << "CogNode::addTicks: BUG! Tried to add more ticks "
			     "than usUntilNextOperation()" << std::endl;
	}*/

	_hp.addTicks(us);
	_ticks_count += us;

	uint64_t beacon_tx_time_us = _hp.beaconSize() * 8 * 1e6 / BEACON_BITRATE;

	for (size_t i = 0; i < _hp.bandsCount(); i++) {
		uint64_t since_us, for_us;
		if (!_hp.bandInfo(i, since_us, for_us))
			continue;

		Band cB = _hp.bandAt(i);
		float centralFreq = cB.start() + (cB.end() - cB.start()) / 2;
		Band beaconBand(centralFreq - BEACON_BANDWIDTH / 2, centralFreq + BEACON_BANDWIDTH / 2);

		if (since_us == BEACON_EMIT_AFTER ||
		    ((since_us - BEACON_EMIT_AFTER) % _beaconPeriod) == 0) {
			uint64_t fID = startFrame(id(), beaconBand);
			_frameIDs[i] = fID;
		} else if(since_us == BEACON_EMIT_AFTER + beacon_tx_time_us ||
			((since_us - BEACON_EMIT_AFTER) % _beaconPeriod) == beacon_tx_time_us)
			stopFrame(_frameIDs[i]);
	}
}

bool CogNode::frameStarted(Band b) const
{
	return _hp.isBandIncludedInCurrentBands(b);
}

bool CogNode::frameStopped(Band b) const
{
	return _hp.isBandIncludedInCurrentBands(b);
}

uint64_t CogNode::startFrame(uint32_t src, Band b)
{
	static uint64_t _fID;

	frame_t frame;
	frame.src = src;
	frame.b = b;

	//std::cout << "startFrame " << _fID << " from node " << frame.src << ": ";
	for (auto &n : _nodes) {
		if (n->frameStarted(b)) {
			frame.nodes.insert(n);
			//std::cout << " " << n->id();
		}
	}
	//std::cout << std::endl;

	_frames[_fID] = frame;

	return _fID++;
}

void CogNode::stopFrame(uint64_t fID)
{
	frame_t frame = _frames[fID];
	std::set<CogNode*> recvSet;

	for (auto &n : _nodes) {
		if (n->frameStopped(frame.b))
			recvSet.insert(n);
	}

	std::set<CogNode*> inters;
	set_intersection(frame.nodes.begin(), frame.nodes.end(),
			 recvSet.begin(), recvSet.end(),
			 std::inserter(inters, inters.begin()));

	//std::cout << "stopFrame " << fID << " from node " << frame.src << ":";
	for (auto it=inters.begin(); it!=inters.end(); ++it) {
		(*it)->_neighbours.insert(frame.src);
		//std::cout << " " << (*it)->id();
	}
	//std::cout << '\n';

	_frames.erase(_frames.find(fID));
}
