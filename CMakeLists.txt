cmake_minimum_required(VERSION 3.12)
project(ChessPlusPlus)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(Vulkan REQUIRED)
set(Vulkan_LIBRARIES Vulkan::Vulkan)

add_executable(ChessPlusPlus main.cpp Renderer.cpp Renderer.h)
target_include_directories(ChessPlusPlus PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(ChessPlusPlus ${Vulkan_LIBRARIES})
set_target_properties(ChessPlusPlus PROPERTIES CXX_STANDARD 17)
target_compile_features(ChessPlusPlus PRIVATE cxx_std_17)