#pragma once

#include <imgui.h>

#include "utility.hpp"
#include "math.hpp"

namespace ImGui {
    template<typename T>
    inline bool ComboEnum(const char* label, T* t) {
        bool result = false;
        
        const auto preview_value = utility::enum_to_string(*t);
        
        if (ImGui::BeginCombo(label, preview_value.c_str())) {
            for(auto& name : magic_enum::enum_values<T>()) {
                const auto n = utility::enum_to_string(name);
                
                if (ImGui::Selectable(n.c_str(), *t == name)) {
                    *t = name;
                    result = true;
                }
            }
            
            ImGui::EndCombo();
        }
        
        return result;
    }

    template<typename T, typename TMax>
    void ProgressBar(const char* label, const T value, const TMax max) {
        const float progress_saturated = value / static_cast<float>(max);
        
        char buf[32] = {};
        sprintf(buf, "%d/%d", static_cast<int>(value), static_cast<int>(max));
        ImGui::ProgressBar(progress_saturated, ImVec2(0.0f, 0.0f), buf);
        
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::Text(label);
    }

    inline bool DragQuat(const char* label, Quaternion* quat) {
        bool result = false;
        
        Vector3 euler = quat_to_euler(*quat);
        
        euler.x = degrees(euler.x);
        euler.y = degrees(euler.y);
        euler.z = degrees(euler.z);
        
        if(ImGui::DragFloat3(label, euler.ptr(), 1.0f, 0.0f, 0.0f, "%.3f°")) {
            euler.x = radians(euler.x);
            euler.y = radians(euler.y);
            euler.z = radians(euler.z);
            
            *quat = euler_to_quat(euler);
            
            result = true;
        }
        
        return result;
    }
}
