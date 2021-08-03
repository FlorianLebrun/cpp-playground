cmake_minimum_required(VERSION 3.16)

project(Playground
  LANGUAGES CXX C CSharp
  VERSION 1.0.0
)

set(OPENSSL_ROOT_DIR "d:/vcpkg/installed/x64-windows")

include(.tools/index.cmake)
add_subdirectory(./Other/boost_1_70_0)

add_subdirectory(./System/SegmentAllocTable)
