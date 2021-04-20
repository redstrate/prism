#pragma once

namespace prism {
    class imgui_backend {
    public:
        imgui_backend();

        void begin_frame(float delta_time);

        void render(int index);

        void process_key_down(unsigned int key_code);

        void process_key_up(unsigned int key_code);
    };
}