# JIPU

[![Build Status](https://github.com/gandis0713/jipu/workflows/Android/badge.svg)](https://github.com/gandis0713/jipu/workflows/Android/badge.svg)
[![Build Status](https://github.com/gandis0713/jipu/workflows/macOS/badge.svg)](https://github.com/gandis0713/jipu/workflows/macOS/badge.svg)
[![Build Status](https://github.com/gandis0713/jipu/workflows/Windows/badge.svg)](https://github.com/gandis0713/jipu/workflows/Windows/badge.svg)

JIPU to implement WebGPU headers (https://github.com/webgpu-native/webgpu-headers).

## Status
|Graphics API| Support | Platform |
|-|-|-|
|DirectX 12 | planed | Windows |
|Vulkan| in progress | Android, Linux, Windows, macOS |
|Metal | Planed | macOS, iOS |

## Requirements

- vulkan ( currently, only support vulkan )
  - If you're on macOS, MoltelVk must be installed.
- vcpkg ( 2024.10.21 )
- cmake ( 3.22 or higher )
- ninja ( if you want to build as ninja )

## How to build
### macOS
```shell
$> cmake --preset arm64-osx-ninja-debug
$> cmake --build arm64-osx-ninja-debug
```

### Windows
```shell
$> cmake --preset x64-windows-ninja-debug
$> cmake --build x64-windows-ninja-debug
```