# Prism
A cross-platform engine that integrates a real-time physically based renderer and makes it easy to get started writing games or other graphical applications in C++!

Here are a couple of screenshots that provide a sense of the graphical capabilities of Prism:

![pcss](https://github.com/redstrate/prism/blob/master/misc/pcss.png?raw=true)
![sponza](https://github.com/redstrate/prism/blob/master/misc/sponza.png?raw=true)

![buddha](https://github.com/redstrate/prism/blob/master/misc/buddha.png?raw=true)
![custom models](https://github.com/redstrate/prism/blob/master/misc/custom%20models.png?raw=true)

These are screenshots taken on macOS, using the _Metal_ API. [Work is still in progress to bring the Vulkan backend up to the same feature set](https://github.com/redstrate/prism/projects/1).

The sibenik, sponza and buddha models shown are from the [McGuire Computer Graphics Archive](https://casual-effects.com/data/), any other models shown are created by me.

## Development
Prism is still a heavy work in progress, and stuff is expected to break. I don't work full time on Prism, so updates are expected to be erratic.

Submitting bug reports and showing stuff you made in Prism is always appreciated! However, if you're submitting a feature request, please look at the [Wiki](https://github.com/redstrate/prism/wiki), [Issues](https://github.com/redstrate/prism/issues) and [Projects](https://github.com/redstrate/prism/projects) first to see my current development plans.

If you're building content for Prism, there is a Blender addon in `addon/` that integrates a content pipeline for easy exporting without leaving Blender!

## Features
Using C++, you can easily build graphically powered applications that is expected to work consistently regardless of the platform used. There is a PBR renderer included in the repository, but anything can built on top of the GFX api and other platform abstractions.

If you're building a game, there is Input, UI and a basic Audio system available to use. There is also a lot of tools in `tools/` that allow you to curate content using the built-in Prism systems such as scenes, cutscenes and materials.

If you're building a tool, [ImGui](https://github.com/ocornut/imgui) is available to use and uses the docking branch. See `tools/` in the repository for examples, most of them is actually built on top of Prism.

You can view a more comprehensive list of features [here](https://github.com/redstrate/prism/wiki/List-of-Features).

## Usage
### Requirements
* CMake
* C++ compiler that _fully_ supports C++17
	* (2019) MSVC, Clang, and GCC have been tested

There are instructions for Windows, Linux, macOS, tvOS and iOS/iPadOS targets in the [wiki](https://github.com/redstrate/prism/wiki). There is an example app provided in `example/`. If you want to build the tooling or the example, use the CMake options `BUILD_EXAMPLE` and `BUILD_TOOLS` respectively.
