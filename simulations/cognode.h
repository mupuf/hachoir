#ifndef COGNODE_H
#define COGNODE_H

#include "band.h"
#include "hoppingpattern.h"

#include <list>
#include <map>
#include <set>
#include <stdint.h>

class CogNode
{
	static std::list<CogNode*> _nodes;

	struct frame_t { uint32_t src; Band b; std::set<CogNode*> nodes; };
	std::map<uint64_t, frame_t> _frames;

	uint32_t _id;
	float _x;
	float _y;
	uint64_t _beaconPeriod;
	size_t _beaconCount;
	HoppingPattern _hp;

	uint64_t _ticks_count;
	std::vector<uint64_t> _frameIDs;
	std::set<uint32_t> _neighbours;

public:
	CogNode(uint32_t id, float x, float y, uint64_t beaconPeriod, HoppingPattern hp, size_t beaconCount);
	~CogNode();

	uint32_t id() const { return _id; }
	uint64_t ticksSinceStart() const { return _ticks_count; }
	bool hasNeighbour(uint32_t id) const { return _neighbours.find(id) != _neighbours.end(); }

	uint64_t usUntilNextOperation();
	void addTicks(uint64_t us);

	bool frameStarted(Band b) const;
	bool frameStopped(Band b) const;

	uint64_t startFrame(uint32_t src, Band b);
	void stopFrame(uint64_t fID);
};

#endif // COGNODE_H
