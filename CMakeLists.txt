cmake_minimum_required(VERSION 3.31)
project(Budget)

set(CMAKE_CXX_STANDARD 20)

# Find SDL3 and SDL3_ttf
find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

# Resource files (fonts, icons)
set(RESOURCE_FILES
    assets/JetBrainsMono-Regular.ttf
    assets/Budget.icns
    assets/edit.svg
    assets/last.svg
)

# macOS-specific resources
set_source_files_properties(${RESOURCE_FILES} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
)

# Executable
add_executable(Budget MACOSX_BUNDLE
    main.cpp
    src/HeaderOnly/Block.h
    src/Textbox/Textbox.cpp
    src/HeaderOnly/Helpers.h
    ${RESOURCE_FILES}
    src/HeaderOnly/Schedule.h
    src/Menu/Menu.h
    src/Menu/Menu.mm
    src/Preferences/Preferences.cpp
    src/Preferences/Preferences.h
    src/Slider/Slider.cpp
    src/Slider/Slider.h
    src/CheckBox/CheckBox.cpp
    src/CheckBox/CheckBox.h
    src/ScrollBar/ScrollBar.cpp
    src/ScrollBar/ScrollBar.h
)

# Target properties
set_target_properties(Budget PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist"
    MACOSX_BUNDLE_ICON_FILE "Budget.icns"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
)

# Installation path
set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/Applications/Nebula")
install(TARGETS Budget
    BUNDLE DESTINATION "."
)

# Link libraries
target_link_libraries(Budget PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf "-framework Cocoa")
