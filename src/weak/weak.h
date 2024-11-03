#pragma once

#include "../shared/shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
private:
    T* ptr_ = nullptr;
    IBlock* ctrl_block_ = nullptr;

    template <typename Y>
    friend class SharedPtr;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() : ptr_{nullptr}, ctrl_block_{nullptr} {};

    WeakPtr(const WeakPtr& other) {
        ptr_ = other.ptr_;
        ctrl_block_ = other.ctrl_block_;

        if (ctrl_block_ != nullptr) {
            ctrl_block_->IncWeak();
        }
    }

    WeakPtr(WeakPtr&& other) {
        ptr_ = other.ptr_;
        ctrl_block_ = other.ctrl_block_;

        other.ptr_ = nullptr;
        other.ctrl_block_ = nullptr;
    };

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) {
        ptr_ = other.ptr_;
        ctrl_block_ = other.ctrl_block_;

        if (ctrl_block_ != nullptr) {
            ctrl_block_->IncWeak();
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (other.ptr_ == ptr_) {
            return *this;
        }

        if (ctrl_block_ != nullptr) {
            ctrl_block_->DecWeak();
        }

        ptr_ = other.ptr_;
        ctrl_block_ = other.ctrl_block_;

        if (ctrl_block_ != nullptr) {
            ctrl_block_->IncWeak();
        }

        return *this;
    };

    WeakPtr& operator=(WeakPtr&& other) {
        if (other.ptr_ == ptr_) {
            return *this;
        }

        if (ctrl_block_ != nullptr) {
            ctrl_block_->DecWeak();
        }

        ptr_ = other.ptr_;
        ctrl_block_ = other.ctrl_block_;

        other.ptr_ = nullptr;
        other.ctrl_block_ = nullptr;

        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        Reset();
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (ctrl_block_ != nullptr) {
            ctrl_block_->DecWeak();
        }

        ptr_ = nullptr;
        ctrl_block_ = nullptr;
    };

    template <typename Y>
    void Swap(WeakPtr<Y>& other) {
        std::swap(ctrl_block_, other.ctrl_block_);
        std::swap(ptr_, other.ptr_);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (ctrl_block_ != nullptr) {
            return ctrl_block_->SharedCount();
        }
        return 0;
    };

    bool Expired() const {
        return ctrl_block_ == nullptr || UseCount() == 0;
    };

    SharedPtr<T> Lock() const {
        return Expired() ? SharedPtr<T>() : SharedPtr<T>(*this);
    };
};
