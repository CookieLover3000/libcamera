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
    uint8_t value;
};

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */