#include "openxr.hpp"

#include "log.hpp"
#include "assertions.hpp"

#define XR_CHECK(x) Expects(x == XR_SUCCESS)

void OpenXR::initialize() {
	XrApplicationInfo application_info = {};
	
	strcpy(application_info.applicationName, "Engine Test");
	strcpy(application_info.engineName, "Prism");

	application_info.apiVersion = XR_VERSION_1_0;

	XrInstanceCreateInfo create_info = {};
	create_info.type = XR_TYPE_INSTANCE_CREATE_INFO;
	create_info.applicationInfo = application_info;

	XR_CHECK(xrCreateInstance(&create_info, &instance));

	console::debug(System::Core, "OpenXR is initialized!");
}