#pragma once

#include "path.hpp"

/*
* Audio API
*/
namespace audio {
    void initialize();
    
    void play_file(const file::Path path, const float gain = 1.0f);
}
