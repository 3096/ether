#pragma once

#include "bgm.hpp"
#include "camera.hpp"
#include "plugin.hpp"
#include "skyline/efl/service.hpp"

namespace ether::bfsw {

void pluginMain() {
    Plugin::init();

    bgm::hook();
    camera::hook();

    // R_ERRORONFAIL(skyline::efl::RegisterPlugin(Plugin::getPluginMeta()));
    // R_ERRORONFAIL(skyline::efl::RegisterSharedMem(Plugin::PLUGIN_NAME, Plugin::getPluginSharedMemInfo()));
}

}  // namespace ether::bfsw
