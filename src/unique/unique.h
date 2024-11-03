#pragma once

#include "compressed_pair.h"
#include <cstddef>  // std::nullptr_t

template <typename T>
struct Slug {
    Slug() = default;

    template <typename U>
    Slug(Slug<U>&&) {
    }

    template <typename U>
    Slug& operator=(Slug<U>&&) {
        return *this;
    }

    void operator()(T* ptr) {
        delete ptr;
    };
};

template <typename T>
struct Slug<T[]> {
    Slug() = default;
    Slug(Slug&) {
    }

    template <typename U>
    Slug(Slug<U>&&) {
    }

    template <typename U>
    Slug& operator=(Slug<U>&&) {
        return *this;
    }

    void operator()(T* ptr) {
        delete[] ptr;
    };
};

// Primary template
template <typename T, typename Deleter = Slug<T>>
class UniquePtr {
public:
    // Constructors
    explicit UniquePtr(T* ptr = nullptr) : data_{ptr, Deleter()} {};

    UniquePtr(T* ptr, Deleter deleter) : data_{ptr, std::forward<Deleter>(deleter)} {};

    template <typename U, typename D>
    UniquePtr(UniquePtr<U, D>&& other) noexcept
        : data_{other.Release(), std::forward<D>(other.GetDeleter())} {};

    UniquePtr(UniquePtr& other) noexcept {
        data_ = other.data_;
    }

    // `operator=`-s
    template <typename U, typename D>
    UniquePtr& operator=(UniquePtr<U, D>&& other) noexcept {
        if (Get() == other.Get()) {
            return *this;
        }

        Reset();
        data_.GetFirst() = other.Release();
        GetDeleter() = std::move(other.GetDeleter());
        return *this;
    };

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (Get() == other.Get()) {
            return *this;
        }

        Reset();
        data_.GetFirst() = other.Release();
        GetDeleter() = std::move(other.GetDeleter());
        return *this;
    };

    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    };

    // Destructor
    ~UniquePtr() {
        Reset();
    };

    // Modifiers
    T* Release() noexcept {
        T* temp = data_.GetFirst();
        data_.GetFirst() = nullptr;
        return temp;
    };

    void Reset(T* ptr = nullptr) {
        T* temp = Get();
        data_.GetFirst() = ptr;

        if (temp != nullptr) {
            GetDeleter()(temp);
        }
    };

    void Swap(UniquePtr& other) {
        std::swap(data_.GetFirst(), other.data_.GetFirst());
        std::swap(data_.GetSecond(), other.data_.GetSecond());
    };

    // Observers
    T* Get() const {
        return data_.GetFirst();
    };

    Deleter& GetDeleter() {
        return data_.GetSecond();
    };

    const Deleter& GetDeleter() const {
        return data_.GetSecond();
    };

    explicit operator bool() const {
        return data_.GetFirst() != nullptr;
    };

    // Single-object dereference operators
    template <typename U = T, typename = std::enable_if_t<!std::is_void_v<U>>>
    U& operator*() const {
        return *data_.GetFirst();
    };

    T* operator->() const {
        return data_.GetFirst();
    };

private:
    CompressedPair<T*, Deleter> data_;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    // Constructors
    explicit UniquePtr(T* ptr = nullptr) : data_{ptr, Deleter()} {};

    // UniquePtr(T* ptr, Deleter deleter) : ptr_{ptr}, deleter_{deleter} {};
    UniquePtr(T* ptr, Deleter deleter) : data_{ptr, deleter} {};

    template <typename U, typename D>
    UniquePtr(UniquePtr<U, D>&& other) noexcept : data_{other.Release(), other.GetDeleter()} {};

    // `operator=`-s
    template <typename U, typename D>
    UniquePtr& operator=(UniquePtr<U, D>&& other) noexcept {
        GetDeleter()(Get());
        data_.GetFirst() = other.Release();
        data_.GetSecond() = other.GetDeleter();
        return *this;
    };

    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        GetDeleter()(Get());
        data_.GerFirst()(nullptr);
        return *this;
    };

    T& operator[](size_t i) {
        return Get()[i];
    }

    // Destructor
    ~UniquePtr() {
        Reset();
    };

    // Modifiers
    T* Release() noexcept {
        T* temp = data_.GetFirst();
        data_.GetFirst() = nullptr;
        return temp;
    };

    void Reset(T* ptr = nullptr) {
        GetDeleter()(data_.GetFirst());
        data_.GetFirst() = ptr;
    };

    void Swap(UniquePtr& other) {
        std::swap(data_.GetFirst(), other.data_.GetFirst());
        std::swap(data_.GetSecond(), other.data_.GetSecond());
    };

    // Observers
    T* Get() const {
        return data_.GetFirst();
    };

    Deleter& GetDeleter() {
        return data_.GetSecond();
    };

    const Deleter& GetDeleter() const {
        return data_.GetSecond();
    };

    explicit operator bool() const {
        return data_.GetFirst() != nullptr;
    };

    // Single-object dereference operators
    template <typename U = T, typename = std::enable_if_t<!std::is_void_v<U>>>
    U& operator*() const {
        return *data_.GetFirst();
    };

    T* operator->() const {
        return data_.GetFirst();
    };

private:
    CompressedPair<T*, Deleter> data_;
};
