
# Add GoogleTest project
include(ExternalProject)
set(INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install)
ExternalProject_Add(GoogleTest
  URL "https://github.com/google/googletest/archive/release-1.10.0.tar.gz"
  TEST_COMMAND ""
  UPDATE_COMMAND ""
  INSTALL_DIR "${INSTALL_DIR}"
  CMAKE_ARGS
    -D gtest_force_shared_crt:BOOL=ON
    -D "CMAKE_INSTALL_PREFIX=${INSTALL_DIR}"
)
set_target_properties(GoogleTest PROPERTIES FOLDER "ThirdParty")

# Define gtest library
add_library(gtest INTERFACE)
add_dependencies(gtest GoogleTest)
target_include_directories(gtest INTERFACE "${INSTALL_DIR}/include/")
target_link_libraries(gtest INTERFACE "${INSTALL_DIR}/lib/gtest$<$<CONFIG:Debug>:d>.lib")
