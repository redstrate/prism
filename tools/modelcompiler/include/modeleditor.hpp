#pragma once

#include "commoneditor.hpp"

class ModelEditor : public CommonEditor {
public:    
	ModelEditor();

	void drawUI() override;
    
    struct Flags {
        bool hide_ui = false;
        bool compile_static = false;
        bool export_animations = true;
        bool export_materials = true;
    } flags;
    
    std::string data_path, model_path;
    
    void compile_model();
};
