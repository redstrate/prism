#include "file.hpp"

#include <cstdio>

#include "string_utils.hpp"
#include "log.hpp"
#include "assertions.hpp"

file::Path file::root_path(const Path path) {
    auto p = path;
    while(p.parent_path() != p && p.parent_path() != "/") {
        p = p.parent_path();
    }
    
    return p;
}

std::optional<file::File> file::open(const file::Path path, const bool binary_mode) {
    Expects(!path.empty());
    
    auto str = get_file_path(path).string();
    FILE* file = fopen(str.c_str(), binary_mode ? "rb" : "r");
    if(file == nullptr) {
        prism::log::error(System::File, "Failed to open file handle from {}!", str);
        return {};
    }
    
    return file::File(file);
}

file::Path file::get_file_path(const file::Path path) {
    auto fixed_path = path;
    auto root = root_path(path);
    if(root == app_domain) {
        fixed_path = domain_data[static_cast<int>(Domain::App)] / path.lexically_relative(root_path(path));
    } else if(root == internal_domain) {
        fixed_path = domain_data[static_cast<int>(Domain::Internal)] / path.lexically_relative(root_path(path));
    }
    
    return fixed_path;
}

file::Path file::get_domain_path(const Domain domain) {
    return domain_data[static_cast<int>(domain)];
}

file::Path parent_domain(const file::Path path) {
    return path;
}

file::Path file::get_relative_path(const Domain domain, const Path path) {
    // unimplemented
    return path;
}
