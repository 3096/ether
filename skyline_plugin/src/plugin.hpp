#pragma once

#include "shared_state.hpp"
#include "skyline/efl/service.hpp"
#include "skyline/nx/kernel/shmem.h"

namespace ether::bfsw {

class Plugin {
   private:
    Plugin();
    Plugin(const Plugin&) = delete;
    ~Plugin();
    static inline auto& getInstance() {
        static Plugin s_instance;
        return s_instance;
    }

    static constexpr auto SHARED_MEM_PERM = Perm_Rw;

    SlPluginMeta m_pluginMeta;

    SharedMemory m_sharedMemStruct;
    SharedState* mp_sharedState;

    void setDefaultOptions_();
    void sharedStateInit_();

   public:
    static constexpr auto PLUGIN_NAME = "ether";
    static constexpr auto PLUGIN_VERSION = 0;
    static constexpr auto API_VERSION = 0;

    static inline auto init() { getInstance(); }
    static inline auto getPluginMeta() { return getInstance().m_pluginMeta; }
    static inline auto getPluginSharedMemInfo() {
        auto sharedMemStruct = getInstance().m_sharedMemStruct;
        return SlPluginSharedMemInfo{sharedMemStruct.handle, sharedMemStruct.size, sharedMemStruct.perm};
    }
    static inline auto getSharedStatePtr() { return getInstance().mp_sharedState; }
};

}  // namespace ether::bfsw
