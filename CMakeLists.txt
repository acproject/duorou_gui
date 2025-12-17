cmake_minimum_required(VERSION 3.20)

project(duorou_gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(duorou_ui INTERFACE)
target_include_directories(duorou_ui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(duorou_demo src/main.cpp)
target_link_libraries(duorou_demo PRIVATE duorou_ui)

if(APPLE)
  enable_language(OBJCXX)
  add_executable(duorou_gpu_demo src/gpu_metal.mm)
  target_link_libraries(duorou_gpu_demo PRIVATE duorou_ui "-framework Cocoa" "-framework CoreGraphics" "-framework CoreText" "-framework Metal" "-framework MetalKit" "-framework QuartzCore")
  target_compile_options(duorou_gpu_demo PRIVATE -fobjc-arc)
elseif(WIN32)
  add_executable(duorou_gpu_demo src/gpu_gl.cpp)
  target_link_libraries(duorou_gpu_demo PRIVATE duorou_ui opengl32 user32 gdi32)
elseif(UNIX)
  add_executable(duorou_gpu_demo src/gpu_gl.cpp)
  find_package(OpenGL REQUIRED)
  find_package(X11 REQUIRED)
  target_link_libraries(duorou_gpu_demo PRIVATE duorou_ui OpenGL::GL X11::X11)
endif()
