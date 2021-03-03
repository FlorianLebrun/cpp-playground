include(ExternalProject)
set(GoogleTestDir ${CMAKE_CURRENT_BINARY_DIR}/install)
ExternalProject_Add(GoogleTest
  URL "https://github.com/google/googletest/archive/release-1.10.0.tar.gz"
  TEST_COMMAND ""
  UPDATE_COMMAND ""
  INSTALL_DIR "${GoogleTestDir}"
  CMAKE_ARGS
    -D gtest_force_shared_crt:BOOL=ON
    -D "CMAKE_INSTALL_PREFIX=${GoogleTestDir}"
)

# Create library
set_target_properties(GoogleTest PROPERTIES FOLDER "ThirdParty")
message(STATUS ${GoogleTestDir})

add_library(gtest INTERFACE)
add_dependencies(gtest GoogleTest)
target_include_directories(gtest INTERFACE "${GoogleTestDir}/include/")
target_link_libraries(gtest INTERFACE "${GoogleTestDir}/lib/gtest*.lib")
