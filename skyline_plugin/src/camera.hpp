#pragma once

#include "shared_state.hpp"

namespace ether::bfsw::camera {

void setDefaultFreeCamOptions(SharedState::Options::FreeCamOptions& freeCamOptions);
void initFreeCamState(SharedState::FreeCamState& freeCamState);
void hook();

}  // namespace ether::bfsw::camera
