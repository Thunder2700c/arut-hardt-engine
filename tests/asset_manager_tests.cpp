#include "arut/core/asset_manager.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {
class MemoryAssets final : public arut::core::AssetSource {
public:
    std::unordered_map<std::string, arut::core::AssetBytes> files;
    bool read(std::string_view path, arut::core::AssetBytes& output) const override {
        const auto it = files.find(std::string(path));
        if (it == files.end()) return false;
        output = it->second;
        return true;
    }
};
}

int main() {
    using namespace arut::core;
    auto source = std::make_shared<MemoryAssets>();
    source->files.emplace("textures/albedo.bin", AssetBytes{1U, 2U, 3U, 4U});
    source->files.emplace("empty.bin", AssetBytes{});
    source->files.emplace("large.bin", AssetBytes{1U, 2U, 3U, 4U, 5U});
    AssetManager assets(source, 4U);

    const auto first = assets.load("textures/albedo.bin");
    assert(first && first.bytes->size() == 4U);
    const auto second = assets.load("textures/albedo.bin");
    assert(second && first.bytes == second.bytes); // Cache returns shared immutable storage.
    assert(assets.stats().hits == 1U && assets.stats().misses == 1U);

    assert(assets.load("missing.bin").error == AssetError::NotFound);
    assert(assets.load("empty.bin").error == AssetError::EmptyAsset);
    assert(assets.load("large.bin").error == AssetError::AssetTooLarge);

    assert(assets.load("../secret").error == AssetError::InvalidPath);
    assert(assets.load("/absolute").error == AssetError::InvalidPath);
    assert(assets.load("textures\\\\escape").error == AssetError::InvalidPath);
    assert(assets.load("textures//double").error == AssetError::InvalidPath);

    assert(assets.evict("textures/albedo.bin"));
    assert(!assets.evict("textures/albedo.bin"));
    const auto after_evict = assets.load("textures/albedo.bin");
    assert(after_evict && after_evict.bytes != first.bytes);
    assert(assets.stats().entries == 1U);
    assert(assets.stats().bytes == 4U);
    assets.clear();
    assert(assets.stats().entries == 0U && assets.stats().bytes == 0U);
}
