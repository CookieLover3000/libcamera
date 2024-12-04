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
    uint32_t lensPos;
    std::map<uint8_t,uint64_t> values;
    uint64_t sharpnessLock;
    uint8_t itt;
};

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */