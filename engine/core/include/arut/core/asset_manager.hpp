#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace arut::core {

using AssetBytes = std::vector<std::uint8_t>;

class AssetSource {
public:
    virtual ~AssetSource() = default;
    // Implementations must replace output only on success.
    [[nodiscard]] virtual bool read(std::string_view normalized_path,
                                    AssetBytes& output) const = 0;
};

enum class AssetError : std::uint8_t {
    None,
    InvalidPath,
    NotFound,
    EmptyAsset,
    AssetTooLarge,
};

struct AssetLoadResult final {
    AssetError error{AssetError::None};
    std::shared_ptr<const AssetBytes> bytes{};
    [[nodiscard]] explicit operator bool() const noexcept {
        return error == AssetError::None && static_cast<bool>(bytes);
    }
};

struct AssetCacheStats final {
    std::size_t entries{0};
    std::size_t bytes{0};
    std::uint64_t hits{0};
    std::uint64_t misses{0};
};

// Thread-safe, immutable asset cache. The source owns platform-specific I/O;
// the manager owns normalized names, size limits and cache lifetime.
class AssetManager final {
public:
    static constexpr std::size_t default_max_asset_bytes = 64U * 1024U * 1024U;

    explicit AssetManager(std::shared_ptr<const AssetSource> source,
                          std::size_t max_asset_bytes = default_max_asset_bytes)
        : source_(std::move(source)), max_asset_bytes_(max_asset_bytes) {}

    [[nodiscard]] AssetLoadResult load(std::string_view path) {
        std::string normalized;
        if (!normalize(path, normalized)) return {AssetError::InvalidPath, {}};
        std::lock_guard<std::mutex> lock(mutex_);
        const auto cached = cache_.find(normalized);
        if (cached != cache_.end()) {
            ++hits_;
            return {AssetError::None, cached->second};
        }
        ++misses_;
        if (!source_) return {AssetError::NotFound, {}};
        AssetBytes data;
        if (!source_->read(normalized, data)) return {AssetError::NotFound, {}};
        if (data.empty()) return {AssetError::EmptyAsset, {}};
        if (data.size() > max_asset_bytes_) return {AssetError::AssetTooLarge, {}};
        auto immutable = std::make_shared<const AssetBytes>(std::move(data));
        cache_bytes_ += immutable->size();
        cache_.emplace(std::move(normalized), immutable);
        return {AssetError::None, std::move(immutable)};
    }

    [[nodiscard]] bool evict(std::string_view path) {
        std::string normalized;
        if (!normalize(path, normalized)) return false;
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = cache_.find(normalized);
        if (found == cache_.end()) return false;
        cache_bytes_ -= found->second->size();
        cache_.erase(found);
        return true;
    }

    void clear() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        cache_bytes_ = 0;
    }

    [[nodiscard]] AssetCacheStats stats() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return {cache_.size(), cache_bytes_, hits_, misses_};
    }

    [[nodiscard]] static bool normalize(std::string_view input,
                                        std::string& output) {
        output.clear();
        if (input.empty() || input.front() == '/' ||
            input.find('\\\\') != std::string_view::npos ||
            input.find(':') != std::string_view::npos) return false;
        std::size_t start = 0;
        while (start < input.size()) {
            const auto end = input.find('/', start);
            const auto length = (end == std::string_view::npos ? input.size() : end) - start;
            const auto part = input.substr(start, length);
            if (part.empty() || part == "." || part == "..") return false;
            if (!output.empty()) output.push_back('/');
            output.append(part);
            if (end == std::string_view::npos) break;
            start = end + 1U;
            if (start == input.size()) return false;
        }
        return !output.empty();
    }

private:
    std::shared_ptr<const AssetSource> source_;
    const std::size_t max_asset_bytes_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<const AssetBytes>> cache_;
    std::size_t cache_bytes_{0};
    std::uint64_t hits_{0};
    std::uint64_t misses_{0};
};

} // namespace arut::core
