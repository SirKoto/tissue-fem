project(ImGuizmo)

add_library(${PROJECT_NAME}
    GraphEditor.cpp GraphEditor.h
    ImCurveEdit.cpp ImCurveEdit.h
    ImGradient.cpp  ImGradient.h
    ImGuizmo.cpp    ImGuizmo.h
    ImSequencer.cpp ImSequencer.h
    ImZoomSlider.h
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} ImGui)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)
