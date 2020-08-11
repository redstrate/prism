#pragma once

#include <string>

#include "commoneditor.hpp"

class CutsceneEditor : public CommonEditor {
public:
    CutsceneEditor();
    
    void drawUI() override;
};
