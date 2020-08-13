#include "file.hpp"

void file::set_domain_path(const file::Domain domain, const file::Path path) {
    domain_data[(int)domain] = path.string();
}

file::Path file::get_writeable_directory() {
    return "";
}