cmake_minimum_required(VERSION 3.10)

project(tissue-fem)

# Find and build libraries

add_subdirectory(src)

add_subdirectory(libs)

# include resources to project
set(RESOURCES_PATH "resources")
set(SHADER_PATH "${RESOURCES_PATH}/shaders")

set(COPY_DATA

    ${SHADER_PATH}/mesh.vert   ${SHADER_PATH}/mesh.frag
)

foreach(data ${COPY_DATA})
    configure_file(${data} ${data} COPYONLY)
endforeach()