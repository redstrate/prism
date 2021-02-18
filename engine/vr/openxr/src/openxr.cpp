#include "openxr.hpp"

#include "log.hpp"
#include "assertions.hpp"

#define XR_CHECK(x) Expects(x == XR_SUCCESS)

void OpenXR::initialize() {
	XrApplicationInfo application_info = {};
	
	strncpy(application_info.applicationName, "Engine Test", XR_MAX_APPLICATION_NAME_SIZE);
	strncpy(application_info.engineName, "Prism", XR_MAX_ENGINE_NAME_SIZE);
	application_info.applicationVersion = 1;
	application_info.engineVersion = 0;
	application_info.apiVersion = XR_CURRENT_API_VERSION;

	XrInstanceCreateInfo create_info = {};
	create_info.type = XR_TYPE_INSTANCE_CREATE_INFO;
	create_info.applicationInfo = application_info;

	console::debug(System::Core, "{}", std::to_string(xrCreateInstance(&create_info, &instance)));

	console::debug(System::Core, "OpenXR is initialized!");
}