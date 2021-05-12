#include "file.hpp"

#include <cstdio>

#include "string_utils.hpp"
#include "log.hpp"
#include "assertions.hpp"

prism::Path prism::root_path(const Path path) {
    auto p = path;
    while(p.parent_path() != p && p.parent_path() != "/") {
        p = p.parent_path();
    }
    
    return p;
}

std::optional<prism::file> prism::open_file(const prism::Path path, const bool binary_mode) {
    Expects(!path.empty());
    
    auto str = get_file_path(path).string();
    FILE* file = fopen(str.c_str(), binary_mode ? "rb" : "r");
    if(file == nullptr) {
        prism::log::error(System::File, "Failed to open file handle from {}!", str);
        return {};
    }
    
    return prism::file(file);
}

prism::Path prism::get_file_path(const prism::Path& path) {
    auto fixed_path = path;
    auto root = root_path(path);
    if(root == app_domain) {
        fixed_path = domain_data[static_cast<int>(domain::app)] / path.lexically_relative(root_path(path));
    } else if(root == internal_domain) {
        fixed_path = domain_data[static_cast<int>(domain::internal)] / path.lexically_relative(root_path(path));
    }
    
    return fixed_path;
}

prism::Path prism::get_domain_path(const domain domain) {
    return domain_data[static_cast<int>(domain)];
}

prism::Path parent_domain(const prism::Path& path) {
    return path;
}

prism::Path prism::get_relative_path(const domain domain, const Path path) {
    // unimplemented
    return path;
}
