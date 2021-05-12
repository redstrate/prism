#include "file.hpp"

#include "string_utils.hpp"

void prism::set_domain_path(const prism::domain domain, const prism::Path path) {
    domain_data[(int)domain] = replace_substring(path.string(), "{resource_dir}/", "");
}

prism::Path prism::get_writeable_directory() {
    return "~";
}
