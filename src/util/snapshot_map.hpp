#pragma once

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
    /**
     * Insert or assign a value to the map
     * @param key The key to insert/assign
     * @param value The value to store
     */
    void insert_or_assign(const std::string& key, const T& value);

    /**
     * Remove a key from the map
     * @param key The key to remove
     */
    void erase(const std::string& key);

    /**
     * Get a value from the map
     * @param key The key to look up
     * @return Optional containing the value if found, nullopt otherwise
     */
    std::optional<T> get(const std::string& key) const;

    // Forward declaration
    class Snapshot;

    /**
     * Create a snapshot of the current map structure
     * @return A Snapshot object that can be iterated over
     */
    Snapshot snapshot() const;

    // Snapshot object that can be iterated over
    class Snapshot {
    public:
        struct Iterator {
            using iterator_category = std::input_iterator_tag;
            using value_type = std::pair<std::string, T>;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;

            Iterator(const Snapshot* snap, size_t pos);

            value_type operator*() const;
            Iterator& operator++();
            bool operator==(const Iterator& other) const;
            bool operator!=(const Iterator& other) const;

        private:
            void advance_to_valid();

            const Snapshot* snap_;
            size_t pos_;
        };

        Iterator begin() const;
        Iterator end() const;

    private:
        friend class SnapshotMap;
        Snapshot(std::vector<std::string> keys, const SnapshotMap& parent);

        std::vector<std::string> keys_;
        const SnapshotMap& parent_;
    };

private:
    mutable std::shared_mutex mutex_;
    std::map<std::string, std::shared_ptr<T>> map_;
};

// Template method implementations
template <typename T>
void SnapshotMap<T>::insert_or_assign(const std::string& key, const T& value) {
    std::unique_lock lock(mutex_);
    map_[key] = std::make_shared<T>(value);
}

template <typename T>
void SnapshotMap<T>::erase(const std::string& key) {
    std::unique_lock lock(mutex_);
    map_.erase(key);
}

template <typename T>
std::optional<T> SnapshotMap<T>::get(const std::string& key) const {
    std::shared_lock lock(mutex_);
    auto it = map_.find(key);
    if (it != map_.end()) {
        return *(it->second);
    }
    return std::nullopt;
}

template <typename T>
typename SnapshotMap<T>::Snapshot SnapshotMap<T>::snapshot() const {
    std::shared_lock lock(mutex_);
    std::vector<std::string> keys;
    keys.reserve(map_.size());
    for (auto& [k, _] : map_) {
        keys.push_back(k);
    }
    return Snapshot(std::move(keys), *this);
}

// Snapshot::Iterator implementation
template <typename T>
SnapshotMap<T>::Snapshot::Iterator::Iterator(const Snapshot* snap, size_t pos) 
    : snap_(snap), pos_(pos) {
    advance_to_valid();
}

template <typename T>
typename SnapshotMap<T>::Snapshot::Iterator::value_type 
SnapshotMap<T>::Snapshot::Iterator::operator*() const {
    // Capture value at the moment this key-value pair is accessed
    auto val = snap_->parent_.get(snap_->keys_[pos_]);
    if (!val) {
        throw std::runtime_error("Invalid key in snapshot iterator");
    }
    return {snap_->keys_[pos_], *val};
}

template <typename T>
typename SnapshotMap<T>::Snapshot::Iterator& 
SnapshotMap<T>::Snapshot::Iterator::operator++() {
    ++pos_;
    advance_to_valid();
    return *this;
}

template <typename T>
bool SnapshotMap<T>::Snapshot::Iterator::operator==(const Iterator& other) const {
    return pos_ == other.pos_ && snap_ == other.snap_;
}

template <typename T>
bool SnapshotMap<T>::Snapshot::Iterator::operator!=(const Iterator& other) const {
    return !(*this == other);
}

template <typename T>
void SnapshotMap<T>::Snapshot::Iterator::advance_to_valid() {
    while (pos_ < snap_->keys_.size()) {
        if (snap_->parent_.get(snap_->keys_[pos_])) {
            // found a valid key/value
            break;
        }
        ++pos_;
    }
}

// Snapshot implementation
template <typename T>
typename SnapshotMap<T>::Snapshot::Iterator 
SnapshotMap<T>::Snapshot::begin() const { 
    return Iterator(this, 0); 
}

template <typename T>
typename SnapshotMap<T>::Snapshot::Iterator 
SnapshotMap<T>::Snapshot::end() const { 
    return Iterator(this, keys_.size()); 
}

template <typename T>
SnapshotMap<T>::Snapshot::Snapshot(std::vector<std::string> keys,
                                   const SnapshotMap& parent)
    : keys_(std::move(keys)), parent_(parent) {}
