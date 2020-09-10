#include "plugin.hpp"

#include <string.h>

// #include "skyline/logger/Logger.hpp"
#include "camera.hpp"

namespace ether::bfsw {

Plugin::Plugin() {
    strcpy(m_pluginMeta.name, PLUGIN_NAME);
    m_pluginMeta.version = PLUGIN_VERSION;
    m_pluginMeta.apiVersion = API_VERSION;

    R_ERRORONFAIL(shmemCreate(&m_sharedMemStruct, SHARED_MEM_SIZE, SHARED_MEM_PERM, SHARED_MEM_PERM));
    R_ERRORONFAIL(shmemMap(&m_sharedMemStruct));

    mp_sharedState = static_cast<decltype(mp_sharedState)>(m_sharedMemStruct.map_addr);
    setDefaultOptions_();
    sharedStateInit_();
}

Plugin::~Plugin() {}

void Plugin::setDefaultOptions_() {
    mp_sharedState->options.disableVisionBgm = true;

    camera::setDefaultFreeCamOptions(mp_sharedState->options.freeCam);
}

void Plugin::sharedStateInit_() { camera::initFreeCamState(mp_sharedState->freeCam); }

}  // namespace ether::bfsw
