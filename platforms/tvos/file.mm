#include "file.hpp"

#import <Foundation/Foundation.h>
#include <array>

#import <UIKit/UIKit.h>

#include "string_utils.hpp"

#include "log.hpp"

void file::initialize_domain(const FileDomain domain, const AccessMode mode, const std::string_view path) {
    NSBundle* bundle = [NSBundle mainBundle];
    NSString* resourceFolderPath = [bundle resourcePath];
        
    std::string s = std::string(path);
    s = replace_substring(s, "{resource_dir}", std::string([resourceFolderPath cStringUsingEncoding:NSUTF8StringEncoding]));
        
    domain_data[(int)domain] = s;
}

std::string file::get_writeable_path(const std::string_view path) {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    
    NSString* resourceFolderPath = paths[0];
    
    std::string s = std::string([resourceFolderPath cStringUsingEncoding:NSUTF8StringEncoding]) + "/" + std::string(path);
    
    return s;
}
