cmake_minimum_required (VERSION 3.7)

include(BundleUtilities)

set(APP_HEADER_FILES
  ${PROJECT_SOURCE_DIR}/platforms/tvos/AppDelegate.h
)

set(APP_SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/platforms/tvos/AppDelegate.m
  ${PROJECT_SOURCE_DIR}/platforms/tvos/main.m
)

set(RESOURCES
  ${PROJECT_SOURCE_DIR}/platforms/tvos/Main.storyboard
  ${PROJECT_SOURCE_DIR}/platforms/tvos/LaunchScreen.storyboard
  ${CMAKE_SOURCE_DIR}/data
  ${CMAKE_BINARY_DIR}/shaders
)

include(../../cmake/AddPlatformExecutable.cmake)

# Locate system libraries on iOS
find_library(UIKIT UIKit)
find_library(FOUNDATION Foundation)
find_library(MOBILECORESERVICES MobileCoreServices)
find_library(CFNETWORK CFNetwork)
find_library(SYSTEMCONFIGURATION SystemConfiguration)
find_library(GAMECONTROLLER GameController)
find_library(QUARTZ QuartzCore)

# link the frameworks located above


add_platform(
    MAIN_FILE
    ViewController.mm.in
    SRC
    MACOSX_BUNDLE
    ${APP_HEADER_FILES}
    ${APP_SOURCE_FILES}
    ${RESOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/file.mm
    EXECUTABLE_PROPERTIES
    MACOSX_BUNDLE ON
    LINK_LIBRARIES
    Core
    GFXMetal
${UIKIT}
 ${FOUNDATION}
 ${MOBILECORESERVICES}
${CFNETWORK}
 ${SYSTEMCONFIGURATION}
${GAMECONTROLLER}
${QUARTZ}
   GFXMetal
    COMPILE_OPTIONS
)

function(add_platform_commands APP_NAME)
set(APP_BUNDLE_IDENTIFIER "com.test.app.fantasy")                # <== Set to your app's bundle identifier

set(CODE_SIGN_IDENTITY "iPhone Developer")                       # <== Set to your team ID from Apple
set(DEVELOPMENT_TEAM_ID "JM5LKVKH48")                  # <== Set to your preferred code sign identity, to see list:
                                                            # /usr/bin/env xcrun security         # <== Set to "1" to target iPhone, set to "2" to target iPad, set to "1,2" to target both
set(RESOURCES
  ${PROJECT_SOURCE_DIR}/engine/platforms/tvos/Main.storyboard
  ${PROJECT_SOURCE_DIR}/engine/platforms/tvos/LaunchScreen.storyboard
  ${CMAKE_SOURCE_DIR}/data
  ${CMAKE_BINARY_DIR}/shaders
)

set(PRODUCT_NAME ${APP_NAME})
set(EXECUTABLE_NAME ${APP_NAME})
set(MACOSX_BUNDLE_EXECUTABLE_NAME ${APP_NAME})
set(MACOSX_BUNDLE_INFO_STRING ${APP_BUNDLE_IDENTIFIER})
set(MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_BUNDLE_IDENTIFIER})
set(MACOSX_BUNDLE_BUNDLE_NAME ${APP_BUNDLE_IDENTIFIER})
set(MACOSX_BUNDLE_ICON_FILE "")
set(MACOSX_BUNDLE_LONG_VERSION_STRING "1.0")
set(MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")
set(MACOSX_BUNDLE_BUNDLE_VERSION "1.0")
set(MACOSX_BUNDLE_COPYRIGHT "Copyright YOU")
set(MACOSX_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-arc")

configure_file(${PROJECT_SOURCE_DIR}/engine/platforms/tvos/plist.in ${CMAKE_BINARY_DIR}/${add_platform_executable_TARGET}Info.plist)

set_target_properties(${APP_NAME} PROPERTIES
                      XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
                      RESOURCE "${RESOURCES}"
                      XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES"
                      XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET}
                      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${CODE_SIGN_IDENTITY}
                      XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${DEVELOPMENT_TEAM_ID}
                      XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY ${DEVICE_FAMILY}
    		      MACOSX_BUNDLE TRUE
                      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/${APP_NAME}Info.plist"
                      XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
                      XCODE_ATTRIBUTE_COMBINE_HIDPI_IMAGES NO
                      XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
                      XCODE_ATTRIBUTE_ENABLE_TESTABILITY YES
                      XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN YES
)

# Set the app's linker search path to the default location on iOS
set_target_properties(
    ${APP_NAME}
    PROPERTIES
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS
    "@executable_path/Frameworks"
)

endfunction()
