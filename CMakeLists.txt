cmake_minimum_required(VERSION 3.20)
set(target_name mp3con)
project(${target_name})
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(install_dir "C:/nicsmocs/bin")

add_executable(${target_name} ${target_name}.cpp)

set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${install_dir})
target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(${target_name} PRIVATE libmp3lame debug nicsmocsd optimized nicsmocs)

install(FILES ${CMAKE_SOURCE_DIR}/lib/libmp3lame.dll DESTINATION ${install_dir})