#include "af.h"

#include <libcamera/base/log.h>

namespace libcamera {
	LOG_DEFINE_CATEGORY(af)

namespace ipa::soft::algorithms {

Af::Af()
	: lensPos(0), highest(0,0), stable(false)
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

	if(stable) {
		itt++;
		if(itt >= 20) {
			stable = false;
		}
		return;
	}
	

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
	int32_t focusMax = context.configuration.af.afocusMax;
    uint64_t sharpness = stats->sharpnessValue_;
    if (lensPos < focusMax) {
        if (sharpness > highest.second) {
            highest = std::make_pair(lensPos, sharpness);
			LOG(af, Info) << "Highest Sharpness: " << highest.second;
			LOG(af, Info) << "Highest focus pos: " << (int32_t)highest.first;
        }
        lensPos++;
        context.activeState.af.focus = lensPos;
    } else {
        lensPos = highest.first;
        sharpnessLock = highest.second;
        highest = std::make_pair(0,0);
        context.activeState.af.sharpnessLock = sharpnessLock;
        context.activeState.af.focus = lensPos;
		stable = true;
        context.activeState.af.state = 1;
    }
}

void Af::smallSweepState([[maybe_unused]] IPAContext &context, [[maybe_unused]] const SwIspStats *stats)
{
    uint64_t sharpness = stats->sharpnessValue_;
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