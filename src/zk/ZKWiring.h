#pragma once

#include "ZKReplayCircuit.h"
#include "../anchor/AnchorRecord.h"

namespace ailee {
namespace zk {

ReplayProofArtifact run_full_v18_pipeline(const anchor::AnchorRecord& record);

} // namespace zk
} // namespace ailee
