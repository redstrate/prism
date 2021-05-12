#include "file.hpp"

#import <Foundation/Foundation.h>
#include <array>

#import <AppKit/AppKit.h>

#include "string_utils.hpp"

#include "log.hpp"

inline std::string clean_path(const std::string_view path) {    
    auto p = replace_substring(path, "%20", " ");
    
    // this is a path returned by an editor, so skip it
    // TODO: find a better way to do this!! NOO!!
    if(p.find("file:///") != std::string::npos)
        return p.substr(7, p.length());
    else
        return p;
}

void prism::set_domain_path(const prism::domain domain, const prism::path path) {
    NSBundle* bundle = [NSBundle mainBundle];
    NSString* resourceFolderPath = [bundle resourcePath];
        
    std::string s = std::string(path);
    s = replace_substring(s, "{resource_dir}", [resourceFolderPath cStringUsingEncoding:NSUTF8StringEncoding]);
        
    // basically, if we pass a relative path this will convert it to an absolute one, making get_relative_path functional.
    NSURL * bundleURL = [[bundle bundleURL] URLByDeletingLastPathComponent];
    NSURL *url = [NSURL URLWithString:[NSString stringWithFormat:@"%s", s.c_str()] relativeToURL:bundleURL];
    
    domain_data[static_cast<int>(domain)] = clean_path([[[url absoluteURL] absoluteString] cStringUsingEncoding:NSUTF8StringEncoding]);
}

prism::path prism::get_writeable_directory() {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* resourceFolderPath = paths[0];
 
    return [resourceFolderPath cStringUsingEncoding:NSUTF8StringEncoding];
}
