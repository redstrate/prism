#pragma once

#include <vector>

#include "vector.hpp"
#include "quaternion.hpp"
#include "math.hpp"

struct PositionKeyFrame {
    float time;
    Vector3 value;
};

inline bool operator==(const PositionKeyFrame& lhs, const PositionKeyFrame& rhs) {
    return nearly_equal(lhs.time, rhs.time) && nearly_equal(lhs.value.x, rhs.value.x);
}

struct RotationKeyFrame {
    float time;
    Quaternion value;
};

struct ScaleKeyFrame {
    float time;
    Vector3 value;
};

struct Bone;

struct AnimationChannel {
    std::string id;
    
    Object target = NullObject;
    Bone* bone = nullptr;
    
    std::vector<PositionKeyFrame> positions;
    std::vector<RotationKeyFrame> rotations;
    std::vector<ScaleKeyFrame> scales;
};

inline bool operator==(const AnimationChannel& lhs, const AnimationChannel& rhs) {
    return lhs.id == rhs.id && lhs.target == rhs.target && lhs.positions == rhs.positions;
}

struct Animation {
    double ticks_per_second = 0.0;
    double duration = 0.0;
    
    std::vector<AnimationChannel> channels;
};

struct Shot {
    int begin, length;
    
    std::vector<AnimationChannel> channels;
    
    Scene* scene = nullptr;
};

inline bool operator==(const Shot& lhs, const Shot& rhs) {
    return lhs.begin == rhs.begin && lhs.length == rhs.length;
}

class Cutscene {
public:
    std::vector<Shot> shots;
    
    int get_real_end() {
        int end = -1;
        for(auto& shot : shots) {
            if((shot.begin + shot.length) >= end)
                end = shot.begin + shot.length;
            
        }
        
        return end;
    }
};
