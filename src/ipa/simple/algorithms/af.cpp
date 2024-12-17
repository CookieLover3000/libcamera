#include "af.h"

#include <libcamera/base/log.h>

namespace libcamera {

namespace ipa::soft::algorithms {

Af::Af()
	: lensPos(0), highest(0,0)
{
}

void Af::process([[maybe_unused]] IPAContext &context, [[maybe_unused]] const uint32_t frame,
		 [[maybe_unused]] IPAFrameContext &frameContext, [[maybe_unused]] const SwIspStats *stats,
		 [[maybe_unused]] ControlList &metadata)
{
	switch (context.activeState.af.state) {
	case 0:
		initState(context);
		break;
	case 1: //Locked
		lockedState(context, stats);
		break;
	case 2: // Full Sweep
		fullSweepState(context, stats);
		break;
	case 3: // small sweep (hill climb)
        smallSweepState(context, stats);
		break;
	}
}

void Af::initState([[maybe_unused]] IPAContext &context)
{
	if (itt < 255) {
		itt++;
	} else {
		itt = 0;
		context.activeState.af.state = 2; // full sweep
	}
}

void Af::lockedState([[maybe_unused]] IPAContext &context, [[maybe_unused]] const SwIspStats *stats)
{
	if (sharpnessLock * 0.6 > stats->sharpnessValue_) { // to sweep
		lensPos = 0;
		context.activeState.af.focus = lensPos;
		context.activeState.af.state = 2; // full sweep
	} else if (sharpnessLock * 0.8 > stats->sharpnessValue_) { // to smallsweep
		if (lensPos < 200) {
			lensPos = 0;
		} else
			lensPos = lensPos - 200;
		context.activeState.af.focus = lensPos;
		itt = 0;
		context.activeState.af.state = 3; // small sweep
	}
}

void Af::fullSweepState([[maybe_unused]] IPAContext &context, [[maybe_unused]] const SwIspStats *stats)
{
	/*if (lensPos < 1023) { // TODO: CHANGE TO DYNAMIC VALUE
		values[lensPos] = stats->sharpnessValue_;
		context.activeState.af.sharpnessLock = values[lensPos];
		context.activeState.af.focus = lensPos;
		lensPos++;
	} else {
		std::pair<uint8_t, uint64_t> highest;
		std::map<uint8_t, uint64_t>::iterator currentEntry;
		for (currentEntry = values.begin(); currentEntry != values.end(); ++currentEntry) {
			if (currentEntry->second > highest.second) {
				highest = std::make_pair(currentEntry->first, currentEntry->second);
				context.activeState.af.focus = highest.first;
				context.activeState.af.sharpnessLock = highest.second;
				sharpnessLock = highest.second;
			}
		}
		context.activeState.af.state = 1; // locked
	}*/
    uint64_t sharpness = stats->sharpnessValue_;
    if (lensPos < 1023) {
        if (sharpness > highest.second) {
            highest = std::make_pair(lensPos, sharpness);
        }
        lensPos++;
        context.activeState.af.focus = lensPos;
    } else {
        lensPos = highest.first;
        sharpnessLock = highest.second;
        highest = std::make_pair(0,0);
        context.activeState.af.sharpnessLock = sharpnessLock;
        context.activeState.af.focus = lensPos;
        context.activeState.af.state = 1;
    }
}

void Af::smallSweepState([[maybe_unused]] IPAContext &context, [[maybe_unused]] const SwIspStats *stats)
{
    uint64_t sharpness = stats.sharpnessValue_;
	if (itt < 400) {
		if (sharpness > highest.second) {
            highest = std::make_pair(lensPos, sharpness);
        }
        lensPos++;
        itt++;
        context.activeState.af.focus = lensPos;
	} else {
        lensPos = highest.first;
        sharpnessLock = highest.second;
        highest = std::make_pair(0,0);
        context.activeState.af.sharpnessLock = sharpnessLock;
        context.activeState.af.focus = lensPos;
        context.activeState.af.state = 1;
	}
}

REGISTER_IPA_ALGORITHM(Af, "Af")

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */