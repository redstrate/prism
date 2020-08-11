#pragma once

#include <functional>

struct Timer {
    std::function<void()> callback;
    
    float duration = 0.0f;
    
    float current_time = 0.0f;
    
    bool remove_on_trigger = true;
    bool continue_during_pause = true;
};

inline bool operator==(const Timer& a, const Timer& b) {
    return a.duration == b.duration && a.current_time == b.current_time;
}
