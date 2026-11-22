#pragma once

#include "arut/core/asset_manager.hpp"

#include <android/asset_manager.h>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace arut::platform::android {

class AndroidAssetSource final : public core::AssetSource {
public:
    explicit AndroidAssetSource(AAssetManager* manager) noexcept : manager_(manager) {}

    [[nodiscard]] bool read(std::string_view path, core::AssetBytes& output) const override {
        if (!manager_ || path.empty()) return false;
        const std::string owned_path(path);
        AAsset* asset = AAssetManager_open(manager_, owned_path.c_str(), AASSET_MODE_BUFFER);
        if (!asset) return false;

        const off_t length = AAsset_getLength(asset);
        if (length < 0 || static_cast<std::uint64_t>(length) >
                              static_cast<std::uint64_t>(core::AssetManager::default_max_asset_bytes)) {
            AAsset_close(asset);
            return false;
        }

        core::AssetBytes bytes(static_cast<std::size_t>(length));
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            const int count = AAsset_read(asset, bytes.data() + offset, bytes.size() - offset);
            if (count <= 0) {
                AAsset_close(asset);
                return false;
            }
            offset += static_cast<std::size_t>(count);
        }
        AAsset_close(asset);
        output = std::move(bytes);
        return true;
    }

private:
    AAssetManager* manager_{nullptr}; // Borrowed from the Activity; Activity outlives runtime state.
};

} // namespace arut::platform::android
