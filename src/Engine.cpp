#include "Engine.hpp"

#include "Context.hpp"
#include "utils/serialization.hpp"
#include "utils/Timer.hpp"
#include <fstream>
#include <thread>
#ifdef _OPENMP
#include <omp.h>
#endif


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

#ifdef _OPENMP
    // Set parallelism
    omp_set_num_threads(std::thread::hardware_concurrency());
#endif

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

    // Search temporary path
    m_scene_path_tmp = std::filesystem::temp_directory_path() / "tmp_scene.bin";
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
    Timer timer;
    EngineTimings timings;
    while (!m_error && !glfwWindowShouldClose(m_window)) {


        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        timings.window_poll = (float)timer.getDuration<Timer::Seconds>().count(); timer.reset();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        const ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        ImGuizmo::BeginFrame();

        timings.imgui_new_frame = (float)timer.getDuration<Timer::Seconds>().count(); timer.reset();

        // Update global context
        m_ctx->update();

        m_ctx->draw_ui();

        timings.context_update = (float)timer.getDuration<Timer::Seconds>().count(); timer.reset();

        m_scene->update_ui(*m_ctx);

        timings.scene_draw_ui = (float)timer.getDuration<Timer::Seconds>().count(); timer.reset();

        m_scene->update(*m_ctx);

        timings.scene_times = m_scene->get_scene_time_update();

        if (m_first_simulation_frame) {
            m_first_simulation_frame = false;
            m_ctx->m_simulation_start_time = m_ctx->get_time();
            m_scene->start_simulation(*m_ctx);
            m_simulation_mode = true;
        }
        
        timings.scene_update = (float)timer.getDuration<Timer::Seconds>().count(); timer.reset();

        // Rendering
        ImGui::Render();

        timings.imgui_render_cpu = (float)timer.getDuration<Timer::Seconds>().count(); timer.reset();

        int display_w, display_h;
        glfwGetFramebufferSize(m_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(m_ctx->get_clear_color().x, m_ctx->get_clear_color().y, m_ctx->get_clear_color().z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the scene
        glEnable(GL_DEPTH_TEST);
        m_scene->render(*m_ctx);

        timings.scene_render = (float)timer.getDuration<Timer::Seconds>().count(); timer.reset();

        // Render UI
        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        timings.imgui_render_gpu = (float)timer.getDuration<Timer::Seconds>().count(); timer.reset();
        timings.time = m_ctx->get_time();

        m_ctx->m_engine_timings.push(timings);

        glfwSwapBuffers(m_window);
    }
}

bool Engine::reload_scene(const std::filesystem::path& path, std::string* error)
{
    assert(error != nullptr);
    

    if (!path.has_extension()) {
        *error = "ERROR: File " + path.string() + " does not have extension";
        return false;
    }

    bool read_json;
    if (path.extension().compare(".json") == 0) {
        read_json = true;
    }
    else if (path.extension().compare(".bin") == 0) {
        read_json = false;
    }
    else {
        *error = "ERROR: File " + path.string() + " does not have .json or .bin";
        return false;
    }

    // Create new scene
    m_scene = std::make_unique<Scene>();
    if (read_json) {
        std::ifstream stream(path, std::ios::in);

        if (!stream) {
            *error = "ERROR: Can't open file " + path.string();
            return false;
        }

        tf::JSONInputArchive ar(stream, *m_ctx, path);

        ar(TF_SERIALIZE_NVP_MEMBER(m_scene));
    }
    else {
        std::ifstream stream(path, std::ios::binary | std::ios::in);

        if (!stream) {
            *error = "ERROR: Can't open file " + path.string();
            return false;
        }

        tf::BinaryInputArchive ar(stream, *m_ctx, path);

        ar(TF_SERIALIZE_NVP_MEMBER(m_scene));
    }
    
    
    return true;
}

bool Engine::save_scene(const std::filesystem::path& path, std::string* error)
{
    if (!path.has_extension()) {
        *error = "ERROR: File " + path.string() + " does not have extension";
        return false;
    }

    bool read_json;
    if (path.extension().compare(".json") == 0) {
        read_json = true;
    }
    else if (path.extension().compare(".bin") == 0) {
        read_json = false;
    }
    else {
        *error = "ERROR: File " + path.string() + " does not have .json or .bin";
        return false;
    }

    if (read_json) {
        std::ofstream stream(path, std::ios::out | std::ios::trunc);

        if (!stream) {
            *error = "ERROR: Can't open file " + path.string();
            return false;
        }

        tf::JSONOutputArchive ar(stream, *m_ctx, path);

        ar(TF_SERIALIZE_NVP_MEMBER(m_scene));
    }
    else {
        std::ofstream stream(path, std::ios::binary | std::ios::out | std::ios::trunc);

        if (!stream) {
            *error = "ERROR: Can't open file " + path.string();
            return false;
        }

        tf::BinaryOutputArchive ar(stream, *m_ctx, path);

        ar(TF_SERIALIZE_NVP_MEMBER(m_scene));
    }

    return true;
}

void Engine::signal_start_simulation()
{
    m_first_simulation_frame = true;
}

void Engine::stop_simulation()
{
    m_simulation_mode = false;
    m_run_simulation = true;
    m_first_simulation_frame = false;
}
