#pragma once

#include <iostream>
#include <type_traits>

template<typename T>
class ISerializer {
public:
    virtual ~ISerializer() = default;
    virtual void write(std::ostream& os, const T& value) = 0;
    virtual void read(std::istream& is, T& value) = 0;
};