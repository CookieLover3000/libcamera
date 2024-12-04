#include "af.h"

#include <libcamera/base/log.h>

namespace libcamera {

namespace ipa::soft::algorithms {

Af::Af() : value(0) {}

void Af::process([[maybe_unused]] IPAContext &context, [[maybe_unused]] const uint32_t frame,
                [[maybe_unused]] IPAFrameContext &frameContext, [[maybe_unused]] const SwIspStats *stats,
                [[maybe_unused]] ControlList &metadata) {
    //if(context.activeState.af.state == 1) { //sweep
    //}
    context.activeState.af.lensPos = value;
    if (context.activeState.af.state == 0) {
        if (value < 255) {
            values[value] = rand();
            context.activeState.af.value = values[value];
            value++;
        } else { context.activeState.af.state = 1; }
    } else if (context.activeState.af.state == 1) {
        // sweep done now
    }
    
    
};

REGISTER_IPA_ALGORITHM(Af, "Af")

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */