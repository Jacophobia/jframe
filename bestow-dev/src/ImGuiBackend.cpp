// bestow-dev/src/ImGuiBackend.cpp
// ImGui backend integration for GLFW + OpenGL3

module;

#include <imgui.h>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

module bestow.dev;

namespace bestow::dev {

//==========================================================================
// OpenGL3 Backend State
//==========================================================================

struct OpenGL3Backend {
    GLuint fontTexture = 0;
    GLuint shaderHandle = 0;
    GLuint vertHandle = 0;
    GLuint fragHandle = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint vao = 0;
    GLint attribLocationTex = 0;
    GLint attribLocationProjMtx = 0;
    GLint attribLocationVtxPos = 0;
    GLint attribLocationVtxUV = 0;
    GLint attribLocationVtxColor = 0;
};

//==========================================================================
// GLFW Backend State
//==========================================================================

struct GLFWBackend {
    GLFWwindow* window = nullptr;
    double time = 0.0;
    bool mouseJustPressed[5] = {false, false, false, false, false};
    GLFWcursor* mouseCursors[ImGuiMouseCursor_COUNT] = {};
};

static OpenGL3Backend g_gl3;
static GLFWBackend g_glfw;

//==========================================================================
// OpenGL3 Backend Implementation
//==========================================================================

static void createDeviceObjects() {
    // Shader sources
    const GLchar* vertexShader =
        "#version 330 core\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main() {\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
        "}\n";

    const GLchar* fragmentShader =
        "#version 330 core\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main() {\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    // Create shaders
    g_gl3.vertHandle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(g_gl3.vertHandle, 1, &vertexShader, nullptr);
    glCompileShader(g_gl3.vertHandle);

    g_gl3.fragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(g_gl3.fragHandle, 1, &fragmentShader, nullptr);
    glCompileShader(g_gl3.fragHandle);

    g_gl3.shaderHandle = glCreateProgram();
    glAttachShader(g_gl3.shaderHandle, g_gl3.vertHandle);
    glAttachShader(g_gl3.shaderHandle, g_gl3.fragHandle);
    glLinkProgram(g_gl3.shaderHandle);

    g_gl3.attribLocationTex = glGetUniformLocation(g_gl3.shaderHandle, "Texture");
    g_gl3.attribLocationProjMtx = glGetUniformLocation(g_gl3.shaderHandle, "ProjMtx");
    g_gl3.attribLocationVtxPos = glGetAttribLocation(g_gl3.shaderHandle, "Position");
    g_gl3.attribLocationVtxUV = glGetAttribLocation(g_gl3.shaderHandle, "UV");
    g_gl3.attribLocationVtxColor = glGetAttribLocation(g_gl3.shaderHandle, "Color");

    // Create buffers
    glGenBuffers(1, &g_gl3.vbo);
    glGenBuffers(1, &g_gl3.ebo);
    glGenVertexArrays(1, &g_gl3.vao);

    // Build font atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    glGenTextures(1, &g_gl3.fontTexture);
    glBindTexture(GL_TEXTURE_2D, g_gl3.fontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    io.Fonts->SetTexID((ImTextureID)(intptr_t)g_gl3.fontTexture);
}

static void destroyDeviceObjects() {
    if (g_gl3.vbo) glDeleteBuffers(1, &g_gl3.vbo);
    if (g_gl3.ebo) glDeleteBuffers(1, &g_gl3.ebo);
    if (g_gl3.vao) glDeleteVertexArrays(1, &g_gl3.vao);
    if (g_gl3.shaderHandle) glDeleteProgram(g_gl3.shaderHandle);
    if (g_gl3.vertHandle) glDeleteShader(g_gl3.vertHandle);
    if (g_gl3.fragHandle) glDeleteShader(g_gl3.fragHandle);
    if (g_gl3.fontTexture) {
        glDeleteTextures(1, &g_gl3.fontTexture);
        ImGui::GetIO().Fonts->SetTexID(0);
    }
    g_gl3 = OpenGL3Backend{};
}

//==========================================================================
// GLFW Backend Implementation
//==========================================================================

static void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button >= 0 && button < 5) {
        if (action == GLFW_PRESS) {
            g_glfw.mouseJustPressed[button] = true;
        }
    }
}

static void glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += (float)xoffset;
    io.MouseWheel += (float)yoffset;
}

static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();

    // Modern ImGui uses AddKeyEvent
    // Keys are handled via character input and special keys via AddKeyEvent in newer versions
    // For simplicity, we let ImGui handle keyboard via char callback
    (void)key;
    (void)scancode;
    (void)action;

    // Set modifier flags
    io.AddKeyEvent(ImGuiMod_Ctrl, (mods & GLFW_MOD_CONTROL) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (mods & GLFW_MOD_SHIFT) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (mods & GLFW_MOD_ALT) != 0);
    io.AddKeyEvent(ImGuiMod_Super, (mods & GLFW_MOD_SUPER) != 0);
}

static void glfwCharCallback(GLFWwindow* window, unsigned int c) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(c);
}

static void updateMouseData() {
    ImGuiIO& io = ImGui::GetIO();

    for (int i = 0; i < 5; i++) {
        io.MouseDown[i] = g_glfw.mouseJustPressed[i] ||
                          glfwGetMouseButton(g_glfw.window, i) != 0;
        g_glfw.mouseJustPressed[i] = false;
    }

    double mouse_x, mouse_y;
    glfwGetCursorPos(g_glfw.window, &mouse_x, &mouse_y);
    io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
}

static void updateMouseCursor() {
    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) ||
        glfwGetInputMode(g_glfw.window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        return;
    }

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
        glfwSetInputMode(g_glfw.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    } else {
        glfwSetCursor(g_glfw.window,
                      g_glfw.mouseCursors[imgui_cursor] ?
                      g_glfw.mouseCursors[imgui_cursor] :
                      g_glfw.mouseCursors[ImGuiMouseCursor_Arrow]);
        glfwSetInputMode(g_glfw.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

//==========================================================================
// Public API
//==========================================================================

void initializeImGui(GLFWwindow* window) {
    g_glfw.window = window;

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

    // Setup style
    ImGui::StyleColorsDark();

    // Note: KeyMap is deprecated in ImGui 1.87+, modern ImGui uses io.AddKeyEvent()
    // We rely on the callback system instead

    // Setup mouse cursors
    g_glfw.mouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    g_glfw.mouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    g_glfw.mouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    g_glfw.mouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    g_glfw.mouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

    // Setup GLFW callbacks
    glfwSetMouseButtonCallback(window, glfwMouseButtonCallback);
    glfwSetScrollCallback(window, glfwScrollCallback);
    glfwSetKeyCallback(window, glfwKeyCallback);
    glfwSetCharCallback(window, glfwCharCallback);

    // Create OpenGL objects
    createDeviceObjects();
}

void beginImGuiFrame() {
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(g_glfw.window, &w, &h);
    glfwGetFramebufferSize(g_glfw.window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    if (w > 0 && h > 0) {
        io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);
    }

    // Setup time step
    double current_time = glfwGetTime();
    io.DeltaTime = g_glfw.time > 0.0 ? (float)(current_time - g_glfw.time) : (float)(1.0f / 60.0f);
    g_glfw.time = current_time;

    // Update mouse
    updateMouseData();
    updateMouseCursor();

    // Start new frame
    ImGui::NewFrame();
}

void renderImGui() {
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    // Avoid rendering when minimized
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) return;

    // Backup GL state
    GLint last_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_array_buffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_vertex_array;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4];
    glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    // Setup render state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, fb_width, fb_height);
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    const float ortho_projection[4][4] = {
        {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f},
    };

    glUseProgram(g_gl3.shaderHandle);
    glUniform1i(g_gl3.attribLocationTex, 0);
    glUniformMatrix4fv(g_gl3.attribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);

    glBindVertexArray(g_gl3.vao);

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        glBindBuffer(GL_ARRAY_BUFFER, g_gl3.vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
                     (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_gl3.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),
                     (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        glEnableVertexAttribArray(g_gl3.attribLocationVtxPos);
        glEnableVertexAttribArray(g_gl3.attribLocationVtxUV);
        glEnableVertexAttribArray(g_gl3.attribLocationVtxColor);
        glVertexAttribPointer(g_gl3.attribLocationVtxPos, 2, GL_FLOAT, GL_FALSE,
                              sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, pos));
        glVertexAttribPointer(g_gl3.attribLocationVtxUV, 2, GL_FLOAT, GL_FALSE,
                              sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, uv));
        glVertexAttribPointer(g_gl3.attribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                              sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, col));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

            ImVec4 clip_rect;
            clip_rect.x = (pcmd->ClipRect.x - draw_data->DisplayPos.x) * draw_data->FramebufferScale.x;
            clip_rect.y = (pcmd->ClipRect.y - draw_data->DisplayPos.y) * draw_data->FramebufferScale.y;
            clip_rect.z = (pcmd->ClipRect.z - draw_data->DisplayPos.x) * draw_data->FramebufferScale.x;
            clip_rect.w = (pcmd->ClipRect.w - draw_data->DisplayPos.y) * draw_data->FramebufferScale.y;

            if (clip_rect.x < fb_width && clip_rect.y < fb_height &&
                clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
                glScissor((int)clip_rect.x, (int)(fb_height - clip_rect.w),
                          (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));

                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                               sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                               (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
            }
        }
    }

    // Restore modified GL state
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2],
               (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2],
              (GLsizei)last_scissor_box[3]);
    if (last_enable_blend)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
    if (last_enable_cull_face)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
    if (last_enable_depth_test)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
}

void shutdownImGui() {
    // Destroy cursors
    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++) {
        if (g_glfw.mouseCursors[cursor_n]) {
            glfwDestroyCursor(g_glfw.mouseCursors[cursor_n]);
            g_glfw.mouseCursors[cursor_n] = nullptr;
        }
    }

    // Destroy OpenGL objects
    destroyDeviceObjects();

    // Destroy ImGui context
    ImGui::DestroyContext();

    g_glfw = GLFWBackend{};
}

}  // namespace bestow::dev
