cmake_minimum_required(VERSION 3.20)
project(InfiniteSight VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Optional: Find libvips
find_package(VIPS QUIET)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImageLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImageCache.cpp
)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImageLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImageCache.h
)

set(RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(VIPS_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${VIPS_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${VIPS_LIBRARIES})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${VIPS_DEFINITIONS})
    message(STATUS "libvips found: ${VIPS_VERSION}")
else()
    message(WARNING "libvips not found. Large image support will be disabled. Set VIPS_ROOT to a vips-dev installation.")
endif()

# Platform-specific settings
if(WIN32)
    set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE TRUE)
endif()

# Install targets
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
