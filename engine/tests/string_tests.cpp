#include <doctest.h>

#include "string_utils.hpp"

TEST_SUITE_BEGIN("String Utilities");

TEST_CASE("Remove substring") {
    std::string str = "foobar";
    
    CHECK(remove_substring(str, "foo") == "bar");
    CHECK(remove_substring(str, "bar") == "foo");
}

TEST_CASE("Replace substring") {
    std::string str = "foobar";
    
    CHECK(replace_substring(str, "foo", "bar") == "barbar");
    CHECK(replace_substring(str, "bar", "foo") == "foofoo");
}

TEST_CASE("String contains") {
    std::string str = "foobar";
    
    CHECK(string_contains(str, "foo") == true);
    CHECK(string_contains(str, "bar") == true);
    CHECK(string_contains(str, "farboo") == false);
}

TEST_CASE("String starts with") {
    std::string str = "(win)and(lose)";
    
    CHECK(string_starts_with(str, "(") == true);
    CHECK(string_starts_with(str, "(win)") == true);
    CHECK(string_starts_with(str, "(lose)") == false);
}

TEST_CASE("Tokenize") {
    std::string first_case = "1,2,3";
    
    CHECK(tokenize(first_case) == std::vector<std::string>({"1", "2", "3"}));
    
    SUBCASE("Alternative delimiters") {
        std::string second_case = "1-2-3";
        
        CHECK(tokenize(second_case, "-") == std::vector<std::string>({"1", "2", "3"}));
    }
}

TEST_SUITE_END();
