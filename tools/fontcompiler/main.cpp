#include <cstdio>

#include <string_view>
#include <array>
#include <iostream>

#include <stb_rect_pack.h>
#include <stb_truetype.h>

constexpr std::array sizes_to_pack = {
    36.0f,
    24.0f
};

constexpr int num_glyphs = 95;
constexpr int texture_width = 2048, texture_height = 1150;

stbtt_pack_context pack_context;
stbtt_packedchar packed_chars[sizes_to_pack.size()][num_glyphs];

stbtt_fontinfo font_info;
unsigned char* font_data = nullptr;

unsigned char font_bitmap[texture_width * texture_height];

void load_ttf(const std::string_view path) {
    FILE* font_file = fopen(path.data(), "rb");
    if (!font_file) {
        std::cerr << "Unable to open font " << path << std::endl;
        return;
    }

    fseek(font_file, 0, SEEK_END);
    const unsigned long file_length = ftell(font_file);
    fseek(font_file, 0, SEEK_SET);

    font_data = new unsigned char[file_length];

    fread(font_data, file_length, 1, font_file);
    fclose(font_file);

    stbtt_InitFont(&font_info, font_data, 0);
}

int main(int argc, char* argv[]) {
    if(argc != 3) {
        std::cout << "Usage: FontCompiler [input ttf] [output fp]" << std::endl;
        return 0;
    }

    load_ttf(argv[1]);

    stbtt_PackBegin(&pack_context, font_bitmap, texture_width, texture_height, 0, 1, nullptr);
    stbtt_PackSetOversampling(&pack_context, 2, 2);

    stbtt_pack_range ranges[sizes_to_pack.size()];
    for(int i = 0; i < sizes_to_pack.size(); i++)
        ranges[i] = {sizes_to_pack[i], 32, nullptr, 95, packed_chars[i], 0, 0};

    stbtt_PackFontRanges(&pack_context, font_data, 0, ranges, sizes_to_pack.size());
    stbtt_PackEnd(&pack_context);

    FILE* file = fopen(argv[2], "wb");
    if(!file)
        return -1;

    fwrite(&texture_width, sizeof(int), 1, file);
    fwrite(&texture_height, sizeof(int), 1, file);

    int ascent, descent, gap;
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &gap);

    fwrite(&ascent, sizeof(int), 1, file);
    fwrite(&descent, sizeof(int), 1, file);
    fwrite(&gap, sizeof(int), 1, file);

    for(int i = 0; i < sizes_to_pack.size(); i++)
        fwrite(&packed_chars[i], sizeof(packed_chars[i]), 1, file);

    for(int i = 0; i < sizes_to_pack.size(); i++) {
        const float f = (ascent + descent) * stbtt_ScaleForPixelHeight(&font_info, ranges[i].font_size);
        fwrite(&f, sizeof(float), 1, file);
    }

    fwrite(font_bitmap, texture_width * texture_height, 1, file);

    fclose(file);

    return 0;
}
