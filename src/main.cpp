/**
* Application entry point and ImGui setup
**/

#include <cstdio>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include "proggy_vector.h"
#include "app.hpp"

#ifdef _WIN32
#include <windows.h>
#define USE_IMGUI_VIEWPORTS
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static bool init_window_pos = true;
static Application *application = nullptr;

void frame(GLFWwindow *window)
{
    bool p_open = true;

    if (init_window_pos)
    {
        init_window_pos = false;

#ifdef USE_IMGUI_VIEWPORTS
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();

        int monitor_pos[2];
        int monitor_size[2];
        glfwGetMonitorWorkarea(monitor, &monitor_pos[0], &monitor_pos[1], &monitor_size[0], &monitor_size[1]);
        
        ImVec2 display_size = ImVec2(monitor_size[0], monitor_size[1]);
#else
        ImVec2 display_size = ImGui::GetMainViewport()->WorkSize;
#endif

        ImVec2 window_size = ImVec2(ImGui::GetFontSize() * 50.0f, ImGui::GetFontSize() * 30.0f);
        ImGui::SetNextWindowSize(window_size);
        ImGui::SetNextWindowPos(ImVec2((display_size.x - window_size.x) / 2.0f, (display_size.y - window_size.y) / 2.0f));

        init_window_pos = false;
    }

    if (ImGui::Begin("Rained Version Manager", &p_open, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar))
    {
        application->render_main_window();
    } ImGui::End();

    if (!p_open)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

int entry()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, false);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    const int WINDOW_WIDTH = 920;
    const int WINDOW_HEIGHT = 720;
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Rained Version Manager", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);

    {
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();

        int monitor_pos[2];
        int monitor_size[2];
        glfwGetMonitorWorkarea(monitor, &monitor_pos[0], &monitor_pos[1], &monitor_size[0], &monitor_size[1]);

        glfwSetWindowPos(window, (monitor_size[0] - WINDOW_WIDTH) / 2, (monitor_size[1] - WINDOW_HEIGHT) / 2);
    }

    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr; // disable .ini
    io.LogFilename = nullptr;

#ifdef USE_IMGUI_VIEWPORTS
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigViewportsNoAutoMerge = true;
#endif

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);

    // if the user is on a high-dpi monitor, use a font that is better suited
    // for scaling
    if (yscale != 1.0f)
    {
        // create copy of font data on heap, because imgui will try to free it when
        // context is destroyed
        unsigned char *font_data = new unsigned char[PROGGY_VECTOR_TTF_LEN];
        memcpy(font_data, PROGGY_VECTOR_TTF, PROGGY_VECTOR_TTF_LEN);

        ImFont *font = io.Fonts->AddFontFromMemoryTTF(font_data, PROGGY_VECTOR_TTF_LEN, 13.0f * yscale);
        io.FontDefault = font;
    }

    application = new Application;

#if !defined(USE_IMGUI_VIEWPORTS)
    glfwShowWindow(window);
#endif
    
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        frame(window);

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#ifdef USE_IMGUI_VIEWPORTS
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
#endif

        glfwSwapBuffers(window);
    }

    delete application;

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

// Main code
int main(int, char**)
{
    return entry();
}

#ifdef _WIN32
int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
    return entry();
}
#endif
