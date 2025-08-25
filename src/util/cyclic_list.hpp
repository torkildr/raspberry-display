#pragma once

#include <memory>
#include <shared_mutex>
#include <string>
#include <atomic>
#include <functional>

template <typename TId, typename TData>
class CyclicList {
public:
    // Forward declaration of Element
    class Element;
    
    // Element class with thread-safe access
    class Element {
    public:
        Element(const TId& id, const TData& data) 
            : id_(id), data_(data), marked_for_deletion_(false) {}
        
        // Thread-safe data access
        TData getData() const {
            std::shared_lock<std::shared_mutex> lock(shared_mutex_);
            return data_;
        }
        
        void setData(const TData& data) {
            std::shared_lock<std::shared_mutex> lock(shared_mutex_);
            data_ = data;
        }
        
        TId getId() const {
            return id_; // id is immutable after construction
        }
        
        // Get next element (cyclic)
        std::shared_ptr<Element> next() const {
            std::shared_lock<std::shared_mutex> lock(*parent_shared_mutex_);
            if (marked_for_deletion_ || !next_) {
                return nullptr;
            }
            
            // Skip deleted elements
            auto current = next_;
            while (current && current->marked_for_deletion_) {
                current = current->next_;
                if (current == next_) break; // Avoid infinite loop
            }
            
            return current && !current->marked_for_deletion_ ? current : nullptr;
        }
        
        bool isMarkedForDeletion() const {
            return marked_for_deletion_.load();
        }
        
    private:
        friend class CyclicList<TId, TData>;
        
        TId id_;
        TData data_;
        std::shared_ptr<Element> next_;
        std::atomic<bool> marked_for_deletion_;
        mutable std::shared_mutex shared_mutex_;
        std::shared_mutex* parent_shared_mutex_;
        
        Element(const TId& id, const TData& data, std::shared_mutex* parent_mutex) 
            : id_(id), data_(data), marked_for_deletion_(false), parent_shared_mutex_(parent_mutex) {}
        
        void markForDeletion() {
            marked_for_deletion_ = true;
        }
    };
    
    CyclicList() : size_(0) {}
    
    // Insert element in sorted order by ID
    std::shared_ptr<Element> insert(const TId& id, const TData& data) {
        std::shared_lock<std::shared_mutex> lock(shared_mutex_);
        
        // Remove existing element with same ID if it exists
        eraseInternal(id);
        
        auto new_element = std::shared_ptr<Element>(new Element(id, data, &shared_mutex_));
        
        if (size_ == 0) {
            // First element - points to itself
            new_element->next_ = new_element;
            first_ = new_element;
        } else {
            // Find insertion point to maintain sorted order
            auto current = first_;
            auto prev = findPrevious(first_, id);
            
            new_element->next_ = prev->next_;
            prev->next_ = new_element;
            
            // Update first_ if new element should be first
            if (id < first_->getId()) {
                first_ = new_element;
            }
        }
        
        size_++;
        return new_element;
    }
    
    // Erase element by ID
    bool erase(const TId& id) {
        std::shared_lock<std::shared_mutex> lock(shared_mutex_);
        return eraseInternal(id);
    }
    
    // Get first element
    std::shared_ptr<Element> first() const {
        std::shared_lock<std::shared_mutex> lock(shared_mutex_);
        if (!first_ || first_->marked_for_deletion_) {
            return findFirstValid();
        }
        return first_;
    }
    
    // Clear all elements
    void clear() {
        std::shared_lock<std::shared_mutex> lock(shared_mutex_);
        if (first_) {
            // Mark all elements for deletion
            auto current = first_;
            do {
                current->markForDeletion();
                current = current->next_;
            } while (current != first_);
        }
        first_.reset();
        size_ = 0;
    }
    
    // Get size (includes elements marked for deletion until cleanup)
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(shared_mutex_);
        return size_;
    }
    
    bool empty() const {
        return size() == 0;
    }
    
    // Iterator-like functionality for compatibility
    template<typename Func>
    void forEach(Func&& func) const {
        std::shared_lock<std::shared_mutex> lock(shared_mutex_);
        if (!first_) return;
        
        auto current = first_;
        do {
            if (!current->marked_for_deletion_) {
                func(current->getId(), current->getData());
            }
            current = current->next_;
        } while (current != first_);
    }
    
    // Get element by ID
    std::shared_ptr<Element> find(const TId& id) const {
        std::shared_lock<std::shared_mutex> lock(shared_mutex_);
        if (!first_) return nullptr;
        
        auto current = first_;
        do {
            if (current->getId() == id && !current->marked_for_deletion_) {
                return current;
            }
            current = current->next_;
        } while (current != first_);
        
        return nullptr;
    }
    
private:
    mutable std::shared_mutex shared_mutex_;
    std::shared_ptr<Element> first_;
    std::atomic<size_t> size_;
    
    // Internal erase without locking (caller must hold lock)
    bool eraseInternal(const TId& id) {
        if (!first_ || size_ == 0) return false;
        
        if (size_ == 1 && first_->getId() == id) {
            first_->markForDeletion();
            first_.reset();
            size_ = 0;
            return true;
        }
        
        // Find the element to delete and its predecessor
        auto current = first_;
        auto prev = current;
        
        // Find the last element (predecessor of first)
        while (prev->next_ != first_) {
            prev = prev->next_;
        }
        
        // Now search for the element to delete
        do {
            if (current->getId() == id) {
                // Found the element to delete
                prev->next_ = current->next_;
                
                // Update first_ if we're deleting the first element
                if (current == first_) {
                    first_ = current->next_;
                }
                
                current->markForDeletion();
                size_--;
                return true;
            }
            prev = current;
            current = current->next_;
        } while (current != first_);
        
        return false;
    }
    
    // Find the element that should come before the given ID in sorted order
    std::shared_ptr<Element> findPrevious(std::shared_ptr<Element> start, const TId& id) const {
        if (!start) return nullptr;
        
        auto current = start;
        auto prev = current;
        
        // Find the last element (to get the previous element in circular list)
        do {
            prev = current;
            current = current->next_;
        } while (current != start);
        
        // Now find the correct insertion point
        current = start;
        do {
            if (current->next_->getId() > id || current->next_ == start) {
                return current;
            }
            current = current->next_;
        } while (current != start);
        
        return prev;
    }
    
    // Find first valid (non-deleted) element
    std::shared_ptr<Element> findFirstValid() const {
        if (!first_) return nullptr;
        
        auto current = first_;
        do {
            if (!current->marked_for_deletion_) {
                return current;
            }
            current = current->next_;
        } while (current != first_);
        
        return nullptr;
    }
};
