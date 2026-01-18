add_executable(duorou_editor
  src/main.cpp
)

target_link_libraries(duorou_editor PRIVATE duorou_ui)
if(DUOROU_HAS_MINISWIFT)
  target_compile_definitions(duorou_editor PRIVATE DUOROU_HAS_MINISWIFT=1)
  if(TARGET miniswift_core)
    target_link_libraries(duorou_editor PRIVATE miniswift_core)
  elseif(TARGET miniswift_ui)
    target_link_libraries(duorou_editor PRIVATE miniswift_ui)
  endif()
endif()

if(WIN32)
  add_executable(duorou_editor_gpu
    ${PROJECT_SOURCE_DIR}/src/gpu_gl.cpp
  )
  target_include_directories(duorou_editor_gpu PRIVATE
    ${PROJECT_SOURCE_DIR}/examples
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_compile_definitions(duorou_editor_gpu PRIVATE
    DUOROU_USE_EDITOR_VIEW=1
    DUOROU_EDITOR_DEFAULT=1
  )

  find_package(glfw3 REQUIRED)
  find_package(Freetype QUIET)
  if(TARGET glfw)
    set(DUOROU_EDITOR_GLFW_TARGET glfw)
  elseif(TARGET glfw3::glfw)
    set(DUOROU_EDITOR_GLFW_TARGET glfw3::glfw)
  endif()
  if(NOT DUOROU_EDITOR_GLFW_TARGET)
    message(FATAL_ERROR "glfw3 found but no CMake target exported")
  endif()

  target_link_libraries(duorou_editor_gpu PRIVATE duorou_ui ${DUOROU_EDITOR_GLFW_TARGET} opengl32 user32 gdi32 shell32 comdlg32)
  if(DUOROU_HAS_MINISWIFT)
    target_compile_definitions(duorou_editor_gpu PRIVATE DUOROU_HAS_MINISWIFT=1)
    if(TARGET miniswift_core)
      target_link_libraries(duorou_editor_gpu PRIVATE miniswift_core)
    elseif(TARGET miniswift_ui)
      target_link_libraries(duorou_editor_gpu PRIVATE miniswift_ui)
    endif()
  endif()
  if(TARGET Freetype::Freetype)
    target_link_libraries(duorou_editor_gpu PRIVATE Freetype::Freetype)
    target_compile_definitions(duorou_editor_gpu PRIVATE DUOROU_HAS_FREETYPE=1)
  endif()
elseif(UNIX AND NOT APPLE)
  add_executable(duorou_editor_gpu
    ${PROJECT_SOURCE_DIR}/src/gpu_gl.cpp
  )
  target_include_directories(duorou_editor_gpu PRIVATE
    ${PROJECT_SOURCE_DIR}/examples
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_compile_definitions(duorou_editor_gpu PRIVATE
    DUOROU_USE_EDITOR_VIEW=1
    DUOROU_EDITOR_DEFAULT=1
  )

  find_package(OpenGL REQUIRED)
  find_package(X11 REQUIRED)
  find_package(glfw3 REQUIRED)
  find_package(Freetype QUIET)
  if(TARGET glfw)
    set(DUOROU_EDITOR_GLFW_TARGET glfw)
  elseif(TARGET glfw3::glfw)
    set(DUOROU_EDITOR_GLFW_TARGET glfw3::glfw)
  endif()
  if(NOT DUOROU_EDITOR_GLFW_TARGET)
    message(FATAL_ERROR "glfw3 found but no CMake target exported")
  endif()

  target_link_libraries(duorou_editor_gpu PRIVATE duorou_ui ${DUOROU_EDITOR_GLFW_TARGET} OpenGL::GL X11::X11)
  if(DUOROU_HAS_MINISWIFT)
    target_compile_definitions(duorou_editor_gpu PRIVATE DUOROU_HAS_MINISWIFT=1)
    if(TARGET miniswift_core)
      target_link_libraries(duorou_editor_gpu PRIVATE miniswift_core)
    elseif(TARGET miniswift_ui)
      target_link_libraries(duorou_editor_gpu PRIVATE miniswift_ui)
    endif()
  endif()
  if(TARGET Freetype::Freetype)
    target_link_libraries(duorou_editor_gpu PRIVATE Freetype::Freetype)
    target_compile_definitions(duorou_editor_gpu PRIVATE DUOROU_HAS_FREETYPE=1)
  endif()
endif()
