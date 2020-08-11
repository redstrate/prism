#include "file.hpp"

#include <array>

#include "string_utils.hpp"

#include "log.hpp"

void file::initialize_domain(const FileDomain domain, const AccessMode mode, const std::string_view path) {
    auto p = replace_substring(std::string(path), "{resource_dir}/", "");

    domain_data[(int)domain] = file::clean_path(p);
}

std::string file::clean_path(const std::string_view path) {
    auto p = replace_substring(std::string(path), "%20", " ");
    p = replace_substring(p, "%7C", "|");

    // this is a path returned by an editor, so skip it
    // TODO: find a better way to do this!! NOO!!
    if(p.find("file:///") != std::string::npos)
        return p.substr(7, p.length());
    else
        return p;
}

/*
 * converts an absolute path to a domain relative one
 */
std::string file::get_relative_path(const FileDomain domain, const std::string_view path) {
    auto p = remove_substring(std::string(path), domain_data[(int)domain] + "/");

    return p;
}

/*
 * Returns the path inside of the game's writable directory.
 * path - the path to use.
 */
std::string file::get_writeable_path(const std::string_view path) {
    return std::string(path.data());
}

std::optional<File> file::read_file(const FileDomain domain, const std::string_view path, bool binary_mode) {
    auto s = std::string(path);
    s = domain_data[(int)domain] + "/" + s;

    FILE* file = fopen(s.c_str(), binary_mode ? "rb" : "r");
    if(file == nullptr) {
        console::error(System::File, "Failed to open file handle from {}!", s);
        return {};
    }

    File f;
    f.handle = file;

    return f;
}

std::string file::get_file_path(const FileDomain domain, const std::string_view path) {
    auto s = std::string(path);
    return domain_data[(int)domain] + "/" + s;
}
