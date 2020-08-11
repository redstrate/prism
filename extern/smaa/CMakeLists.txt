add_library(SMAA INTERFACE)
target_include_directories(SMAA INTERFACE include)

add_library(SMAA::SMAA ALIAS SMAA)
