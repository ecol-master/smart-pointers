#pragma once

// #include <exception>
#include <iostream>

class BadWeakPtr : public std::exception {};

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;
