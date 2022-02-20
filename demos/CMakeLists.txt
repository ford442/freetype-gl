# Freetype GL - A C OpenGL Freetype engine
#
# Distributed under the OSI-approved BSD 2-Clause License.  See accompanying
# file `LICENSE` for more details.

macro(CREATE_DEMO _target _sources)
    add_executable(${_target} ${_sources} $<TARGET_OBJECTS:demo-utils>)
    if(${_target} MATCHES "^harfbuzz")
        target_link_libraries(${_target}
            freetype-gl-hb
            ${HARFBUZZ_LIBRARIES}
        )
    else()
        target_link_libraries(${_target}
            freetype-gl
        )
    endif()
    target_link_libraries(${_target}
        ${OPENGL_LIBRARY}
        ${MATH_LIBRARY}
    )

endmacro()


# Find dependencies
find_package(AntTweakBar)
find_package(Fontconfig)

add_library(demo-utils OBJECT
    mat4.h
    mat4.c
    screenshot-util.h
    screenshot-util.c
    shader.h
    shader.c
)

target_include_directories(demo-utils
    PUBLIC $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>
)

add_custom_command(
    DEPENDS
        makefont
        ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    COMMAND
        ../$<$<BOOL:MSVC>:$(Configuration)/>makefont
    ARGS
        --font fonts/Vera.ttf
        --header ${CMAKE_CURRENT_SOURCE_DIR}/vera-16.h
        --size 16
        --variable font
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/vera-16.h
)

add_custom_target(generate-embedded-font
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vera-16.h
)

create_demo(embedded-font embedded-font.c)
add_dependencies(embedded-font generate-embedded-font)

create_demo(cartoon cartoon.c)

if(FONTCONFIG_FOUND)
    include_directories(${FONTCONFIG_INCLUDE_DIR})
    create_demo(markup markup.c)
    target_link_libraries(markup ${Fontconfig_LIBRARY})
endif()

if(ANT_TWEAK_BAR_FOUND)
    include_directories(${ANT_TWEAK_BAR_INCLUDE_PATH})
    create_demo(atb-agg atb-agg.c)
    target_link_libraries(atb-agg ${ANT_TWEAK_BAR_LIBRARY})
    set_target_properties(atb-agg PROPERTIES LINKER_LANGUAGE CXX)
endif()

if(TARGET freetype-gl-hb)
    create_demo(harfbuzz harfbuzz.c)
    create_demo(harfbuzz-texture harfbuzz-texture.c)
    target_include_directories(harfbuzz
        BEFORE
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../harfbuzz
        PRIVATE ${HARFBUZZ_INCLUDE_DIRS}
    )
    target_include_directories(harfbuzz-texture
        BEFORE
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../harfbuzz
        PRIVATE ${HARFBUZZ_INCLUDE_DIRS}
    )
endif()

# Copy font and shaders into build directory for in-place testing
file(COPY ../fonts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ../data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ../shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

