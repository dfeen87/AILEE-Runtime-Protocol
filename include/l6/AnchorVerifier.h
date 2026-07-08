#pragma once

#include "l6/AnchorSerializer.h"
#include "l6/StateRootBuilder.h"

namespace ailee {
namespace l6 {

class AnchorVerifier {
public:
    bool verify(const AnchorRecord& rec, const AnchorState& current) const;
};

} // namespace l6
} // namespace ailee
