#include "af.h"

#include <libcamera/base/log.h>

namespace libcamera {

namespace ipa::soft::algorithms {

Af::Af() : value(0) {}

void Af::process([[maybe_unused]] IPAContext &context, [[maybe_unused]] const uint32_t frame,
                [[maybe_unused]] IPAFrameContext &frameContext, [[maybe_unused]] const SwIspStats *stats,
                [[maybe_unused]] ControlList &metadata) {
    if (context.activeState.af.state == 0) {
        if (value < 255) { // TODO: CHANGE 255 TO DYNAMIC VALUE
            values[value] = stats->sharpnessValue_;
            context.activeState.af.value = values[value];
            value++;
        } else {
            std::pair<uint8_t, uint8_t> highest;
            std::map<uint8_t, uint8_t>::iterator currentEntry;
            for (currentEntry = values.begin(); currentEntry != values.end(); ++currentEntry) {
                if (currentEntry->second > highest.second) {
                    highest = std::make_pair(currentEntry->first, currentEntry->second);
                    context.activeState.af.lensPos = highest.first;
                    sharp = highest.second;
                }
            }
            context.activeState.af.state = 1;
        }
    } else if (context.activeState.af.state == 1) { //locked
        if (sharp < stats->sharpnessValue_) {
            value = 0;
            context.activeState.af.state = 0;
        }
    }
};

REGISTER_IPA_ALGORITHM(Af, "Af")

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */