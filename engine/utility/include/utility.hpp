#pragma once

#include <magic_enum.hpp>

#include <algorithm>
#include <vector>
#include <random>
#include <unordered_map>
#include <cmath>

#include "vector.hpp"

namespace utility {
    template<class Enum>
    std::string enum_to_string(const Enum e) {
        const std::string_view name = magic_enum::enum_name(e);
        
        std::string s = name.data();
        return s.substr(0, name.length());
    }

    template<class T, class F>
    void erase_if(std::vector<T>& vec, const F& f) {
         vec.erase(std::remove_if(vec.begin(), vec.end(), f), vec.end());
    }

    template<class T, class V>
    void erase(T& vec, const V& t) {
        vec.erase(std::remove(vec.begin(), vec.end(), t), vec.end());
    }

    template<class T>
    void erase_at(std::vector<T>& vec, const int index) {
        vec.erase(vec.begin() + index);
    }

    inline int get_random(const int min, const int max) {
        std::random_device rd;
        std::mt19937 eng(rd());
        std::uniform_int_distribution<> distr(min, max);

        return distr(eng);
    }

    inline int get_random(const int max) {
        return get_random(0, max);
    }

    template<class T>
    inline auto get_random(const std::vector<T>& vec) {
        return vec[get_random(static_cast<int>(vec.size() - 1))];
    }

    template<class T, size_t S>
    inline auto get_random(const std::array<T, S>& vec) {
        return vec[get_random(static_cast<int>(vec.size() - 1))];
    }

    template<class T, class V>
    inline auto get_random(const std::unordered_map<T, V>& map) {
        const auto item = map.begin();
        std::advance(item, get_random(static_cast<int>(map.size() - 1)));

        return item;
    }

    template<class T, class TIter = decltype(std::begin(std::declval<T>())), class = decltype(std::end(std::declval<T>()))>
    constexpr auto enumerate(T&& iterable)
    {
        struct iterator {
            size_t i;
            TIter iter;
            bool operator != (const iterator & other) const { return iter != other.iter; }
            void operator ++ () { ++i; ++iter; }
            auto operator * () const { return std::tie(i, *iter); }
        };
        
        struct iterable_wrapper {
            T iterable;
            auto begin() { return iterator{ 0, std::begin(iterable) }; }
            auto end() { return iterator{ 0, std::end(iterable) }; }
        };
        
        return iterable_wrapper{ std::forward<T>(iterable) };
    }
        
    template<class T, class V>
    inline bool contains(const std::vector<T>& vec, const V& val) {
        return std::count(vec.begin(), vec.end(), val);
    }

    /// Packs two 16-bit unsigned ints into one 32 bit unsigned integer.
    inline uint32_t pack_u32(const uint16_t a, const uint16_t b) {
        return (b << 16) | a;
    }
    
    /// Converts a floating-point to a fixed 16-bit unsigned integer.
    inline uint16_t to_fixed(const float f) {
        return static_cast<uint16_t>(32.0f * f + 0.5f);
    }

    inline prism::float3 from_srgb_to_linear(const prism::float3 sRGB) {
        prism::float3 linear = sRGB;
        for(auto& component : linear.data) {
            if(component > 0.04045f) {
                component = std::pow((component + 0.055) / (1.055), 2.4f);
            } else if (component <= 0.04045) {
                component /= 12.92f;
            }
        }

        return linear;
    }
}
