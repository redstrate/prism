#pragma once

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "vr.hpp"

class OpenXR : public VR {
public:
	void initialize() override;

	XrInstance instance = XR_NULL_HANDLE;
};