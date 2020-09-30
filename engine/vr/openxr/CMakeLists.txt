add_library(openxr STATIC src/openxr.cpp)
target_include_directories(openxr PUBLIC include PUBLIC ${CMAKE_BINARY_DIR}/_deps/openxr-src/include) # openxr cmake is weird, and includes their headers under PRIVATE instead of PUBLIC :-/
target_link_libraries(openxr PUBLIC openxr_loader VR PRIVATE Log)