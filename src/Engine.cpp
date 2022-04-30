#include "Engine.hpp"

#include "Context.hpp"
#include "utils/serialization.hpp"
#include <fstream>


static void glfw_error_callback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void GLAPIENTRY
gl_message_callback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        type, severity, message);
}

Engine::Engine() : m_ctx(std::make_unique<Context>(this))
{
    // Setup window
    {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) {
            
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_SAMPLES, 4);

        m_window = glfwCreateWindow(1280, 720, "Tissue Simulator", NULL, NULL);
        if (m_window == NULL) {
            m_error = true;
            return;
        }
        glfwMakeContextCurrent(m_window);

        // glfwSwapInterval(0); // Disable vsync
    }

    bool err = gladLoadGL() == 0;
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        m_error = true;
        return;
    }

    // Enable OpenGL debug
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(gl_message_callback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, false);
    glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, nullptr, true);
    glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, GL_DONT_CARE, 0, nullptr, true);
    glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, GL_DONT_CARE, 0, nullptr, true);
    glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PERFORMANCE, GL_DONT_CARE, 0, nullptr, true);

    // Cull back-faces
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // disable .ini file
    io.IniFilename = nullptr;
    io.FontAllowUserScaling = true; // zoom wiht ctrl + mouse wheel 

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 430 core");

    // Setup imgizmo
    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

    // Create Scene
    m_scene = std::make_unique<Scene>();
}

Engine::~Engine()
{
    ImPlot::DestroyContext();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Engine::run()
{

    while (!m_error && !glfwWindowShouldClose(m_window)) {


        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        const ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        ImGuizmo::BeginFrame();

        // Update global context
        m_ctx->update();

        m_ctx->draw_ui();

        m_scene->update_ui(*m_ctx);
        m_scene->update(*m_ctx);
        

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(m_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(m_ctx->get_clear_color().x, m_ctx->get_clear_color().y, m_ctx->get_clear_color().z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the scene
        glEnable(GL_DEPTH_TEST);
        m_scene->render(*m_ctx);

        // Render UI
        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }
}

bool Engine::reload_scene(std::string* error)
{
    assert(error != nullptr);
    

    if (!m_scene_path.has_extension()) {
        *error = "ERROR: File " + m_scene_path.string() + " does not have extension";
        return false;
    }

    bool read_json;
    if (m_scene_path.extension().compare(".json") == 0) {
        read_json = true;
    }
    else if (m_scene_path.extension().compare(".bin") == 0) {
        read_json = false;
    }
    else {
        *error = "ERROR: File " + m_scene_path.string() + " does not have .json or .bin";
        return false;
    }

    // Create new scene
    m_scene = std::make_unique<Scene>();
    if (read_json) {
        std::ifstream stream(m_scene_path, std::ios::in);

        if (!stream) {
            *error = "ERROR: Can't open file " + m_scene_path.string();
            return false;
        }

        tf::JSONInputArchive ar(stream, *m_ctx, m_scene_path);

        ar(TF_SERIALIZE_NVP_MEMBER(m_scene));
    }
    else {
        std::ifstream stream(m_scene_path, std::ios::binary | std::ios::in);

        if (!stream) {
            *error = "ERROR: Can't open file " + m_scene_path.string();
            return false;
        }

        tf::BinaryInputArchive ar(stream, *m_ctx, m_scene_path);

        ar(TF_SERIALIZE_NVP_MEMBER(m_scene));
    }
    
    
    return true;
}

bool Engine::save_scene(std::string* error)
{
    if (!m_scene_path.has_extension()) {
        *error = "ERROR: File " + m_scene_path.string() + " does not have extension";
        return false;
    }

    bool read_json;
    if (m_scene_path.extension().compare(".json") == 0) {
        read_json = true;
    }
    else if (m_scene_path.extension().compare(".bin") == 0) {
        read_json = false;
    }
    else {
        *error = "ERROR: File " + m_scene_path.string() + " does not have .json or .bin";
        return false;
    }

    if (read_json) {
        std::ofstream stream(m_scene_path, std::ios::out | std::ios::trunc);

        if (!stream) {
            *error = "ERROR: Can't open file " + m_scene_path.string();
            return false;
        }

        tf::JSONOutputArchive ar(stream, *m_ctx, m_scene_path);

        ar(TF_SERIALIZE_NVP_MEMBER(m_scene));
    }
    else {
        std::ofstream stream(m_scene_path, std::ios::binary | std::ios::out | std::ios::trunc);

        if (!stream) {
            *error = "ERROR: Can't open file " + m_scene_path.string();
            return false;
        }

        tf::BinaryOutputArchive ar(stream, *m_ctx, m_scene_path);

        ar(TF_SERIALIZE_NVP_MEMBER(m_scene));
    }

    return true;
}
