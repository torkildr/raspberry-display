#include <cstddef>
#include <map>
#include <mutex>
#include <string>
#include <shared_mutex>
#include <vector>
#include <memory>
#include <optional>
#include <iterator>
#include <stdexcept>

template <typename T>
class SnapshotMap {
public:
    void insert_or_assign(const std::string& key, const T& value) {
        std::unique_lock lock(mutex_);
        map_[key] = std::make_shared<T>(value);
    }

    void erase(const std::string& key) {
        std::unique_lock lock(mutex_);
        map_.erase(key);
    }

    std::optional<T> get(const std::string& key) const {
        std::shared_lock lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            return *(it->second);
        }
        return std::nullopt;
    }

    size_t size() const {
        std::shared_lock lock(mutex_);
        return map_.size();
    }

    bool empty() const {
        return size() == 0;
    }

    void clear() {
        std::shared_lock lock(mutex_);
        map_.clear();
    }

    // Snapshot object that can be iterated over
    class Snapshot {
    public:
        struct Iterator {
            using iterator_category = std::input_iterator_tag;
            using value_type = std::pair<std::string, T>;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;

            Iterator(const Snapshot* snap, size_t pos) 
                : snap_(snap), pos_(pos) {
                advance_to_valid();
            }

            value_type operator*() const {
                if (pos_ >= snap_->keys_.size()) {
                    throw std::runtime_error("Cannot dereference end iterator");
                }
                auto val = snap_->parent_.get(snap_->keys_[pos_]);
                if (!val) {
                    throw std::runtime_error("Cannot dereference iterator pointing to erased element");
                }
                return {snap_->keys_[pos_], *val};
            }

            Iterator& operator++() {
                ++pos_;
                advance_to_valid();
                return *this;
            }

            bool operator==(const Iterator& other) const {
                return pos_ == other.pos_ && snap_ == other.snap_;
            }
            bool operator!=(const Iterator& other) const {
                return !(*this == other);
            }

        private:
            void advance_to_valid() {
                while (pos_ < snap_->keys_.size()) {
                    if (snap_->parent_.get(snap_->keys_[pos_])) {
                        break; // found a valid key/value
                    }
                    ++pos_;
                }
            }

            const Snapshot* snap_;
            size_t pos_;
        };

        Iterator begin() const { return Iterator(this, 0); }
        Iterator end() const { return Iterator(this, keys_.size()); }

    private:
        friend class SnapshotMap;
        Snapshot(std::vector<std::string> keys,
                 const SnapshotMap& parent)
            : keys_(std::move(keys)), parent_(parent) {}

        std::vector<std::string> keys_;
        const SnapshotMap& parent_;
    };

    Snapshot snapshot() const {
        std::shared_lock lock(mutex_);
        std::vector<std::string> keys;
        keys.reserve(map_.size());
        for (auto& [k, _] : map_) {
            keys.push_back(k);
        }
        return Snapshot(std::move(keys), *this);
    }

private:
    mutable std::shared_mutex mutex_;
    std::map<std::string, std::shared_ptr<T>> map_;
};
