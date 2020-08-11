#include <doctest.h>

#include "utility.hpp"

TEST_SUITE_BEGIN("General Utilities");

TEST_CASE("Enum to string") {
    enum TestEnum {
        A,
        B
    };
    
    CHECK(utility::enum_to_string(TestEnum::A) == "A");
    CHECK(utility::enum_to_string(TestEnum::B) == "B");
}

TEST_SUITE_END();
