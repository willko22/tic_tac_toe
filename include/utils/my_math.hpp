#pragma once
#include <stdexcept>
#include <cmath>
#include <type_traits>

namespace utils {
    template<typename T>
    int oddifyMinMax(T value, int min_value, int max_value) {
        int int_value;
        if constexpr (std::is_floating_point_v<T>) {
            int_value = static_cast<int>(std::round(value));
        } else {
            try {
                int_value = static_cast<int>(value);
            } catch (const std::invalid_argument&) {
                throw std::runtime_error("Invalid value for oddifyMinMax, must be int or float.");
            }
        }

        if (value % 2 == 0) {
            // If even, try to add 1 first, then subtract 1 if that exceeds max
            if (value + 1 <= max_value) {
                return value + 1;
            } else if (value - 1 >= min_value) {  // Assuming minimum value should be 1
                return value - 1;
            } else {
                throw std::runtime_error("Cannot make value odd within bounds.");
            }
        }
        return value;  // Already odd
    }
}