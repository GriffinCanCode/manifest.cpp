#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <stack>
#include <type_traits>
#include <vector>

namespace Manifest {
namespace Core {
namespace Memory {

template <typename T>
class Pool {
    static_assert(std::is_destructible<T>::value, "T must be destructible");
    static_assert(alignof(T) <= alignof(std::max_align_t), "T alignment too large");
    struct Block {
        alignas(T) std::byte data[sizeof(T)];
    };

    std::vector<std::unique_ptr<Block[]>> chunks_;
    std::stack<Block*> available_;
    std::size_t chunk_size_;
    std::size_t total_capacity_{};
    std::size_t used_count_{};

    void allocate_chunk() {
        auto chunk = std::make_unique<Block[]>(chunk_size_);
        Block* raw_chunk = chunk.get();

        // Add all blocks to available stack
        for (std::size_t i = 0; i < chunk_size_; ++i) {
            available_.push(&raw_chunk[i]);
        }

        total_capacity_ += chunk_size_;
        chunks_.emplace_back(std::move(chunk));
    }

   public:
    explicit Pool(std::size_t initial_chunk_size = 1024) : chunk_size_{initial_chunk_size} {
        allocate_chunk();
    }

    ~Pool() = default;
    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;
    Pool(Pool&&) = default;
    Pool& operator=(Pool&&) = default;

    template <typename... Args>
    T* acquire(Args&&... args) {
        if (available_.empty()) {
            allocate_chunk();
        }

        Block* block = available_.top();
        available_.pop();
        ++used_count_;

        return std::construct_at(reinterpret_cast<T*>(block), std::forward<Args>(args)...);
    }

    void release(T* ptr) noexcept {
        if (!ptr) return;

        std::destroy_at(ptr);
        available_.push(reinterpret_cast<Block*>(ptr));
        --used_count_;
    }

    [[nodiscard]] std::size_t capacity() const noexcept { return total_capacity_; }
    [[nodiscard]] std::size_t used() const noexcept { return used_count_; }
    [[nodiscard]] std::size_t available() const noexcept { return available_.size(); }
    [[nodiscard]] double usage_ratio() const noexcept {
        return total_capacity_ > 0 ? static_cast<double>(used_count_) / total_capacity_ : 0.0;
    }
};

template <Poolable T>
class PoolPtr {
    Pool<T>* pool_{};
    T* ptr_{};

   public:
    PoolPtr() = default;

    PoolPtr(Pool<T>* pool, T* ptr) noexcept : pool_{pool}, ptr_{ptr} {}

    ~PoolPtr() {
        if (pool_ && ptr_) {
            pool_->release(ptr_);
        }
    }

    PoolPtr(const PoolPtr&) = delete;
    PoolPtr& operator=(const PoolPtr&) = delete;

    PoolPtr(PoolPtr&& other) noexcept : pool_{other.pool_}, ptr_{other.ptr_} {
        other.pool_ = nullptr;
        other.ptr_ = nullptr;
    }

    PoolPtr& operator=(PoolPtr&& other) noexcept {
        if (this != &other) {
            if (pool_ && ptr_) {
                pool_->release(ptr_);
            }
            pool_ = other.pool_;
            ptr_ = other.ptr_;
            other.pool_ = nullptr;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    T* get() const noexcept { return ptr_; }
    T& operator*() const noexcept { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }

    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    T* release() noexcept {
        T* result = ptr_;
        ptr_ = nullptr;
        pool_ = nullptr;
        return result;
    }
};

template <Poolable T>
class PoolAllocator {
    Pool<T>* pool_;

   public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template <typename U>
    struct rebind {
        using other = PoolAllocator<U>;
    };

    explicit PoolAllocator(Pool<T>& pool) noexcept : pool_{&pool} {}

    template <typename U>
    PoolAllocator(const PoolAllocator<U>& other) noexcept
        : pool_{static_cast<Pool<T>*>(other.pool_)} {}

    pointer allocate(size_type n) {
        if (n != 1) {
            throw std::bad_alloc{};  // Pool only supports single object allocation
        }
        return pool_->acquire();
    }

    void deallocate(pointer p, size_type) noexcept { pool_->release(p); }

    template <typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        std::construct_at(p, std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* p) noexcept {
        std::destroy_at(p);
    }

    bool operator==(const PoolAllocator& other) const noexcept { return pool_ == other.pool_; }

    bool operator!=(const PoolAllocator& other) const noexcept { return !(*this == other); }
};

}  // namespace Memory
}  // namespace Core
}  // namespace Manifest
