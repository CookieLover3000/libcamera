#include "af.h"

#include <libcamera/base/log.h>

namespace libcamera {
LOG_DEFINE_CATEGORY(af)

namespace ipa::soft::algorithms {

Af::Af()
	: frameCounter(0), wantSharpness(false), lensPos(0), highest(0, 0), stable(false), waitFlag(false), afState(init)
{
}

void Af::prepare([[maybe_unused]] typename Module::Context &context,
		 [[maybe_unused]] const uint32_t frame,
		 [[maybe_unused]] typename Module::FrameContext &frameContext,
		 [[maybe_unused]] typename Module::Params *params)
{

	switch (afState) {
	case init:
		wantSharpness = false;
		break;
	case locked: //Locked
		if(frameCounter < CframeSkip){
			frameCounter++;
			wantSharpness = false;
		} else{
			wantSharpness = true;
			frameCounter = 0;
		}
		break;
	case full: // Full Sweep
		if (!wantSharpness) {
			wantSharpness = true;
			break;
		}
		wantSharpness = false;
		break;
	case small: // small sweep (hill climb)
		if (!wantSharpness) {
			wantSharpness = true;
			break;
		}
		wantSharpness = false;
		break;
	}
	params->wantSharpness = wantSharpness;
}

void Af::process([[maybe_unused]] IPAContext &context, [[maybe_unused]] const uint32_t frame,
		 [[maybe_unused]] IPAFrameContext &frameContext, [[maybe_unused]] const SwIspStats *stats,
		 [[maybe_unused]] ControlList &metadata)
{
	uint64_t sharpness = stats->sharpnessValue_;
	bool hasSharpness = stats->hasSharpness;

	switch (afState) {
	case init:
		initState(context);
		break;
	case locked: //Locked
		lockedState(context, sharpness, hasSharpness);
		break;
	case full: // Full Sweep
		fullSweepState(context, sharpness, hasSharpness);
		break;
	case small: // small sweep (hill climb)
		smallSweepState(context, sharpness, hasSharpness);
		break;
	}
}

void Af::initState([[maybe_unused]] IPAContext &context)
{
	context.activeState.af.focus = 0;
	if (itt < 255) {
		itt++;
	} else {
		itt = 0;
		afState = full; // full sweep
	}
}

void Af::lockedState([[maybe_unused]] IPAContext &context, [[maybe_unused]] uint64_t sharpness, [[maybe_unused]] bool hasSharpness)
{
	if (stable) {
		itt++;
		if (itt >= 20) {
			stable = false;
			itt = 0;
		}
		return;
	}

	if (!hasSharpness)
		return;

	if (sharpnessLock < sharpness) {
		sharpnessLock = sharpness;
		context.activeState.af.sharpnessLock = sharpness;
	}

	else if ((uint64_t)((double)sharpnessLock * 0.6) > sharpness) { // to sweep
		lensPos = 0;
		waitFlag = true;
		context.activeState.af.focus = lensPos;
		afState = full; // full sweep
	} else if ((uint64_t)((double)sharpnessLock * 0.8) > sharpness) { // to smallsweep
		if (lensPos < 200) {
			lensPos = 0;
		} else {
			lensPos = lensPos - 200;
		}
		waitFlag = true;
		context.activeState.af.focus = lensPos;
		itt = 0;
		afState = small; // small sweep
	}
}

void Af::fullSweepState([[maybe_unused]] IPAContext &context, [[maybe_unused]] uint64_t sharpness, [[maybe_unused]] bool hasSharpness)
{
	//in case VCM is moving
	if (waitFlag) {
		itt++;
		if (itt >= 20) {
			waitFlag = false;
			itt = 0;
		}
		return;
	}
	if (!hasSharpness)
		return;

	int32_t focusMax = context.configuration.af.afocusMax;
	if (lensPos < focusMax && highest.second * 0.5 < sharpness) {
		if (sharpness > highest.second) {
			highest = std::make_pair(lensPos, sharpness);
			LOG(af, Info) << "Highest Sharpness: " << highest.second;
			LOG(af, Info) << "Highest focus pos: " << (int32_t)highest.first;
		}
		lensPos += context.configuration.af.stepValue;
		if (lensPos > focusMax)
			lensPos = focusMax;
		context.activeState.af.focus = lensPos;
	} else {
		lensPos = highest.first;
		sharpnessLock = highest.second;
		highest = std::make_pair(0, 0);
		context.activeState.af.sharpnessLock = sharpnessLock;
		context.activeState.af.focus = lensPos;
		stable = true;
		itt = 0;
		afState = locked;
	}
}

void Af::smallSweepState([[maybe_unused]] IPAContext &context, [[maybe_unused]] uint64_t sharpness, [[maybe_unused]] bool hasSharpness)
{
	if (waitFlag) {
		itt++;
		if (itt >= 20) {
			waitFlag = false;
			itt = 0;
		}
		return;
	}

	if (!hasSharpness)
		return;

	if (itt < 400 && highest.second * 0.5 < sharpness) {
		if (sharpness > highest.second) {
			highest = std::make_pair(lensPos, sharpness);
		}
		lensPos++;
		itt++;
		context.activeState.af.focus = lensPos;
	} else {
		lensPos = highest.first;
		sharpnessLock = highest.second;
		highest = std::make_pair(0, 0);
		stable = true;
		itt = 0;
		context.activeState.af.sharpnessLock = sharpnessLock;
		context.activeState.af.focus = lensPos;
		afState = locked;
	}
}

REGISTER_IPA_ALGORITHM(Af, "Af")

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */