#pragma once

#include "sw_fwd.h"  // Forward declaration
#include <cstddef>   // std::nullptr_t
#include <type_traits>

// the base class for Enable Shared From This
class ESFTBase {};


// Control Blocks
// Base class for other control blocks
class IBlock {
private:
    size_t shared_count_ = 1;
    size_t weak_count_ = 0;

    virtual void Deleter() {};

public:
    IBlock() {
        shared_count_ = 1;
        weak_count_ = 0;
    }
    // api for shared obj refs

    size_t SharedCount() {
        return shared_count_;
    }

    void IncShared() {
        ++shared_count_;
    }

    void DecShared() {
        // std::cout << "DecShared() strong: " << shared_count_ << ", weak: " << weak_count_ <<
        // "\n";
        --shared_count_;
        if (shared_count_ == 0) {
            // std::cout << "DecShared Deleter()\n";
            Deleter();
            // if (WeakCount() == 0) {
            //     std::cout << "Deleting object: " << this << std::endl;
            //     delete this;
            // }
        }
    }

    // api for weak obj refs
    size_t WeakCount() {
        return weak_count_;
    }

    void IncWeak() {
        weak_count_++;
    }

    void DecWeak() {
        weak_count_--;
        // std::cout << "DecWeak() strong: " << shared_count_ << ", weak: " << weak_count_ << "\n";
        if (shared_count_ == 0 && WeakCount() == 0) {
            // std::cout << "Deleting object: " << this << std::endl;
            delete this;
        }
    }

    // virtual methods
    virtual ~IBlock(){};
};

// Control Block for shared_ptr(T* ptr)
template <typename T>
class RawPtrBlock : public IBlock {
private:
    void Deleter() override {
        delete ptr_;
    }

public:
    T* ptr_;

    RawPtrBlock(T* ptr) : IBlock(), ptr_{ptr} {};

    ~RawPtrBlock() override {
        // std::cout << "~RawPtrBlock()\n";
    }
};

// Control Block for make_shared(Args&&...)
template <typename T>
class SingleAllocateBlock : public IBlock {
private:
    void Deleter() override {
        reinterpret_cast<T*>(ptr_)->~T();
    };

public:
    char bytes_[sizeof(T)] = {};
    T* ptr_ = nullptr;

    SingleAllocateBlock() : IBlock() {
        ptr_ = new (bytes_) T();
    }

    template <typename... Args>
    SingleAllocateBlock(Args&&... args) : IBlock() {
        ptr_ = new (bytes_) T(std::forward<Args>(args)...);
    }

    ~SingleAllocateBlock() override {
    }
};

template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() {
        ptr_ = nullptr;
        ctrl_block_ = nullptr;
    };

    SharedPtr(std::nullptr_t) {
        ptr_ = nullptr;
        ctrl_block_ = nullptr;
    };

    template <typename Y>
    SharedPtr(SingleAllocateBlock<Y>* ctrl_block)
        : ptr_{ctrl_block->ptr_}, ctrl_block_{ctrl_block} {
        // std::cout << ptr_ << ", " << ctrl_block_ << "\n";
        // std::cout << ctrl_block_->SharedCount() << "\n";
        if constexpr (std::is_convertible_v<Y*, ESFTBase*>) {
            ptr_->weak_this_ = WeakPtr<T>(*this);
            // ctrl_block_->IncWeak();
        }
        // std::cout << ctrl_block_->SharedCount() << ", weak: " << ctrl_block_->WeakCount() <<
        // "\n";
    }

    template <typename Y>
    explicit SharedPtr(Y* ptr) : ptr_{ptr}, ctrl_block_{new RawPtrBlock<Y>(ptr)} {
        // std::cout << "SharedPtr(T* ptr)" << "\n";
        if constexpr (std::is_convertible_v<Y*, ESFTBase*>) {
            // std::cout << "BEFORE WEAK THIS: strong: " << ctrl_block_->SharedCount() << ", weak: "
            // << ctrl_block_->WeakCount() << "\n";
            ptr_->weak_this_ = WeakPtr<T>(*this);
        }
        // std::cout << "--- AFTER WEAK THIS: strong: " << ctrl_block_->SharedCount()     << ",
        // weak: " << ctrl_block_->WeakCount() << "\n";
    };

    // copy contructor for working   SharedPtr<const int> s2 = s1;
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) : ptr_{other.ptr_}, ctrl_block_{other.ctrl_block_} {
        // increment the count of refs on block
        // std::cout << "SharedPtr(const SharedPtr<Y>& other)" << "\n";
        if (ctrl_block_ != nullptr) {
            ctrl_block_->IncShared();
        }
    };

    SharedPtr(const SharedPtr& other) : ptr_{other.ptr_}, ctrl_block_{other.ctrl_block_} {
        // std::cout << "SharedPtr(const SharedPtr& other)" << "\n";
        //  increment the count of refs on block
        if (ctrl_block_ != nullptr) {
            ctrl_block_->IncShared();
        }
    };

    // move
    SharedPtr(SharedPtr&& other) {
        // std::cout << "SharedPtr(SharedPtr&& other)" << "\n";
        std::swap(ptr_, other.ptr_);
        std::swap(ctrl_block_, other.ctrl_block_);
    };

    // move contstructor for working this SharedPtr<const int> s3 = std::move(s1);
    // in operator=(SharedPtr<Y>&& other) use SharedPtr(std::move(other)).Swap(*this);
    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) {
        // std::cout << "SharedPtr(SharedPtr<Y>&& other)" << "\n";
        ptr_ = other.ptr_;
        ctrl_block_ = other.ctrl_block_;

        other.ptr_ = nullptr;
        other.ctrl_block_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        // std::cout << "SharedPtr(SharedPtr<Y>&other, T* ptr)" << "\n";
        ptr_ = ptr;
        ctrl_block_ = other.ctrl_block_;

        if (ctrl_block_ != nullptr) {
            ctrl_block_->IncShared();
        }
    };

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.Expired()) {
            throw BadWeakPtr();
            return;
        }

        ptr_ = other.ptr_;
        ctrl_block_ = other.ctrl_block_;

        if (ctrl_block_ != nullptr) {
            ctrl_block_->IncShared();
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) noexcept {
        SharedPtr<T>(other).Swap(*this);
        return *this;
    };

    template <typename Y, typename = std::enable_if_t<std::is_convertible_v<Y, T>>>
    SharedPtr<T>& operator=(const SharedPtr<Y>& other) noexcept {
        SharedPtr<T>(other).Swap(*this);
        return *this;
    };

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (ctrl_block_ != nullptr) {
            ctrl_block_->DecShared();
        }

        ptr_ = other.ptr_;
        ctrl_block_ = other.ctrl_block_;

        other.ptr_ = nullptr;
        other.ctrl_block_ = nullptr;
        return *this;
    };

    template <typename Y, typename = std::enable_if_t<std::is_convertible_v<Y, T>>>
    SharedPtr<T>& operator=(SharedPtr<Y>&& other) {
        SharedPtr<T>(std::move(other)).Swap(*this);
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        Reset();
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (ctrl_block_ != nullptr) {
            ctrl_block_->IncWeak();
            ctrl_block_->DecShared();
            ctrl_block_->DecWeak();
            if (ctrl_block_->SharedCount() == 0 && ctrl_block_->WeakCount() == 0) {
                delete ctrl_block_;
            }
        }

        ptr_ = nullptr;
        ctrl_block_ = nullptr;
    };

    template <typename Y>
    void Reset(Y* ptr) {
        Reset();
        ptr_ = ptr;
        ctrl_block_ = new RawPtrBlock<Y>(ptr);
    };

    template <typename Y>
    void Swap(SharedPtr<Y>& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(ctrl_block_, other.ctrl_block_);
    };

    void Swap(SharedPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(ctrl_block_, other.ctrl_block_);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_;
    };

    T& operator*() const {
        return *ptr_;
    };

    T* operator->() const {
        return ptr_;
    };

    size_t UseCount() const {
        if (ctrl_block_ != nullptr) {
            return ctrl_block_->SharedCount();
        }
        return 0;
    };

    explicit operator bool() const {
        return ptr_ != nullptr;
    };

private:
    T* ptr_ = nullptr;
    IBlock* ctrl_block_ = nullptr;

    // fiend class
    template <typename Y>
    friend class SharedPtr;

    template <typename U, typename... Args>
    friend SharedPtr<U> MakeShared(Args&&... args);

    template <typename Y>
    friend class WeakPtr;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
};

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    // std::cout << "SharedPtr(ControlBlock)\n";
    return SharedPtr<T>(new SingleAllocateBlock<T>(std::forward<Args>(args)...));
};

template <typename T>
class EnableSharedFromThis : public ESFTBase {
public:
    SharedPtr<T> SharedFromThis() {
        return SharedPtr<T>(weak_this_);
    };
    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr<const T>(weak_this_);
    };

    WeakPtr<T> WeakFromThis() noexcept {
        return WeakPtr<T>(weak_this_);
    };
    WeakPtr<const T> WeakFromThis() const noexcept {
        return WeakPtr<const T>(weak_this_);
    };

    template <class _Up>
    friend class SharedPtr;

    template <typename U, typename... Args>
    friend SharedPtr<U> MakeShared(Args&&... args);

    virtual ~EnableSharedFromThis() {
        // std::cout << "~EnableSharedFromThis\n";
    }

private:
    WeakPtr<T> weak_this_;
};
