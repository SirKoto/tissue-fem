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
    ${SHADER_PATH}/particles.vert   ${SHADER_PATH}/particles.frag
    ${SHADER_PATH}/generate_face_normal.geom
    ${SHADER_PATH}/simple_mesh.vert

    ${MODEL_PATH}/armadilloSimp/armadSimp.ele
    ${MODEL_PATH}/armadilloSimp/armadSimp.face
    ${MODEL_PATH}/armadilloSimp/armadSimp.node

    ${MODEL_PATH}/armadilloSimp/armadilloComp.ele
    ${MODEL_PATH}/armadilloSimp/armadilloComp.face
    ${MODEL_PATH}/armadilloSimp/armadilloComp.node

    ${MODEL_PATH}/sphere/sphere.ele
    ${MODEL_PATH}/sphere/sphere.face
    ${MODEL_PATH}/sphere/sphere.node
)

foreach(data ${COPY_DATA})
    configure_file(${data} ${data} COPYONLY)
endforeach()