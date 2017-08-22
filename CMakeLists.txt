cmake_minimum_required(VERSION 3.7)

project(editor)

find_package(OPENGL REQUIRED)

add_subdirectory(tests)

add_executable(editor
    editor.cpp
    txt.cpp
    txt.h
    )

target_compile_features(editor
    PRIVATE cxx_auto_type
    PRIVATE cxx_nullptr
    PRIVATE cxx_range_for
    )

target_link_libraries(editor
    ${OPENGL_LIBRARIES}
    )
