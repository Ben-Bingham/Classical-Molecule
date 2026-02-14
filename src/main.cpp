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
#include <thread>

using namespace RenderingUtilities;

struct Rect {
    Transform transform;

    glm::vec3 color;
};

struct PointMass {
    float mass;
    glm::vec3 position;
    glm::vec3 velocity;
};

struct PointCharge : PointMass {
    float charge;
};

struct Nucleon : PointCharge { };

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

struct PhysicsState {
    std::vector<PointMass> pointMasses;
    std::vector<PointCharge> pointCharges;
    std::vector<Nucleon> nucleons;
} physicsState;

struct RenderState {
    std::vector<Rect> rects;
} renderState;

glm::vec3 NextPosition(int index, int max) {
    float s = glm::pow(max, 1.0f / 3.0f);
    int size = (int)glm::ceil(s);

    int x = index / (size * size);
    int y = (index / size) % size;
    int z = index % size;

    return glm::vec3{ (float)x, (float)y, (float)z };
}

void AddToState(int neutronCount, int protonCount, int electronCount) {
    int max = neutronCount + protonCount + electronCount;
    int j = 0;

    int neutronLeft = neutronCount;
    int protonLeft = protonCount;
    int electronLeft = electronCount;

    while (neutronLeft != 0 || protonLeft != 0 || electronLeft != 0) {
        if (neutronLeft > 0) {
            Nucleon n{ 200.0f, NextPosition(j, max), glm::vec3{ 0.0f }, 0.0f };

            physicsState.nucleons.push_back(n);
            ++j;
            --neutronLeft;
        }

        if (protonLeft > 0) {
            Nucleon p{ 200.0f, NextPosition(j, max), glm::vec3{ 0.0f }, 1.0f };

            physicsState.nucleons.push_back(p);
            ++j;
            --protonLeft;
        }

        if (electronLeft > 0) {
            PointCharge e{ 0.1f, NextPosition(j, max), glm::vec3{ 0.0f }, -1.0f };

            physicsState.pointCharges.push_back(e);
            ++j;
            --electronLeft;
        }
    }
}

int main() {
    AddToState(2, 2, 1);

    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        std::cout << "ERROR: Failed to initialize GLFW." << std::endl;
    }

    glm::ivec2 windowSize{ 1600, 900 };
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    GLFWwindow* window = glfwCreateWindow(windowSize.x, windowSize.y, "Classical Atom", nullptr, nullptr);

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
        -0.5f, -0.5f, -0.5f, 0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f, 0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f, 0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f, 0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
                                                 
        -0.5f, -0.5f,  0.5f, 0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f, 0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f, 0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, 0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
                                                 
        -0.5f,  0.5f,  0.5f, 1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, 1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, 1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
                                                 
         0.5f,  0.5f,  0.5f, 1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f, 1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f, 1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f, 1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
                                                 
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f, 0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f, 0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
                                                 
        -0.5f,  0.5f, -0.5f, 0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f, 0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f, 0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, 0.0f,  1.0f,  0.0f, 0.0f, 0.0f
    } };

    ElementBufferObject ebo{ std::vector<unsigned int>{
        2,  1,  0,
        0,  3,  2,

        4,  5,  6,
        6,  7,  4,

        8,  9, 10,
        10, 11,  8,

        14, 13, 12,
        12, 15, 14,

        16, 17, 18,
        18, 19, 16,

        22, 21, 20,
        20, 23, 22,
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

    glEnable(GL_DEPTH_TEST);
    glfwSwapInterval(1);

    const size_t physicsStateQueueSize = 4;
    std::array<PhysicsState, physicsStateQueueSize> physicsStateQueue;
    int mostRecentPhysicsState = 0;

    int newSceneProtonCount = 2;
    int newSceneNeutronCount = 2;
    int newSceneElectronCount = 1;

    float timeMultiplier = 1.0f;

    float dt = 1.0f / 1000.0f;

    bool closePhysicsThread = false;
    bool reloadScene = true;

    std::thread physicsThread{ [&]() {
        while (!closePhysicsThread) {
            TimeScope physicsTimeScope{ &physicsTime };

            if (reloadScene) {
                for (int i = 0; i < physicsStateQueueSize; ++i) {
                    physicsStateQueue[i] = PhysicsState{ };
                }

                physicsStateQueue[0] = physicsState;
                mostRecentPhysicsState = 0;

                reloadScene = false;
            }

            PhysicsState state = physicsStateQueue[mostRecentPhysicsState];

            std::vector<PointCharge*> chargedParticles{ };

            for (auto& pc : state.pointCharges) {
                chargedParticles.push_back(&pc);
            }

            for (auto& n : state.nucleons) {
                if (n.charge == 0.0f) continue;

                chargedParticles.push_back(&n);
            }

            for (int i = 0; i < chargedParticles.size(); ++i) {
                for (int j = 0; j < chargedParticles.size(); ++j) {
                    if (i == j) continue;

                    PointCharge& c1 = *chargedParticles[i];
                    PointCharge& c2 = *chargedParticles[j];

                    float distance = glm::distance(c1.position, c2.position);

                    float force = (c1.charge * c2.charge) / (distance * distance);

                    glm::vec3 direction = glm::normalize(c1.position - c2.position);

                    glm::vec3 accel = (force * direction) / c1.mass;

                    c1.velocity += accel * dt;
                }
            }

            for (auto& c : state.pointCharges) {
                c.position += c.velocity * dt;
            }

            for (int i = 0; i < state.nucleons.size(); ++i) {
                for (int j = 0; j < state.nucleons.size(); ++j) {
                    if (i == j) continue;

                    PointCharge& n1 = state.nucleons[i];
                    PointCharge& n2 = state.nucleons[j];

                    float distance = glm::distance(n1.position, n2.position);

                    // Using Yukawa Potential as an approximation:
                    // U(r) = (e^(-r)) / r
                    // -> F(r) = -(e^(-r) * r^(-1) + e^(-r) * r^(-2))

                    // Where r is the distance between the nucleons

                    // We then finally add 1 / r^(10) to the force to act as a repulsive core
                    // here 10 is an arbitrary large number

                    // F(r) = 1 / r^(10) -(e^(-r) * r^(-1) + e^(-r) * r^(-2))

                    float force = 1 / glm::pow(distance, 10.0f) - (glm::exp(distance) / (distance * distance) + (glm::exp(distance)) / (distance));

                    glm::vec3 direction = glm::normalize(n1.position - n2.position);

                    glm::vec3 accel = (force * direction) / n1.mass;

                    n1.velocity += accel * dt;
                }
            }

            for (auto& n : state.nucleons) {
                n.position += n.velocity * dt;
            }

            ++mostRecentPhysicsState;
            mostRecentPhysicsState %= physicsStateQueueSize;
            physicsStateQueue[mostRecentPhysicsState] = state;

            dt = physicsTime.count() * timeMultiplier;
        }
    } };

    while (!glfwWindowShouldClose(window)) {
        TimeScope frameTimeScope{ &frameTime };

        glfwPollEvents();

        glm::ivec2 mousePositionWRTViewport{ mousePosition.x - viewportOffset.x, lastFrameViewportSize.y - (viewportOffset.y - mousePosition.y) };

        MoveCamera(camera, window, static_cast<float>(frameTime.count()), mousePositionWRTViewport, lastFrameViewportSize, mouseOverViewPort);

        {
            TimeScope renderingTimeScope{ &renderTime };

            PhysicsState physState = physicsStateQueue[mostRecentPhysicsState];

            rendererTarget.Bind();

            glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            solidShader.Bind();
            
            renderState.rects.clear();

            for (const auto& pm : physState.pointMasses) {
                Transform t{ pm.position, glm::vec3{ 0.6f } };

                Rect r{ t, glm::vec3{ 0.0f } };
                renderState.rects.push_back(r);
            }

            for (const auto& pc : physState.pointCharges) {
                Transform t{ pc.position, glm::vec3{ 0.4f } };

                glm::vec3 color;

                if (pc.charge > 0.0f) { color = glm::vec3{ 0.0f, 0.0f, 1.0f }; }
                else { color = glm::vec3{ 1.0f, 0.0f, 0.0f }; }

                Rect r{ t, color };
                renderState.rects.push_back(r);
            }

            for (const auto& n : physState.nucleons) {
                Transform t{ n.position, glm::vec3{ 0.5f } };

                glm::vec3 color;

                if (n.charge == 0.0f) { color = glm::vec3{ 1.0f, 1.0f, 1.0f }; }
                else if (n.charge > 0.0f) { color = glm::vec3{ 1.0f, 1.0f, 0.0f }; }
                else { color = glm::vec3{ 1.0f, 0.0f, 1.0f }; }

                Rect r{ t, color };
                renderState.rects.push_back(r);
            }

            for (auto& r : renderState.rects) {
                solidShader.SetVec3("color", r.color);

                glm::mat4 projection = glm::perspective(glm::radians(camera.fov), (float)rendererTarget.GetSize().x / (float)rendererTarget.GetSize().y, camera.nearPlane, camera.farPlane);
                
                r.transform.CalculateMatrix();
                glm::mat4 mvp = projection * camera.View() * r.transform.matrix;

                solidShader.SetMat4("mvp", mvp);

                vao.Bind();
                glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
            }

            rendererTarget.Unbind();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //ImGui::ShowDemoWindow();
        //ImGui::ShowMetricsWindow();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        { ImGui::Begin("Scene");
            float frameRate = 1.0f / frameTime.count();
            float physicsFrameRate = 1.0f / physicsTime.count();

            ImGui::Text("Total Framerate: %10.2f", frameRate);
            ImGui::Text("Physics Framerate: %10.2f", physicsFrameRate);

            ImGui::Separator();

            ImGui::DragFloat("Time Multiplier", &timeMultiplier, 0.001f, 0.0000f, 1000.0f);

            ImGui::Separator();

            ImGui::DragInt("Protons", &newSceneProtonCount, 0.1f, 0, 100);
            ImGui::DragInt("Neutrons", &newSceneNeutronCount, 0.1f, 0, 100);
            ImGui::DragInt("Electrons", &newSceneElectronCount, 0.1f, 0, 100);

            if (ImGui::Button("Clear")) {
                physicsState = PhysicsState{ };
                reloadScene = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Load")) {
                physicsState = PhysicsState{ };
                AddToState(newSceneNeutronCount, newSceneProtonCount, newSceneElectronCount);
                reloadScene = true;
            }
        } ImGui::End();

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

    closePhysicsThread = true;
    physicsThread.join();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwTerminate();
}