#include "string_utils.hpp"

std::string remove_substring(const std::string_view string, const std::string_view substring) {
    std::string result(string);
    
    const auto substring_length = substring.length();
    for(auto i = result.find(substring); i != std::string::npos; i = result.find(substring))
        result.erase(i, substring_length);
    
    return result;
}

std::string replace_substring(const std::string_view string, const std::string_view substring, const std::string_view replacement) {
    std::string result(string);
    
    const auto substring_length = substring.length();
    size_t i = 0;
    while((i = result.find(substring, i)) != std::string::npos) {
        result.erase(i, substring_length);
        result.insert(i, replacement);
        i += replacement.length();
    }
    
    return result;
}

bool string_contains(const std::string_view a, const std::string_view b) {
    return a.find(b) != std::string::npos;
}

bool string_starts_with(const std::string_view haystack, const std::string_view needle) {
    return needle.length() <= haystack.length()
    && std::equal(needle.begin(), needle.end(), haystack.begin());
}

bool is_numeric(const std::string_view string) {
    return std::find_if(string.begin(), string.end(), [](unsigned char c) { return !std::isdigit(c); }) == string.end();
}

std::vector<std::string> tokenize(const std::string_view string, const std::string_view& delimiters) {
    std::vector<std::string> tokens;
    
    const size_t length = string.length();
    size_t lastPos = 0;
    
    while(lastPos < length + 1) {
        size_t pos = string.find_first_of(delimiters, lastPos);
        if(pos == std::string_view::npos)
            pos = length;
        
        if(pos != lastPos)
            tokens.emplace_back(string.data() + lastPos, pos - lastPos);
        
        lastPos = pos + 1;
    }
    
    return tokens;
}
