add_library(Platform INTERFACE)
target_include_directories(Platform INTERFACE include)
target_link_libraries(Platform INTERFACE Utility)
