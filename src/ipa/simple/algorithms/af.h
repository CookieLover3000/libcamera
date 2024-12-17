#pragma once

#include "algorithm.h"

namespace libcamera {

namespace ipa::soft::algorithms {

class Af : public Algorithm
{
public:
	Af();
	~Af() = default;

	void process(IPAContext &context, const uint32_t frame,
		     IPAFrameContext &frameContext,
		     const SwIspStats *stats,
		     ControlList &metadata) override;

private:
	void fullSweepState(IPAContext &context, const SwIspStats *stats);
	void smallSweepState(IPAContext &context, const SwIspStats *stats);
	void lockedState(IPAContext &context, const SwIspStats *stats);
	void initState(IPAContext &context);

	int32_t lensPos;
	std::map<uint8_t, uint64_t> values;
	uint64_t sharpnessLock;
	uint32_t itt;
	std::pair<int32_t, uint64_t> highest;
};

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */