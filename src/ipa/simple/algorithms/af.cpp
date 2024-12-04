#include "af.h"

namespace libcamera {

namespace ipa::soft::algorithms {

void process(IPAContext &context, const uint32_t frame, IPAFrameContext &frameContext, const SwIspStats *stats, ControlList &metadata) {
    context.activeState.af.lensPos = 100;
};

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */