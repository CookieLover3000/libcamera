#include "af.h"

#include <libcamera/base/log.h>

namespace libcamera {

namespace ipa::soft::algorithms {

Af::Af() : lensPos(0) {}

void Af::process([[maybe_unused]] IPAContext &context, [[maybe_unused]] const uint32_t frame,
                [[maybe_unused]] IPAFrameContext &frameContext, [[maybe_unused]] const SwIspStats *stats,
                [[maybe_unused]] ControlList &metadata) {
    switch (context.activeState.af.state) {
        case 0:     // Full Sweep
            if (lensPos < 255) { // TODO: CHANGE 255 TO DYNAMIC VALUE
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
                        sharpnessLock = highest.second;
                    }
                }
                context.activeState.af.state = 2;
            }
            break;
        case 1:     // small sweep (hill climb)
            if (itt < 100) {
                values[lensPos] = stats->sharpnessValue_;
                context.activeState.af.sharpnessLock = values[lensPos];
                context.activeState.af.focus = lensPos++;
                itt++;
            } else {
                context.activeState.af.state = 2;
            }
            break;
        case 2:     //Locked
            if (sharpnessLock*0.6 > stats->sharpnessValue_) {   // to smallsweep
                lensPos = 0;
                context.activeState.af.focus = lensPos;
                context.activeState.af.state = 0;
            } else if (sharpnessLock*0.8 > stats->sharpnessValue_) {    // to sweep
                if (lensPos < 50) { lensPos = 0; }
                else lensPos = lensPos - 50;
                context.activeState.af.focus = lensPos;
                itt = 0;
                context.activeState.af.state = 1;
            }
            break;
    }
};

REGISTER_IPA_ALGORITHM(Af, "Af")

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */