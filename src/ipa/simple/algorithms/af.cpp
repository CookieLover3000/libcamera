#include "af.h"

namespace libcamera {

namespace ipa::soft::algorithms {

Af::Af() {}

void Af::process([[maybe_unused]] IPAContext &context, [[maybe_unused]] const uint32_t frame,
                [[maybe_unused]] IPAFrameContext &frameContext, [[maybe_unused]] const SwIspStats *stats,
                [[maybe_unused]] ControlList &metadata) {
    context.activeState.af.lensPos = 100;
};

REGISTER_IPA_ALGORITHM(Af, "Af")

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */