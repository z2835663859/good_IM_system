cmake_minimum_required(VERSION 3.16)
project(IM_Client_Qt VERSION 1.0.0 LANGUAGES CXX)

# macOS specific settings
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS deployment target")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
find_package(Qt6 COMPONENTS
    Core
    Gui
    Widgets
    Network
    REQUIRED
)

# Remove AGL framework from Qt6Gui
get_target_property(QT6_GUI_LIBS Qt6::Gui LINK_LIBRARIES)
if(QT6_GUI_LIBS)
    string(REPLACE "-framework;AGL" "" QT6_GUI_LIBS "${QT6_GUI_LIBS}")
    set_target_properties(Qt6::Gui PROPERTIES LINK_LIBRARIES "${QT6_GUI_LIBS}")
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/ui/main_window.hpp
    src/ui/main_window.cpp
    src/network/im_client_network.hpp
    src/network/im_client_network.cpp
    src/models/chat_model.hpp
    src/models/chat_model.cpp
)

# Create executable
add_executable(im_client_gui ${SOURCES})

# Add include directories
target_include_directories(im_client_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link Qt libraries
target_link_libraries(im_client_gui
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

# macOS specific settings
if(APPLE)
    target_link_options(im_client_gui PRIVATE "LINKER:-framework,CoreFoundation")
    # Use Metal instead of AGL
    target_link_options(im_client_gui PRIVATE "-Xlinker" "-alias_list" "-Xlinker" "/dev/null")
    # Remove AGL and use modern OpenGL
    string(REPLACE "-framework AGL" "" FIXED_LINK_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${FIXED_LINK_FLAGS}")
endif()

# Set output directory
set_target_properties(im_client_gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
