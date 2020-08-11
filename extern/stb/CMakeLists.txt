set(HEADERS
    include/stb_image.h)
add_library(stb STATIC
    src/stb.c
    ${HEADERS})
target_include_directories(stb PUBLIC include)
