#pragma once

class ImGuiLayer {
public:
    ImGuiLayer();
    
    void begin_frame(const float delta_time);    
    void render(int index);
    
    void process_key_down(unsigned int keyCode);
    void process_key_up(unsigned int keyCode);
};
