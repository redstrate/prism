# Prism
A cross-platform game engine that integrates a real-time physically based workflow and makes it easy to get started writing games or other graphical applications in C++!

I've been using this to actually develop a game i've been working on, and includes multiple systems to facilitate gameplay (like the UI and audio systems) along with your usual engine systems (like Renderer, and Log). However, the same engine is also proven to be useful for other graphical applications, like the Prism editor.

Although I developed this on macOS, it has been tested on Windows, Linux,  iOS, iPadOS and tvOS machines. Apart from some platform glue, these share a majority of the same code!

![pcss](https://github.com/redstrate/prism/blob/master/misc/pcss.png?raw=true)
![sponza](https://github.com/redstrate/prism/blob/master/misc/sponza.png?raw=true)

![buddha](https://github.com/redstrate/prism/blob/master/misc/buddha.png?raw=true)
![custom models](https://github.com/redstrate/prism/blob/master/misc/custom%20models.png?raw=true)

The sibenik, sponza and buddha models shown are from the [McGuire Computer Graphics Archive](https://casual-effects.com/data/), other models shown are created by me.

## Features
Here is a list of some of the notable features of Prism:

* Cross platform graphics API is used for rendering
	* Vulkan and Metal backends available
        * Shaders are written in GLSL, and compiled to SPIR-V or MSL offline and at runtime using SPIRV-Cross.
* Windowing and System abstraction
	* main() and other platform-specific tidbits are abstracted as well, for easy support on non-desktop platforms.
    * HiDPI is also supported!
	* If available, support for multiple windows as well.
* Dear ImGui used for debug/editor UIs
	* Includes the new docking/viewport branch!
	* Automatic DPI scaling on fonts for crisp rendering when the window is in a HiDPI environment.
	* Custom backend built on top the GFX api and other platform agnostic systems.
	* Plenty of custom widgets available for easy debug tooling, see [imgui_stdlib.h](https://github.com/redstrate/prism/blob/master/extern/imgui/include/imgui_stdlib.h) and [imgui_utility.hpp](https://github.com/redstrate/prism/blob/master/engine/core/include/imgui_utility.hpp)!
* Entity-Component system for scene authoring
	* No runtime polymorphism is involved and leverages the native C++ type system. [Components are simple structs](https://github.com/redstrate/prism/blob/master/engine/core/include/components.hpp).
* Asset management
	* Custom model pipeline allowing for blazingly fast model loading and authoring.
		* Includes a custom blender addon for easy export!
	* Assets are referenced from disk, and only loaded once - unloaded once they are no longer referenced.
	* Thumbnails are created by the editor and stored on disk until further use.
* Custom UI system for easy authoring
	* UIs are declared in JSON, and there is a graphical editor available as well.
	* They can even be placed in the world with the UI component!
* Custom math library
	* Quaternions, matrices, vectors, rays, and more are available!
	* Infinite perspective matrices are created by default.
* Advanced rendering techniques
	* Rendering is consistent regardless of platform or API used!
		* Where older or underperforming GPUs are supported (for example, an Apple TV 4K, or a Thinkpad X230), the engine supports disabling expensive features like IBL or point light shadows along with other scalability options.
	* Real-time physically based rendering including scene probes used for image-based lighting.
	* Material node editor for easy and fast material authoring.
	* Skinned mesh support with animation.
	* PCSS for soft shadows, supported on every type of light.
* Editors supporting scene, cutscene, material authoring
	* Multidocument workflow similar to UE4.
	* Transform handles for easy object manipulation.

## Usage
### Requirements
* CMake
* Support for Vulkan or Metal
* C++ compiler that fully supports C++17
	* MSVC, Clang, and GCC have been tested

There is no example available, but if you clone this repository and include it in your cmake tree, use the function `add_platform_executable` from [AddPlatformExecutable.cmake](https://github.com/redstrate/prism/blob/master/cmake/AddPlatformExecutable.cmake) to create a new Prism app.
