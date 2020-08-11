#include "file.hpp"

#include <array>

#include "string_utils.hpp"

#include "log.hpp"

struct DomainData {
    std::string path;
};

std::array<DomainData, 3> domain_data;

void file::initialize_domain(const FileDomain domain, const AccessMode mode, const std::string_view path) {
    domain_data[(int)domain].path = path;
}

std::string file::clean_path(const std::string_view path) {
    auto p = replaceSubstring(std::string(path), "%20", " ");
    p = replaceSubstring(p, "%7C", "|");

    // this is a path returned by an editor, so skip it
    // TODO: find a better way to do this!! NOO!!
    if (p.find("file:///") != std::string::npos)
        return p.substr(7, p.length());
    else
        return p;
}

/*
 * converts an absolute path to a domain relative one
 */
std::string file::get_relative_path(const FileDomain domain, const std::string_view path) {
    auto p = removeSubstring(std::string(path), domain_data[(int)domain].path + "/");

    return p;
}

std::string file::get_writeable_path(const std::string_view path) {
    return std::string(path);
}

std::optional<File> file::read_file(const FileDomain domain, const std::string_view path, bool binary_mode) {
    auto s = std::string(path);
    s = domain_data[(int)domain].path + "/" + s;

    FILE* file = fopen(s.c_str(), binary_mode ? "rb" : "r");
    if (file == nullptr) {
        console::error(System::File, "Failed to open file handle from {}!", s);
        return {};
    }

    File f;
    f.handle = file;

    return f;
}

std::string file::get_file_path(const FileDomain domain, const std::string_view path) {
    auto s = std::string(path);
    return domain_data[(int)domain].path + "/" + s;
}
