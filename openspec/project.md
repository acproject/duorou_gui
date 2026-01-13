# Project Context

## Purpose

A declarative UI framework implemented in C++20 inspired by SwiftUI/React patterns. Provides a cross-platform rendering engine with a declarative API for building desktop GUI applications. The framework uses a virtual view tree that computes layouts and renders to platform-specific graphics APIs (Metal on macOS, OpenGL on Linux/Windows via GLFW).

## Tech Stack

- **Language**: C++20
- **Build System**: CMake 3.20+
- **Rendering**: Metal (macOS), OpenGL (Linux/Windows via GLFW)
- **Platforms**: macOS (Apple Silicon & Intel), Linux, Windows

## Project Conventions

### Code Style

- **Headers**: Use `#pragma once` for include guards
- **Namespaces**: `namespace duorou::ui` for all public APIs
- **Free Functions**: Mark as `inline` where appropriate
- **Naming**:
  - Files: kebab-case (e.g., `component_button.hpp`)
  - Types: PascalCase (e.g., `ViewNode`, `State<T>`)
  - Functions/variables: snake_case (e.g., `measure_node`, `tree_`)
  - Template parameters: UpperCamelCase (e.g., `State<T>`)
- **Access Control**: Private members suffixed with underscore (e.g., `value_`)
- **Braces**: K&R style (1TBS)

### Architecture Patterns

- **Declarative UI**: Views built via builder pattern (`view("Type").prop(...).children(...).build()`)
- **State Management**: `State<T>` for reactive state with subscription-based change detection
- **Component Model**: Leaf components (Text, Button, TextField) and layout components (Column, Row, Box)
- **Rendering Pipeline**: ViewNode -> LayoutNode -> RenderOps -> Platform graphics
- **Event Handling**: Bubble-up event dispatch with pointer capture and focus management
- **Library Design**: Header-only interface (`duorou_ui` interface library) with platform-specific implementations

### Testing Strategy

No formal testing framework established. Demo executables (`duorou_demo`, `duorou_gpu_demo`) used for manual verification.

### Git Workflow

Standard feature branch workflow. Commits should be descriptive. No strict conventions enforced.

## Domain Context

The framework implements a subset of SwiftUI-like components:

- **Layout**: Column (VStack), Row (HStack), Box (ZStack), Spacer, ScrollView
- **Inputs**: Button, TextField, Checkbox, Slider
- **Display**: Text, Image, Divider
- **Props**: All components accept typed props (string, int64, double, bool) via uniform API
- **Events**: pointer_down, pointer_up, pointer_move, click, focus, blur, key_down, key_up, text_input

## Important Constraints

- C++20 minimum required
- Header-only public interface
- Platform-specific rendering backends must be compiled separately
- No runtime dependency on external C++ libraries (except GLFW/OpenGL for non-Apple platforms)
- State must be captured by value in event handlers (closure pattern)

## External Dependencies

- **macOS**: Cocoa, CoreGraphics, CoreText, Metal, MetalKit, QuartzCore (system frameworks)
- **Linux/Windows**: GLFW 3.x, OpenGL, X11 (Linux only)
- **Optional**: Freetype (for improved text rendering when available)
