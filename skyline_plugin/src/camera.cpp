#include "camera.hpp"

#include <map>

#include "fw/camera.hpp"
#include "fw/input_manager.hpp"
#include "game/behavior_pc.hpp"
#include "plugin.hpp"
#include "stuff/utils/debug_util.hpp"
#include "stuff/utils/hid.hpp"
#include "stuff/utils/util.hpp"

namespace ether::bfsw::camera {

void setDefaultFreeCamOptions(SharedState::Options::FreeCamOptions& freeCamOptions) {
    freeCamOptions.freeCamType = fw::DEBUG_CAMERA::MAYA;
    freeCamOptions.keyMap = {
        .toggle = nn::hid::KEY_MINUS,
        .forward = nn::hid::KEY_R,
        .backward = nn::hid::KEY_L,
        .zoomIn = nn::hid::KEY_DRIGHT,
        .zoomOut = nn::hid::KEY_DLEFT,
        .fovIncrease = nn::hid::KEY_ZL,
        .fovDecrease = nn::hid::KEY_ZR,
        .bankLeft = nn::hid::KEY_NONE,
        .bankRight = nn::hid::KEY_NONE,
        .teleport = nn::hid::KEY_RSTICK,
    };
}

void initFreeCamState(SharedState::FreeCamState& freeCamState) { freeCamState.isToggledOn = false; }

static auto s_npadScanner = util::NpadScanner{.useHandheldStyle = true, .npadId = nn::hid::CONTROLLER_PLAYER_1};

static auto s_freeCamOffset = mm::Vec3{0};

static auto s_teleportRequested = false;
static auto s_teleportPos = mm::Vec3{};

GENERATE_CLASS_HOOK(fw::Camera, setLookAt, mm::Vec3 const& camPos, mm::Vec3 const& lookAtPos, float banking) {
    static constexpr auto DELTA_UNIT = 0.3;

    auto p_sharedState = Plugin::getSharedStatePtr();

    // static auto curIsToggledOn = false;
    auto& freeCamIsToggledOn = p_sharedState->freeCam.isToggledOn;

    if (not freeCamIsToggledOn) {
        return setLookAtBak(p_this, camPos, lookAtPos, banking);
    }

    auto delta = lookAtPos - camPos;
    auto& keyMap = p_sharedState->options.freeCam.keyMap;
    auto curButtons = s_npadScanner.keyState.Buttons;
    if (curButtons & keyMap.forward) {
        s_freeCamOffset += delta * DELTA_UNIT;
    }
    if (curButtons & keyMap.backward) {
        s_freeCamOffset -= delta * DELTA_UNIT;
    }

    auto camPosAdjusted = camPos + s_freeCamOffset;
    auto lookAtPosAdjusted = lookAtPos + s_freeCamOffset;

    if (curButtons & keyMap.teleport) {
        s_teleportPos = camPosAdjusted;
        s_teleportRequested = true;
        freeCamIsToggledOn = false;
    }

    return setLookAtBak(p_this, camPosAdjusted, lookAtPosAdjusted, banking);
}

GENERATE_CLASS_HOOK_NAMED(cameraManagerUpdate, fw::CameraManager, update, fw::Document const& document,
                          fw::UpdateInfo const& updateInfo) {
    auto p_sharedState = Plugin::getSharedStatePtr();
    auto& freeCamOptions = p_sharedState->options.freeCam;

    s_npadScanner.scanInput();

    auto& freeCamIsToggledOn = p_sharedState->freeCam.isToggledOn;
    if (s_npadScanner.keyComboJustPressed(freeCamOptions.keyMap.toggle)) {
        freeCamIsToggledOn = !freeCamIsToggledOn;
    }

    if (freeCamIsToggledOn) {
        if (p_this->debugCameraType == fw::DEBUG_CAMERA::NONE) {
            p_this->setDebugCamera(freeCamOptions.freeCamType);
        }
    } else {
        if (p_this->debugCameraType != fw::DEBUG_CAMERA::NONE) {
            p_this->setDebugCamera(fw::DEBUG_CAMERA::NONE);
            s_freeCamOffset = {0};
        }
    }

    return cameraManagerUpdateBak(p_this, document, updateInfo);
}

GENERATE_CLASS_HOOK_NAMED(behaviorPcUpdate, game::BehaviorPc, update, fw::UpdateInfo const& updateInfo) {
    if (p_this->getFormationIdx() == 0) {
        if (s_teleportRequested) {
            auto warpSetting = game::BehaviorPc::WarpSettings{0};
            warpSetting.pos = s_teleportPos;
            p_this->setWarp(warpSetting);
            s_teleportRequested = false;
        }
    }
    return behaviorPcUpdateBak(p_this, updateInfo);
}

GENERATE_CLASS_HOOK(fw::InputManager, testKeyBoardStatus, fw::InputStatus inputStatus, char const* keyStr) {
    static auto& keyMap = Plugin::getSharedStatePtr()->options.freeCam.keyMap;
    static const auto KB_TO_PAD_MAP = std::map<std::string, HidControllerKeyData&>{
        {"up", keyMap.zoomIn},        {"down", keyMap.zoomOut},  {"right", keyMap.fovIncrease},
        {"left", keyMap.fovDecrease}, {"\x81", keyMap.bankLeft}, {"\x83", keyMap.bankRight},
    };

    if (inputStatus == fw::InputStatus::HELD and strcmp(keyStr, "alt") == 0) {
        // ignore alt hold check
        return true;
    }

    if (inputStatus == fw::InputStatus::DOWN) {
        auto curKeyFoundEntry = KB_TO_PAD_MAP.find(keyStr);
        if (curKeyFoundEntry == end(KB_TO_PAD_MAP)) {
            return false;
        }

        return s_npadScanner.keyState.Buttons & curKeyFoundEntry->second;
    }

    return false;
}

static const char* s_curMouseKey;
GENERATE_CLASS_HOOK(fw::InputManager, testMouseStatus, fw::InputStatus inputStatus, char const* keyStr) {
    if (inputStatus == fw::InputStatus::HELD and strcmp(keyStr, s_curMouseKey) == 0) {
        return true;
    }
    return false;
}

static auto s_curMousePoint = mm::Pnt{};
GENERATE_CLASS_HOOK(fw::InputManager, getMouseMovement) { return &s_curMousePoint; }

GENERATE_CLASS_HOOK_NAMED(ctrlMayaCamUpdate, fw::CtrlMayaCam, update, fw::DebugCamInfo& debugCamInfo,
                          fw::Document const& document, fw::UpdateInfo const&) {
    static constexpr auto MOUSE_POINT_SENSITIVITY_ADJUST_PAN = 5000;
    static constexpr auto MOUSE_POINT_SENSITIVITY_ADJUST_DRAG = 512;

    auto& inputManager = document.getInputManager();
    p_this->moveKeyboard(debugCamInfo, inputManager);

    s_curMousePoint = {s_npadScanner.keyState.RStickX / MOUSE_POINT_SENSITIVITY_ADJUST_PAN,
                       -s_npadScanner.keyState.RStickY / MOUSE_POINT_SENSITIVITY_ADJUST_PAN};
    s_curMouseKey = "left";
    p_this->moveMouse(debugCamInfo, inputManager);

    s_curMousePoint = {-s_npadScanner.keyState.LStickX / MOUSE_POINT_SENSITIVITY_ADJUST_DRAG,
                       s_npadScanner.keyState.LStickY / MOUSE_POINT_SENSITIVITY_ADJUST_DRAG};
    s_curMouseKey = "center";
    p_this->moveMouse(debugCamInfo, inputManager);
}

void hook() {
    // camera pos offset
    setLookAtHook();

    // free cam toggle
    cameraManagerUpdateHook();

    // teleport
    behaviorPcUpdateHook();

    // simulate inputs
    testKeyBoardStatusHook();
    testMouseStatusHook();
    getMouseMovementHook();

    // custom update
    ctrlMayaCamUpdateHook();
}

}  // namespace ether::bfsw::camera
