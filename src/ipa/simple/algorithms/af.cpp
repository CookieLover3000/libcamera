#include "af.h"

namespace libcamera {

namespace ipa::soft::algorithms {

Af::Af() {}

void Af::process(IPAContext &context, const uint32_t frame, IPAFrameContext &frameContext, const SwIspStats *stats, ControlList &metadata) {
    context.activeState.af.lensPos = 100;
};

REGISTER_IPA_ALGORITHM(Af, "Af")

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */