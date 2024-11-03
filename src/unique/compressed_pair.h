#pragma once

#include <iostream>
#include <type_traits>

template <typename F, typename S,
          bool = std::is_empty<F>() & !std::is_base_of_v<F, S> & !std::is_base_of_v<S, F> &
                 !std::is_final_v<F>,
          bool = std::is_empty<S>() & !std::is_base_of_v<F, S> & !std::is_base_of_v<S, F> &
                 !std::is_final_v<S>>
class CompressedPair {};

template <typename F, typename S>
class CompressedPair<F, S, false, false> {
public:
    CompressedPair() : first_{}, second_{} {};
    CompressedPair(const F& first, const S& second) : first_(first), second_(second) {
    }
    CompressedPair(F& first, S& second) : first_{first}, second_{second} {
    }

    CompressedPair(const F&& first, const S&& second)
        : first_{std::move(first)}, second_{std::move(second)} {
    }
    CompressedPair(F&& first, S&& second) : first_{std::move(first)}, second_{std::move(second)} {
    }

    CompressedPair(F& first, S&& second) : first_{first}, second_{std::move(second)} {
    }

    F& GetFirst() {
        return first_;
    }

    S& GetSecond() {
        return second_;
    };

    const F& GetFirst() const {
        return first_;
    }

    const S& GetSecond() const {
        return second_;
    };

private:
    F first_;
    S second_;
};

template <typename F, typename S>
class CompressedPair<F, S, false, true> : public S {
public:
    CompressedPair() = default;
    CompressedPair(const F& first, const S&) : first_(first) {
    }
    CompressedPair(F& first, S&) : first_{first} {
    }
    CompressedPair(const F&& first, const S&&) : first_{std::move(first)} {
    }
    CompressedPair(F&& first, S&&) : first_{std::move(first)} {
    }

    F& GetFirst() {
        return first_;
    }

    S& GetSecond() {
        return *this;
    }

    const F& GetFirst() const {
        return first_;
    }

    const S& GetSecond() const {
        return *this;
    }

private:
    F first_;
};

template <typename F, typename S>
class CompressedPair<F, S, true, false> : public F {
public:
    CompressedPair() = default;
    CompressedPair(const F&, const S& second) : second_(second) {
    }
    CompressedPair(F&, S& second) : second_{second} {
    }
    CompressedPair(const F&&, const S&& second) : second_{std::move(second)} {
    }
    CompressedPair(F&&, S&& second) : second_{std::move(second)} {
    }

    F& GetFirst() {
        return *this;
    }

    S& GetSecond() {
        return *this;
    }
    const F& GetFirst() const {
        return *this;
    }

    const S& GetSecond() const {
        return *this;
    }

private:
    S second_;
};

template <typename F, typename S>
class CompressedPair<F, S, true, true> : public F, public S {
public:
    CompressedPair() = default;
    CompressedPair(const F&, const S&) {
    }

    F& GetFirst() {
        return *this;
    }

    S& GetSecond() {
        return *this;
    }
};
