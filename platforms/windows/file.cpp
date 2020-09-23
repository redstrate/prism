#include "file.hpp"

#include "string_utils.hpp"

void file::set_domain_path(const file::Domain domain, const file::Path path) {
    domain_data[(int)domain] = replace_substring(path.string(), "{resource_dir}/", "");
}

file::Path file::get_writeable_directory() {
    return "";
}