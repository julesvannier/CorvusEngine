#include "CorvusEditor.h"

#include <random>
#include <set>
#include <sstream>

#include "Logger.h"
#include "ImGui/ImGuizmo.h"
#include <ImGui/imgui.h>

#include "Image.h"
#include "InputSystem.h"
#include "Rendering/LightingRenderPass.h"
#include "Rendering/ShaderCompiler.h"
#include "RHI/Buffer.h"
#include "RHI/Uploader.h"
#include "Rendering/RenderingLayouts.h"
#include "Rendering/SkyBoxRenderPass.h"
#include "Rendering/TransparencyRenderPass.h"
#include "RHI/D3D12Renderer.h"

CorvusEditor::CorvusEditor()
{
    LOG(Debug, "Starting Corvus Editor");

    Logger::DefineOnMessageLogged([this](LogType type, const std::string& content)
    {
        m_loggedMessages.emplace_back(std::make_tuple(type, content));
    });

    InputSystem::Create();
    InputSystem::Get()->AddListener(this);
    InputSystem::Get()->ShowCursor(false);

    int defaultWidth = 1380;
    int defaultHeight = 960;

    m_window = std::make_shared<Window>(defaultWidth, defaultHeight, L"Corvus Editor");
    m_window->DefineOnResize([this](int width, int height)
    {
        LOG(Debug, "Window resize !");
        m_renderer->Resize(width, height);
    });

    m_renderer = std::make_shared<D3D12Renderer>(m_window->GetHandle());

    m_resourceManager = std::make_shared<ResourcesManager>(m_renderer);

    m_shadowRenderPass = std::make_shared<ShadowRenderPass>();
    m_shadowRenderPass->Initialize(m_renderer, m_shadowMapResolution, m_shadowMapResolution);

    m_GBufferRenderPass = std::make_shared<GBufferRenderPass>();
    m_GBufferRenderPass->Initialize(m_renderer, defaultWidth, defaultHeight);

    m_SSAORenderPass = std::make_shared<SSAORenderPass>();
    m_SSAORenderPass->Initialize(m_renderer, defaultWidth / 2, defaultHeight / 2);

    m_deferredLightingPass = std::make_shared<LightingRenderPass>();
    m_deferredLightingPass->Initialize(m_renderer, defaultWidth, defaultHeight);

    m_skyboxPass = std::make_shared<SkyBoxRenderPass>();
    m_skyboxPass->Initialize(m_renderer, defaultWidth, defaultHeight);

    m_transparencyPass = std::make_shared<TransparencyRenderPass>();
    m_transparencyPass->Initialize(m_renderer, defaultWidth, defaultHeight);

    m_waterPass = std::make_shared<WaterRenderPass>();
    m_waterPass->Initialize(m_renderer, defaultWidth, defaultHeight);

    m_sceneRenderTexture = m_renderer->CreateTexture(defaultWidth, defaultHeight, TextureFormat::RGBA8, TextureType::RenderTarget);
    m_renderer->CreateRenderTargetView(m_sceneRenderTexture);
    m_renderer->CreateShaderResourceView(m_sceneRenderTexture);

    m_scene = std::make_shared<Scene>("DemoScene");

    // ----------------------------------------------- ASSETS DEMO ------------------------------------------------
    
    constexpr bool assetsDemo = true;
    if(assetsDemo)
    {
        AddModelToScene("SciFiHelmet", "Assets/SciFiHelmet.gltf", "Assets/SciFiHelmet_BaseColor.png",
            "Assets/SciFiHelmet_Normal.png", "Assets/SciFiHelmet_MetallicRoughness.png",
            { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
    
        AddLightToScene({ -1.5f, 1.0f, 0.0f }, {}, true);

        AddModelToScene("DamagedHelmet", "Assets/DamagedHelmet.gltf", "Assets/DamagedHelmet_albedo.jpg",
            "Assets/DamagedHelmet_normal.jpg", "Assets/DamagedHelmet_metalRoughness.jpg",
            { -3.0f, 0.0f, 0.0f }, { 90.0f, 0.0f, 0.0f });

        AddLightToScene({ -4.5f, 1.0f, 0.0f }, {}, true);

        AddModelToScene("Dragon", "Assets/dragon.obj", "", "", "",
            { -6.25f, -0.9f, 0.0f }, {}, { 0.25f, 0.25f, 0.25f });

        AddModelToScene("Dragon", "Assets/dragon.obj", "", "", "",
            { -10.2f, -0.9f, 0.0f }, {}, { 0.25f, 0.25f, 0.25f });

        AddModelToScene("Sphere", "Assets/sphere.gltf", "", "", "",
            { -13.5f, 0.0f, 0.0f }, {}, { 1.0f, 1.0f, 1.0f }, true);

        AddWaterToScene({ -5.0f, -0.1f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });

        AddModelToScene("Cube", "Assets/cube.obj", "", "", "",
            { -5.0f, -2.25f, 0.0f }, {}, { 12.0f, 0.5f, 6.8f });
    }
    
    // ----------------------------------------------- POINT LIGHTS DEMO ------------------------------------------------

    constexpr bool pointLightsDemo = false;
    if(pointLightsDemo)
    {
        m_dirLightIntensity = 0.1f;
        m_enableSkyBox = false;
        m_enablePointLights = true;
        m_enableShadows = false;
        
        constexpr float space = 3.0f;
        constexpr int row = 10;
        constexpr int column = 10;
    
        // AddModelToScene("Assets/cube.obj", "", "", { space * row/2, -0.5f, space * column/2 }, {}, { 25.0f, 0.2f, 25.0f });

        for(int i = 0; i < row; i++)
        {
            for(int j = 0; j < column; j++)
            {
                float posX = space * (float)i;
                float posZ = space * (float)j;

                if(i % 2 == 0)
                {
                    AddModelToScene("Dragon", "Assets/dragon.obj", "", "", "", { posX, 0.0f, posZ }, {}, { 0.25f, 0.25f, 0.25f });
                }
                else
                    AddLightToScene({ posX, 1.0f, posZ }, {}, true);
            }
        }
    }

    for(auto pair : m_meshesInstancesCount)
    {
        auto instancesBuffer = m_renderer->CreateBuffer(sizeof(InstanceData) * pair.second, sizeof(InstanceData), BufferType::Structured, false);
        m_meshesInstancesBuffers.emplace(pair.first, instancesBuffer);
    }

    m_startTime = clock();

    m_window->Maximize();

    uint32_t width, height;
    m_window->GetSize(width, height);
    
    InputSystem::Get()->SetCursorPosition(Vec2((float)width/2.0f, (float)height/2.0f));

    m_lastMousePos[0] = width/2.0f;
    m_lastMousePos[1] = height/2.0f;
}

CorvusEditor::~CorvusEditor()
{
    LOG(Debug, "Destroying Corvus Editor");

    InputSystem::Get()->RemoveListener(this);
    InputSystem::Release();
    
    Logger::WriteLogsToFile();
}

void CorvusEditor::Run()
{
    while(m_window->IsRunning())
    {
        InputSystem::Get()->Update();

        // ------------------------------------------------------------- Scene Constants Update --------------------------------------------------------------------
        
        float time = clock() - m_startTime;
        float dt = (time - m_lastTime) / 1000.0f;
        m_lastTime = time;
        m_elapsedTime += dt;
        
        uint32_t width, height;
        m_window->GetSize(width, height);

        if(m_fov != m_previousFov)
        {
            m_camera.UpdatePerspectiveFOV(m_fov * 3.14159f, m_viewportCachedSize.x / m_viewportCachedSize.y);
            m_previousFov = m_fov;
        }

        m_camera.Walk(m_cameraForward * (m_moveSpeed * dt));
        m_camera.Strafe(m_cameraRight * (m_moveSpeed * dt));

        m_camera.UpdateViewMatrix();
        m_camera.UpdateInvViewProjMatrix(m_viewportCachedSize.x, m_viewportCachedSize.y);

        // ----------------------------------------------------------- ECS data -> renderer ----------------------------------------------------------

        std::unordered_map<std::string, RenderMeshData> renderMeshesData;
        std::vector<PointLight> pointLights;
        for(const auto go : m_scene->m_gameObjects)
        {
            if(const auto tfComp = go->GetComponent<TransformComponent>())
            {
                if(const auto meshComp = go->GetComponent<MeshComponent>())
                {
                    auto renderItem = meshComp->GetRenderItem();
                    
                    if (renderItem->GetMaterial().IsTransparent && !m_renderTransparentObjects)
                        continue;

                    if (renderItem->GetMaterial().IsWater && !m_renderWater)
                        continue;
                    
                    auto idRmd = renderMeshesData.find(renderItem->GetMeshIdentifier());
                    if(idRmd != renderMeshesData.end()) // RMD already exists for this mesh, let's add the transform only
                    {
                        auto& rmd = idRmd->second;
                        rmd.InstancesTransforms.emplace_back(tfComp->m_transform);
                    }
                    else
                    {
                        RenderMeshData rmd;
                        rmd.MeshIdentifier = renderItem->GetMeshIdentifier();
                        rmd.InstancesTransforms.emplace_back(tfComp->m_transform);
                        rmd.Material = renderItem->GetMaterial();
                        rmd.Primitives = renderItem->GetPrimitives();
                        rmd.InstancesDataBuffer = m_meshesInstancesBuffers.find(renderItem->GetMeshIdentifier())->second;
                        renderMeshesData.emplace(renderItem->GetMeshIdentifier(), rmd);
                    }
                }

                if(const auto pointLightComp = go->GetComponent<PointLightComponent>())
                {
                    pointLightComp->m_pointLight.Position = { tfComp->m_transform.m[3][0], tfComp->m_transform.m[3][1], tfComp->m_transform.m[3][2] };
                    pointLights.emplace_back(pointLightComp->m_pointLight);
                }
            }
        }

        std::vector<RenderMeshData> RMDs;
        for(auto idRdm : renderMeshesData)
            RMDs.emplace_back(idRdm.second);
        
        // --------------------------------------------------------- Global Pass Datas -----------------------------------------------------------------
        std::vector<PointLight> pl = {};
        GlobalPassData passData = {};
        passData.DeltaTime = dt;
        passData.ElapsedTime = m_elapsedTime;
        passData.ViewMode = m_viewMode;
        passData.PointLights = m_enablePointLights ? pointLights : pl;
        passData.DirectionalInfo.Direction = { m_dirLightDirection[0], m_dirLightDirection[1], m_dirLightDirection[2] };
        passData.DirectionalInfo.Intensity = m_dirLightIntensity;
        passData.ViewportSizeX = m_viewportCachedSize.x;
        passData.ViewportSizeY = m_viewportCachedSize.y;
        passData.EnviroMaps = m_skyboxPass->GetEnvironmentMaps();
        passData.EnableShadows = m_enableShadows;
        passData.WaterParams = m_waterSettings;

        // ------------------------------------------------------------- Render Passes --------------------------------------------------------------------

        auto commandList = m_renderer->GetCurrentCommandList();
        auto backbuffer = m_renderer->GetBackBuffer();
        
        commandList->Begin();
        commandList->ImageBarrier(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->SetViewport(0, 0, width, height);
        commandList->BindRenderTargets({ backbuffer }, nullptr);
        commandList->ClearRenderTarget(backbuffer, 0.0f, 0.0f, 0.0f, 1.0f);
        commandList->ClearRenderTarget(m_sceneRenderTexture, 0.0f, 0.0f, 0.0f, 1.0f);

        RenderTargetInfo rtInfo;
        rtInfo.RenderTexture = m_sceneRenderTexture;
        
        if(m_enableShadows)
        {
            m_shadowRenderPass->Pass(m_renderer, passData, m_camera, RMDs, rtInfo);
            passData.ShadowMap = m_shadowRenderPass->GetShadowMap();
        }

        m_GBufferRenderPass->Pass(m_renderer, passData, m_camera, RMDs, rtInfo);
        passData.GBuffer = m_GBufferRenderPass->GetGBuffer();

        if(m_enableSSAO)
            m_SSAORenderPass->Pass(m_renderer, passData, m_camera, RMDs, rtInfo);
        
        m_deferredLightingPass->Pass(m_renderer, passData, m_camera, RMDs, rtInfo);

        if(m_enableSkyBox)
        {
            rtInfo.DepthBuffer = m_GBufferRenderPass->GetGBuffer().DepthBuffer;
            m_skyboxPass->Pass(m_renderer, passData, m_camera, {}, rtInfo);
        }

        if (m_renderTransparentObjects)
        {
            rtInfo.DepthBuffer = m_GBufferRenderPass->GetGBuffer().DepthBuffer;
            m_transparencyPass->Pass(m_renderer, passData, m_camera, RMDs, rtInfo);
        }

        if (m_renderWater)
        {
            rtInfo.DepthBuffer = m_GBufferRenderPass->GetGBuffer().DepthBuffer;
            m_waterPass->Pass(m_renderer, passData, m_camera, RMDs, rtInfo);
        }

        // ------------------------------------------------------------- UI Rendering --------------------------------------------------------------------
        
        commandList->BindRenderTargets({ backbuffer }, nullptr);
        m_renderer->BeginImGuiFrame();
        RenderUI((float)width, (float)height);
        m_renderer->EndImGuiFrame();
        
        commandList->ImageBarrier(backbuffer, D3D12_RESOURCE_STATE_PRESENT);
        commandList->End();
        m_renderer->ExecuteCommandBuffers({ commandList }, D3D12_COMMAND_LIST_TYPE_DIRECT);

        // m_renderer->WaitForGPU();

        m_renderer->Present(false);
        m_renderer->EndFrame();

        m_window->BroadCast();
    }
}

std::shared_ptr<GameObject> CorvusEditor::AddModelToScene(std::string name, const std::string& modelPath, const std::string& albedoPath, const std::string& normalPath,
    const std::string& mrPath, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale, bool transparent)
{
    auto model = m_resourceManager->LoadMesh(modelPath);

    Uploader uploader = m_renderer->CreateUploader();
    Image albedoImg;
    Image normalImg;
    Image mrImg;

    if(!albedoPath.empty())
    {
        auto albedoTexture = m_resourceManager->LoadTexture(albedoPath, uploader, albedoImg);
        model->GetMaterial().HasAlbedo = true;
        model->GetMaterial().Albedo = albedoTexture;
    }

    if(!normalPath.empty())
    {
        auto normalTexture = m_resourceManager->LoadTexture(normalPath, uploader, normalImg);
        model->GetMaterial().HasNormal = true;
        model->GetMaterial().Normal = normalTexture;
    }

    if(!mrPath.empty())
    {
        auto mrTexture = m_resourceManager->LoadTexture(mrPath, uploader, mrImg);
        model->GetMaterial().HasMetallicRoughness = true;
        model->GetMaterial().MetallicRoughness = mrTexture;
    }

    model->GetMaterial().IsTransparent = transparent;

    if(uploader.HasCommands())
        m_renderer->FlushUploader(uploader);

    auto idRmd = m_meshesInstancesCount.find(modelPath);
    if(idRmd != m_meshesInstancesCount.end())
    {
        int& count = idRmd->second;
        count++;
    }
    else
    {
        m_meshesInstancesCount.emplace(modelPath, 1);
    }

    auto go = m_scene->CreateGameObject(name, position, rotation, scale);
    auto meshComp = go->AddComponent<MeshComponent>();
    meshComp->SetRenderItem(model);
    
    return go;
}

std::shared_ptr<GameObject> CorvusEditor::AddWaterToScene(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale)
{
    auto quad = std::make_shared<RenderItem>();
    const std::string quadIdentifier = "QuadMesh";
    quad->SetPath(quadIdentifier);
    quad->CreateQuadMesh(m_renderer);

    quad->GetMaterial().IsTransparent = false;
    quad->GetMaterial().CastShadows = false;
    quad->GetMaterial().IsWater = true;
    
    auto idRmd = m_meshesInstancesCount.find(quadIdentifier);
    if(idRmd != m_meshesInstancesCount.end())
    {
        int& count = idRmd->second;
        count++;
    }
    else
    {
        m_meshesInstancesCount.emplace(quadIdentifier, 1);
    }

    auto go = m_scene->CreateGameObject("Quad", position, rotation, scale);
    auto meshComp = go->AddComponent<MeshComponent>();
    meshComp->SetRenderItem(quad);
    
    return go;
}

void CorvusEditor::RemoveModelFromScene(std::shared_ptr<GameObject> goToRemove)
{
    m_scene->RemoveGameObject(goToRemove);
}

std::shared_ptr<GameObject> CorvusEditor::AddLightToScene(DirectX::XMFLOAT3 position, DirectX::XMFLOAT4 color, bool randomColor)
{
    auto RandFloatRange = [](float min, float max) -> float
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> distribution(min, max);
        return distribution(gen);
    };
    
    PointLight pointLight;
    pointLight.Position = position;
    if(randomColor)
        pointLight.Color = { RandFloatRange(0.0f, 1.0f), RandFloatRange(0.0f, 1.0f), RandFloatRange(0.0f, 1.0f), 1.0f };
    else
        pointLight.Color = color;
    pointLight.ConstantAttenuation = m_defaultLightConstAttenuation;
    pointLight.LinearAttenuation = m_defaultLightLinearAttenuation;
    pointLight.QuadraticAttenuation = m_defaultLightQuadraticAttenuation;
    
    auto go = m_scene->CreateGameObject("PointLight", position);
    auto lightComp = go->AddComponent<PointLightComponent>();
    lightComp->m_pointLight = pointLight;

    return go;
}

void CorvusEditor::RenderUI(float width, float height)
{
    if(ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                m_window->Close();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    static bool dockspaceOpen = true;
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }
        
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;
        
    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        
    ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
    
    { // Dockspace scope
        if (!opt_padding)
        ImGui::PopStyleVar();
        
        if (opt_fullscreen)
            ImGui::PopStyleVar(2);
            
        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        ImGui::Begin("FrameRate");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        ImGui::Begin("Debug");
        ImGui::SliderFloat("FOV", &m_fov, 0.1f, 1.0f);
        ImGui::SliderFloat("Move Speed", &m_moveSpeed, 1.0f, 40.0f);
        static const char* modes[] = { "Default", "Albedo", "Normal", "Depth", "WorldPosition", "MetallicRoughness", "Debug" };
        ImGui::Combo("View Mode", (int*)&m_viewMode, modes, 7);
        ImGui::Separator();
        ImGui::SliderFloat3("DirLight Direction", m_dirLightDirection, -1.0f, 1.0f);
        ImGui::SliderFloat("DirLight Intensity", &m_dirLightIntensity, 0.0f, 3.0f);
        ImGui::Separator();
        ImGui::Checkbox("Enable SkyBox", &m_enableSkyBox);
        ImGui::Checkbox("Enable Point Lights", &m_enablePointLights);
        ImGui::Checkbox("Enable Shadows", &m_enableShadows);
        ImGui::Checkbox("Enable SSAO", &m_enableSSAO);
        ImGui::Checkbox("Translucancy pass", &m_renderTransparentObjects);
        ImGui::Checkbox("Water", &m_renderWater);
        ImGui::End();

        ImGui::Begin("Log");
        ImGui::Separator();
        if (ImGui::BeginChild("scrolling", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
        {
            for (const auto& [type, message] : m_loggedMessages)
            {
                switch (type)
                {
                    case LogType::Debug:
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), message.c_str());
                        break;
                    case LogType::Warning:
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), message.c_str());
                        break;
                    case LogType::Error:
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), message.c_str());
                        break;
                }
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();
        }
        ImGui::End();

        ImGui::Begin("SceneHierarchy");
        if(ImGui::BeginListBox("Objects"))
        {
            for(auto go : m_scene->m_gameObjects)
            {
                bool isSelected = false;
                if(m_selectedGo != nullptr)
                {
                    if(m_selectedGo->GetName() == go->GetName())
                        isSelected = true;
                }
        
                if(ImGui::Selectable(go->GetName().c_str(), isSelected))
                    m_selectedGo = go;
            }

            ImGui::EndListBox();
        }
        ImGui::Separator();
        if(m_selectedGo != nullptr)
        {
            ImGui::Text(m_selectedGo->GetName().c_str());
            ImGui::Spacing();
            if(auto tfComp = m_selectedGo->GetComponent<TransformComponent>())
            {
                ImGui::Text("Transform");
                ImGui::Spacing();
                
                float pos[3] = { tfComp->m_transform.m[3][0], tfComp->m_transform.m[3][1], tfComp->m_transform.m[3][2] };
                ImGui::InputFloat3("Position", pos);

                float scale[3] = { tfComp->m_transform.m[0][0], tfComp->m_transform.m[1][1], tfComp->m_transform.m[2][2] };
                ImGui::InputFloat3("Scale", scale);

                if (ImGui::RadioButton("Translate", m_gizmoOperation == ImGuizmo::TRANSLATE))
                    m_gizmoOperation = ImGuizmo::TRANSLATE;
                ImGui::SameLine();
                if (ImGui::RadioButton("Rotate", m_gizmoOperation == ImGuizmo::ROTATE))
                    m_gizmoOperation = ImGuizmo::ROTATE;
                ImGui::SameLine();
                if (ImGui::RadioButton("Scale", m_gizmoOperation == ImGuizmo::SCALE))
                    m_gizmoOperation = ImGuizmo::SCALE;

                if (m_gizmoOperation != ImGuizmo::SCALE)
                {
                    if (ImGui::RadioButton("Local", m_gizmoMode == ImGuizmo::LOCAL))
                        m_gizmoMode = ImGuizmo::LOCAL;
                    ImGui::SameLine();
                    if (ImGui::RadioButton("World", m_gizmoMode == ImGuizmo::WORLD))
                        m_gizmoMode = ImGuizmo::WORLD;
                }
            }

            if (auto meshComp = m_selectedGo->GetComponent<MeshComponent>())
            {
                ImGui::Text("Mesh");
                ImGui::Spacing();

                auto& material = meshComp->GetRenderItem()->GetMaterial();
                ImGui::Text("Material");
                ImGui::Checkbox("Cast Shadows", &material.CastShadows);
                ImGui::Checkbox("Transparent", &material.IsTransparent);
                if (material.IsTransparent)
                    ImGui::SliderFloat("Opacity", &material.Opacity, 0.0f, 1.0f);
            }

            if (auto lightComp = m_selectedGo->GetComponent<PointLightComponent>())
            {
                ImGui::Text("Point Light");
                ImGui::Spacing();

                float color[4] = { lightComp->m_pointLight.Color.x, lightComp->m_pointLight.Color.y, lightComp->m_pointLight.Color.z, lightComp->m_pointLight.Color.w };
                ImGui::ColorEdit4("Color", color);
                lightComp->m_pointLight.Color = { color[0], color[1], color[2], color[3] };

                float constant = lightComp->m_pointLight.ConstantAttenuation;
                ImGui::SliderFloat("Constant", &constant, 0.0f, 1.0f);
                lightComp->m_pointLight.ConstantAttenuation = constant;

                float linear = lightComp->m_pointLight.LinearAttenuation;
                ImGui::SliderFloat("Linear", &linear, 0.0f, 1.0f);
                lightComp->m_pointLight.LinearAttenuation = linear;

                float quadratic = lightComp->m_pointLight.QuadraticAttenuation;
                ImGui::SliderFloat("Quadratic", &quadratic, 0.0f, 1.0f);
                lightComp->m_pointLight.QuadraticAttenuation = quadratic;
            }

            ImGui::Spacing();
            if (ImGui::Button("Delete"))
            {
                RemoveModelFromScene(m_selectedGo);
                m_selectedGo.reset();
            }
        }
        ImGui::Separator();
        if (ImGui::Button("Add Cube"))
        {
            auto go = AddModelToScene("Cube", "Assets/cube.obj", "", "", "",
                { 0.0f, 0.f, 0.0f }, {}, { 1.0f, 1.0f, 1.0f });
            m_selectedGo = go;
        }
        if (ImGui::Button("Add Dragon"))
        {
            auto go = AddModelToScene("Dragon", "Assets/dragon.obj", "", "", "",
            { 0.0f, 0.0f, 0.0f }, {}, { 0.25f, 0.25f, 0.25f });
            m_selectedGo = go;
        }
        if (ImGui::Button("Add Light"))
        {
            auto go = AddLightToScene({ 0.0f, 1.0f, 0.0f }, {}, true);
            m_selectedGo = go;
        }
        
        ImGui::End();

        auto GBuffer = m_GBufferRenderPass->GetGBuffer();
        
        ImGui::Begin("Debug GBuffer");
        ImGui::Image((ImTextureID)GBuffer.AlbedoRenderTarget->m_srvUav.GPU.ptr, ImVec2(320, 180));
        ImGui::Image((ImTextureID)GBuffer.NormalRenderTarget->m_srvUav.GPU.ptr, ImVec2(320, 180));
        ImGui::Image((ImTextureID)GBuffer.MetallicRoughnessRenderTarget->m_srvUav.GPU.ptr, ImVec2(320, 180));
        ImGui::Image((ImTextureID)GBuffer.DepthBuffer->m_srvUav.GPU.ptr, ImVec2(320, 180));
        ImGui::End();

        if(m_enableShadows)
        {
            ImGui::Begin("Debug Shadow Map");
            ImGui::Image((ImTextureID)m_shadowRenderPass->GetShadowMap().DepthBuffer->m_srvUav.GPU.ptr, ImVec2(320, 320));
            ImGui::End();
        }

        if(m_enableSSAO)
        {
            ImGui::Begin("Debug SSAO");
            ImGui::Image((ImTextureID)m_SSAORenderPass->GetSSAOTexture()->m_srvUav.GPU.ptr, ImVec2(320, 180));
            ImGui::End();
        }

        if (m_renderWater)
        {
            ImGui::Begin("Water Settings");
            ImGui::SliderFloat("Waves Intensity", &m_waterSettings.WavesScalar, 0.0f, 2.0f);
            ImGui::Text("Normal Maps");
            ImGui::Spacing();
            ImGui::SliderFloat("Normal Scroll Speed", &m_waterSettings.NormalScrollSpeed, 0.0f, 0.2f);
            ImGui::SliderFloat("Normal Tiling", &m_waterSettings.NormalTilingFactor, 1.0f, 10.0f);
            ImGui::SliderFloat("Normal Tiling 2", &m_waterSettings.NormalTilingFactor2, 1.0f, 10.0f);
            float color[4] = { m_waterSettings.WaterColor.x, m_waterSettings.WaterColor.y, m_waterSettings.WaterColor.z, m_waterSettings.WaterColor.w };
            ImGui::ColorEdit4("Water Color", color, ImGuiColorEditFlags_AlphaBar);
            m_waterSettings.WaterColor = { color[0], color[1], color[2], color[3] };
            ImGui::End();
        }

        ImGui::Begin("Viewport");
        
        auto viewportSize = ImGui::GetContentRegionAvail();
        if(m_viewportCachedSize.x != viewportSize.x || m_viewportCachedSize.y != viewportSize.y)
        {
            m_viewportCachedSize = viewportSize;

            m_sceneRenderTexture.reset();
            m_sceneRenderTexture = m_renderer->CreateTexture(m_viewportCachedSize.x, m_viewportCachedSize.y, TextureFormat::RGBA8, TextureType::RenderTarget);
            m_renderer->CreateRenderTargetView(m_sceneRenderTexture);
            m_renderer->CreateShaderResourceView(m_sceneRenderTexture);

            m_GBufferRenderPass->OnResize(m_renderer, m_viewportCachedSize.x, m_viewportCachedSize.y);
            m_SSAORenderPass->OnResize(m_renderer, m_viewportCachedSize.x / 2, m_viewportCachedSize.y / 2);
            m_deferredLightingPass->OnResize(m_renderer, m_viewportCachedSize.x, m_viewportCachedSize.y);
            m_skyboxPass->OnResize(m_renderer, m_viewportCachedSize.x, m_viewportCachedSize.y);
            m_transparencyPass->OnResize(m_renderer, m_viewportCachedSize.x, m_viewportCachedSize.y);
            UpdateProjMatrix(m_viewportCachedSize.x, m_viewportCachedSize.y);
        }

        ImGui::Image((ImTextureID)m_sceneRenderTexture->m_srvUav.GPU.ptr, ImVec2(m_viewportCachedSize.x , m_viewportCachedSize.y));

        if(m_selectedGo != nullptr)
        {
            if(auto tfComp = m_selectedGo->GetComponent<TransformComponent>())
            {
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist(); 
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
                
                DirectX::XMFLOAT4X4 view;
                DirectX::XMStoreFloat4x4(&view, m_camera.GetViewMatrix());

                DirectX::XMFLOAT4X4 projection;
                DirectX::XMStoreFloat4x4(&projection, m_camera.GetProjMatrix());

                auto tfMat = DirectX::XMLoadFloat4x4(&tfComp->m_transform);
                DirectX::XMFLOAT4X4 tfCopy;
                DirectX::XMStoreFloat4x4(&tfCopy, tfMat);
                
                ImGuizmo::Manipulate(view.m[0], projection.m[0], m_gizmoOperation, m_gizmoMode, tfCopy.m[0]);

                if(ImGuizmo::IsUsing())
                    DirectX::XMStoreFloat4x4(&tfComp->m_transform, DirectX::XMLoadFloat4x4(&tfCopy));
            }
        }
        
        ImGui::End();
    } // End dockspace scope
    
    ImGui::End();
}

void CorvusEditor::OnKeyDown(int key)
{
    if(key == 'Z')
        m_cameraForward = 1.0f;
    else if(key == 'S')
        m_cameraForward = -1.0f;
    else if(key == 'Q')
        m_cameraRight = -1.0f;
    else if(key == 'D')
        m_cameraRight = 1.0f;
}

void CorvusEditor::OnKeyUp(int key)
{
    m_cameraForward = 0.0f;
    m_cameraRight = 0.0f;
    
    if(key == 'E')
    {
        m_mouseLocked = m_mouseLocked ? false : true;
        InputSystem::Get()->ShowCursor(!m_mouseLocked);
        uint32_t width, height;
        m_window->GetSize(width, height);
        InputSystem::Get()->SetCursorPosition(Vec2(width/2.0f, height/2.0f));
        m_lastMousePos[0] = width/2.0f;
        m_lastMousePos[1] = height/2.0f;
    }
    else if(key == 'R')
        m_selectedGo.reset();
}

void CorvusEditor::OnMouseMove(const InputListener::Vec2& mousePosition)
{
    if(!m_mouseLocked)
        return;

    float dx = DirectX::XMConvertToRadians(0.25f * (mousePosition.X - m_lastMousePos[0]));
    float dy = DirectX::XMConvertToRadians(0.25f * (mousePosition.Y - m_lastMousePos[1]));

    m_camera.Pitch(dy);
    m_camera.RotateY(dx);
    
    m_lastMousePos[0] = mousePosition.X;
    m_lastMousePos[1] = mousePosition.Y;
}

void CorvusEditor::OnLeftMouseDown(const InputListener::Vec2& mousePos)
{
}

void CorvusEditor::OnRightMouseDown(const InputListener::Vec2& mousePos)
{
}

void CorvusEditor::OnLeftMouseUp(const InputListener::Vec2& mousePos)
{
}

void CorvusEditor::OnRightMouseUp(const InputListener::Vec2& mousePos)
{
}

void CorvusEditor::UpdateProjMatrix(float width, float height)
{
    float aspectRatio = width / height;
    m_camera.UpdatePerspectiveFOV(m_fov * 3.14159f, aspectRatio);
}