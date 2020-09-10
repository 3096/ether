#pragma once

#include "stuff/utils/util.hpp"

namespace ether::bfsw::bgm {

GENERATE_SYM_HOOK(getBattleBgmId, "_ZNK4game3BGM14getBattleBGMIDERKNS_14GameControllerE", int, void* thisObj,
                  uint8_t* p_gameController) {
    p_gameController[0x211] = p_gameController[0x211] & ~(1 << 3);  // toggle off vision bit for the bgm function
    return getBattleBgmIdBak(thisObj, p_gameController);
}

void hook() { getBattleBgmIdHook(); }

}  // namespace ether::bfsw::bgm
