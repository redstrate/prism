#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <optional>
#include <array>
#include <filesystem>

#include "log.hpp"
#include "file_utils.hpp"
#include "path.hpp"

namespace prism {
    enum class domain {
        system,
        internal,
        app
    };

    /// Represents a file handle. The file may or may not be fully loaded in memory.
    class file {
    public:
        explicit file(FILE* handle) : handle(handle) {}
        
        file(file&& f) noexcept :
            mem(std::move(f.mem)),
            handle(std::exchange(f.handle, nullptr)),
            data(std::move(f.data)) {}
        
        ~file() {
            if(handle != nullptr)
                fclose(handle);
        }
        
        /** Loads a type.
         @param t The pointer to the type.
         @param s If not 0, the size to load is derived from sizeof(T).
         */
        template<typename T>
        void read(T* t, const size_t s = 0) {
            fread(t, s == 0 ? sizeof(T) : s, 1, handle);
        }

        /// Reads a string. Assumes the length is an unsigned integer.
        void read_string(std::string& str) {
            unsigned int length = 0;
            read(&length);

            if(length > 0) {
                str.resize(length);

                read(str.data(), sizeof(char) * length);
            }
        }

        /// Loads the entire file into memory, accessible via cast_data()
        void read_all() {
            fseek(handle, 0L, SEEK_END);
            const auto _size = static_cast<size_t>(ftell(handle));
            rewind(handle);

            data.resize(_size);
            fread(data.data(), _size, 1, handle);
        }
        
        /// If the file is loaded in memory, cast the underlying data.
        template<typename T>
        T* cast_data() {
            return reinterpret_cast<T*>(data.data());
        }

        /// If the file is loaded in memory, return the size of the file.
        [[nodiscard]] size_t size() const {
            return data.size();
        }

        /// Reads the entire file as a string.
        std::string read_as_string() {
            auto s = read_as_stream();
            return std::string((std::istreambuf_iterator<char>(s)),
                                std::istreambuf_iterator<char>());
        }

        /// Reads the entire file as a ifstream, useful for JSON deserialization.
        std::istream read_as_stream() {
            read_all();

            auto char_data = cast_data<char>();
            mem = std::make_unique<membuf>(char_data, char_data + size());
            return std::istream(mem.get());
        }
        
    private:
        struct membuf : std::streambuf {
            inline membuf(char* begin, char* end) {
                this->setg(begin, begin, end);
            }
        };
        
        std::unique_ptr<membuf> mem;
        
        FILE* handle = nullptr;
        
        std::vector<std::byte> data;
    };

    /** Sets the domain path to a location in the filesystem.
     @param domain The domain type.
     @param mode The access mode.
     @param path The absolute file path.
     */
    void set_domain_path(domain domain, path domain_path);
    path get_domain_path(domain domain);

    /// Converts an absolute path to a domain relative path.
    path get_relative_path(domain domain, path domain_relative_path);

	/// Returns the path to a writeable directory.
	path get_writeable_directory();

    /**
     Opens a file handle.
     @param domain The file domain.
     @param path The file path.
     @param binary_mode Whether or not to open the file as binary or ASCII. Defaults to false.
     @return An optional with a value if the file was loaded correctly, otherwise it's empty.
     */
    std::optional<file> open_file(path file_path, bool binary_mode = false);
    
    path root_path(path path);
    path get_file_path(const path& path);
    
    inline path internal_domain = "/internal", app_domain = "/app";
}

inline std::array<std::string, 3> domain_data;
