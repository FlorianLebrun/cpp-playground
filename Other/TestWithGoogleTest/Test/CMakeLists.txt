set(target Test)
set(files main.cpp)
source_group("" FILES ${files})
add_executable(${target} WIN32 ${files})

target_link_options(${target} PRIVATE /SUBSYSTEM:CONSOLE)
target_link_libraries(${target} PUBLIC gtest)

