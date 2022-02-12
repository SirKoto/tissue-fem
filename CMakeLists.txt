cmake_minimum_required(VERSION 3.10)

project(tissue-fem)

option(USE_RELATIVE_PATH "Use the relative path for finding resources" OFF)

# Find and build libraries

add_subdirectory(src)

add_subdirectory(libs)

# include resources to project
set(RESOURCES_PATH "resources")
set(SHADER_PATH "${RESOURCES_PATH}/shaders")
set(MODEL_PATH "${RESOURCES_PATH}/models")


set(COPY_DATA
    ${SHADER_PATH}/mesh.vert   ${SHADER_PATH}/mesh.frag

    ${MODEL_PATH}/armadilloSimp/armadSimp.ele
    ${MODEL_PATH}/armadilloSimp/armadSimp.face
    ${MODEL_PATH}/armadilloSimp/armadSimp.node
)

foreach(data ${COPY_DATA})
    configure_file(${data} ${data} COPYONLY)
endforeach()