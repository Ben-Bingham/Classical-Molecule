#include <iostream>

#include <gl/glew.h>
#include <GLFW/glfw3.h>

#include <glm/ext/matrix_clip_space.hpp>

#include <imgui.h>
#include <implot.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <utility/OpenGl/Shader.h>
#include <utility/OpenGl/VertexAttributeObject.h>
#include <utility/OpenGl/Buffer.h>
#include <utility/OpenGl/GLDebug.h>
#include <utility/OpenGl/RenderTarget.h>

#include <utility/Camera.h>
#include <utility/TimeScope.h>
#include <utility/Transform.h>
#include <glm/ext/quaternion_trigonometric.hpp>

using namespace RenderingUtilities;

struct Rect {
    Transform transform;

    glm::vec3 color;
};

struct PointMass {
    float mass;
    glm::vec2 position;
    glm::vec2 velocity;
};

struct Spring {
    float k;
    float length;
    const float restLength;

    PointMass* end1;
    PointMass* end2;
};

void glfwErrorCallback(int error, const char* description) {
    std::cout << "ERROR: GLFW has thrown an error: " << std::endl;
    std::cout << description << std::endl;
}

glm::ivec2 mousePosition{ };
void mouseMoveCallback(GLFWwindow* window, double x, double y) {
    mousePosition.x = static_cast<int>(x);
    mousePosition.y = static_cast<int>(y);
}

void MoveCamera(Camera& camera, GLFWwindow* window, float dt, const glm::ivec2& mousePositionWRTViewport, const glm::ivec2& viewportSize, bool mouseOverViewport) {
    static bool mouseDown{ false };
    static bool hasMoved{ false };
    static glm::ivec2 lastMousePosition{ };

    if (!hasMoved) {
        lastMousePosition = mousePositionWRTViewport;
        hasMoved = true;
    }

    bool positionChange{ false };
    bool directionChange{ false };
    const float velocity = camera.speed * dt;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        positionChange = true;
        camera.position += camera.frontVector * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        positionChange = true;
        camera.position -= camera.frontVector * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        positionChange = true;
        camera.position += camera.rightVector * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        positionChange = true;
        camera.position -= camera.rightVector * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        positionChange = true;
        camera.position += camera.upVector * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        positionChange = true;
        camera.position -= camera.upVector * velocity;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
        if (mouseDown == false) {
            lastMousePosition.x = mousePositionWRTViewport.x;
            lastMousePosition.y = mousePositionWRTViewport.y;
        }

        mouseDown = true;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE) {
        mouseDown = false;
    }

    if (!mouseOverViewport) {
        hasMoved = false;
    }

    if (mouseDown && mouseOverViewport) {
        const float xDelta = (float)mousePositionWRTViewport.x - (float)lastMousePosition.x;
        const float yDelta = (float)lastMousePosition.y - (float)mousePositionWRTViewport.y;

        camera.yaw += xDelta * camera.lookSensitivity;
        camera.pitch += yDelta * camera.lookSensitivity;

        if (camera.pitch > 89.9f) {
            camera.pitch = 89.9f;
        }
        else if (camera.pitch < -89.9f) {
            camera.pitch = -89.9f;
        }

        directionChange = true;
    }

    if (mouseDown && mouseOverViewport) {
        camera.frontVector.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        camera.frontVector.y = sin(glm::radians(camera.pitch));
        camera.frontVector.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        camera.frontVector = glm::normalize(camera.frontVector);

        camera.rightVector = glm::normalize(glm::cross(camera.frontVector, camera.upVector));

        lastMousePosition.x = mousePositionWRTViewport.x;
        lastMousePosition.y = mousePositionWRTViewport.y;
    }
}

std::vector<Rect> rects;
std::vector<PointMass> pointMasses;
std::vector<Spring> springs;

int main() {
    PointMass pm1{ 100000000000.0f, glm::vec2{ -5.5f, -2.0f }, glm::vec2{ 0.0f } };
    PointMass pm2{ 2.0f, glm::vec2{ 5.5f, 1.0f }, glm::vec2{ 0.0f } };

    pointMasses.push_back(pm1);
    pointMasses.push_back(pm2);

    Spring spring{ 0.5f, 2.5f, 7.5f, &pointMasses[0], &pointMasses[1] };

    springs.push_back(spring);

    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        std::cout << "ERROR: Failed to initialize GLFW." << std::endl;
    }

    glm::ivec2 windowSize{ 1600, 900 };
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    GLFWwindow* window = glfwCreateWindow(windowSize.x, windowSize.y, "Rutile", nullptr, nullptr);

    if (!window) {
        std::cout << "ERROR: Failed to create window." << std::endl;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cout << "ERROR: Failed to initialize GLEW." << std::endl;
    }

    glfwSetCursorPosCallback(window, mouseMoveCallback);

    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.FontGlobalScale = 2.0f;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    glm::ivec2 defaultFramebufferSize{ 800, 600 };
    glm::ivec2 lastFrameViewportSize{ defaultFramebufferSize };

    RenderTarget rendererTarget{ defaultFramebufferSize };

    Shader solidShader{
        "assets\\shaders\\solid.vert",
        "assets\\shaders\\solid.frag"
    };

    Camera camera{ };

    VertexAttributeObject vao{ };

    // TODO remove uv and normal
    VertexBufferObject vbo{ std::vector<float>{
        -0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
        -0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
        -0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
         0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
    } };

    ElementBufferObject ebo{ std::vector<unsigned int>{
        2, 1, 0,
        5, 4, 3
    } };

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    vao.Unbind();
    vbo.Unbind();
    ebo.Unbind();

    Transform transform{ };
    transform.position = glm::vec3{ 0.0f, 0.0f, 5.0f };

    std::chrono::duration<double> frameTime{ };
    std::chrono::duration<double> renderTime{ };
    std::chrono::duration<double> physicsTime{ };

    bool mouseOverViewPort{ false };
    glm::ivec2 viewportOffset{ 0, 0 };

    float dt = 1.0f / 60.0f;

    while (!glfwWindowShouldClose(window)) {
        TimeScope frameTimeScope{ &frameTime };

        glfwPollEvents();

        glm::ivec2 mousePositionWRTViewport{ mousePosition.x - viewportOffset.x, lastFrameViewportSize.y - (viewportOffset.y - mousePosition.y) };

        MoveCamera(camera, window, static_cast<float>(frameTime.count()), mousePositionWRTViewport, lastFrameViewportSize, mouseOverViewPort);

        {
            TimeScope physicsTimeScope{ &physicsTime };

            for (auto& spring : springs) {
                float deltaX = spring.length - spring.restLength;

                float force = deltaX * spring.k;

                glm::vec2 end1ToEnd2 = spring.end1->position - spring.end2->position;
                float springLength = glm::length(end1ToEnd2);
                end1ToEnd2 = glm::normalize(end1ToEnd2);

                glm::vec2 end1Accel = (-end1ToEnd2 * force) / spring.end1->mass;
                glm::vec2 end2Accel = (end1ToEnd2 * force) / spring.end2->mass;

                spring.end1->velocity += end1Accel * dt;
                spring.end2->velocity += end2Accel * dt;

                spring.end1->position += spring.end1->velocity * dt;
                spring.end2->position += spring.end2->velocity * dt;

                spring.length = springLength;
            }
        }

        {
            TimeScope renderingTimeScope{ &renderTime };

            rendererTarget.Bind();

            glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            solidShader.Bind();
            
            rects.clear();

            for (const auto& spring : springs) {
                glm::vec2 end1Pos = spring.end1->position;
                glm::vec2 end2Pos = spring.end2->position;

                glm::vec2 center = (end1Pos + end2Pos) / 2.0f;

                glm::vec2 v1 = end1Pos;
                glm::vec2 v2 = end2Pos;

                v1 = v1 - v2;
                v2 = glm::vec2{ 1.0f, 0.0f };

                v1 = glm::normalize(v1);
                v2 = glm::normalize(v2);

                float angle = glm::acos(glm::dot(v1, v2));

                Transform t{ glm::vec3{ center, 0.0f }, glm::vec3{ spring.length, 0.3f, 0.0f }, glm::angleAxis(-angle, glm::vec3{ 0.0f, 0.0f, 1.0f })};

                Rect r{ t, glm::vec3{ 1.0f, 0.0f, 0.0f } };
                rects.push_back(r);
            }

            for (const auto& pm : pointMasses) {
                Transform t{ glm::vec3{ pm.position, 0.0f }, glm::vec3{ 0.6f } };

                Rect r{ t, glm::vec3{ 0.0f } };
                rects.push_back(r);
            }

            for (auto& r : rects) {
                solidShader.SetVec3("color", r.color);

                glm::mat4 projection = glm::perspective(glm::radians(camera.fov), (float)rendererTarget.GetSize().x / (float)rendererTarget.GetSize().y, camera.nearPlane, camera.farPlane);
                
                r.transform.CalculateMatrix();
                glm::mat4 mvp = projection * camera.View() * r.transform.matrix;

                solidShader.SetMat4("mvp", mvp);

                vao.Bind();
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            }

            rendererTarget.Unbind();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //ImGui::ShowDemoWindow();
        //ImGui::ShowMetricsWindow();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        glm::ivec2 newViewportSize{ };

        { ImGui::Begin("Viewport");
            // Needs to be the first call after "Begin"
            newViewportSize = glm::ivec2{ ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };

            // Display the frame with the last frames viewport size (The same size it was rendered with)
            ImGui::Image((ImTextureID)rendererTarget.GetTexture().Get(), ImVec2{ (float)lastFrameViewportSize.x, (float)lastFrameViewportSize.y }, ImVec2{ 0.0f, 1.0f }, ImVec2{ 1.0f, 0.0f });

            mouseOverViewPort = ImGui::IsItemHovered();

            viewportOffset = glm::ivec2{ (int)ImGui::GetCursorPos().x, (int)ImGui::GetCursorPos().y };

        } ImGui::End(); // Viewport

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        const ImGuiIO& io = ImGui::GetIO();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* currentContextBackup = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(currentContextBackup);
        }

        // After ImGui has rendered its frame, we resize the framebuffer if needed for next frame
        if (newViewportSize != lastFrameViewportSize) {
            rendererTarget.Resize(newViewportSize);
        }

        lastFrameViewportSize = newViewportSize;

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwTerminate();
}