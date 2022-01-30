project(ImGuiFileDialog)

add_library(${PROJECT_NAME}
    ImGuiFileDialog.cpp
    ImGuiFileDialog.h
    ImGuiFileDialogConfig.h
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    USE_STD_FILESYSTEM
    $<$<CXX_COMPILER_ID:MSVC>:MSVC _CRT_SECURE_NO_WARNINGS>
)


target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_link_libraries(${PROJECT_NAME} ImGui)
