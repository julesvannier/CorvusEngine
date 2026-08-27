#pragma once
#include "Camera.h"
#include "Core.h"
#include "InputListener.h"
#include "ResourcesManager.h"
#include "Rendering/RenderItem.h"
#include "Window.h"
#include "ECS/Scene.h"
#include "ImGui/ImGuizmo.h"
#include "Rendering/GBufferRenderPass.h"
#include "Rendering/LightingRenderPass.h"
#include "RHI/D3D12Driver.h"
#include "Rendering/RenderPass.h"
#include "Rendering/ShadowRenderPass.h"
#include "Rendering/SkyBoxRenderPass.h"
#include "Rendering/SSAORenderPass.h"
#include "Rendering/TransparencyRenderPass.h"
#include "Rendering/WaterRenderPass.h"

class CorvusEditor : public InputListener
{
public:
    CorvusEditor();
    ~CorvusEditor();

    std::shared_ptr<GameObject> AddModelToScene(std::string name, const std::string& modelPath, const std::string& albedoPath, const std::string& normalPath, const std::string& mrPath,
        DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f }, DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f }, bool transparent = false);
    std::shared_ptr<GameObject> AddWaterToScene( DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f }, DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f });
    void RemoveModelFromScene(std::shared_ptr<GameObject> goToRemove);
    std::shared_ptr<GameObject> AddLightToScene(DirectX::XMFLOAT3 position, DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f }, bool randomColor = false);
    void RenderUI(float width, float height);
    void Run();

    // InputListener interface
    void OnKeyDown(int key) override;
    void OnKeyUp(int key) override;
    void OnMouseMove(const InputListener::Vec2& mousePosition) override;
    virtual void OnLeftMouseDown(const InputListener::Vec2& mousePos) override;
    virtual void OnRightMouseDown(const InputListener::Vec2& mousePos) override;
    virtual void OnLeftMouseUp(const InputListener::Vec2& mousePos) override;
    virtual void OnRightMouseUp(const InputListener::Vec2& mousePos) override;
    // end InputListener interface

private:
    void UpdateProjMatrix(float width, float height);
    
    std::shared_ptr<Window> m_window;
    std::shared_ptr<D3D12Driver> m_device;
    std::shared_ptr<GraphicsPipeline> m_trianglePipeline;
    std::shared_ptr<Buffer> m_constantBuffer;
    std::shared_ptr<Buffer> m_lightsConstantBuffer;
    std::shared_ptr<Sampler> m_textureSampler;

    std::shared_ptr<Texture> m_sceneRenderTexture;
    
    std::shared_ptr<ShadowRenderPass> m_shadowRenderPass;
    std::shared_ptr<GBufferRenderPass> m_GBufferRenderPass;
    std::shared_ptr<SSAORenderPass> m_SSAORenderPass;
    std::shared_ptr<LightingRenderPass> m_deferredLightingPass;
    std::shared_ptr<SkyBoxRenderPass> m_skyboxPass;
    std::shared_ptr<TransparencyRenderPass> m_transparencyPass;
    std::shared_ptr<WaterRenderPass> m_waterPass;

    std::shared_ptr<ResourcesManager> m_resourceManager;

    std::shared_ptr<Scene> m_scene;
    std::shared_ptr<GameObject> m_selectedGo;

    std::unordered_map<std::string, std::shared_ptr<Buffer>> m_meshesInstancesBuffers;
    std::unordered_map<std::string, int> m_meshesInstancesCount;

    std::vector<std::tuple<LogType, std::string>> m_loggedMessages;

    float m_startTime;
    float m_lastTime;
    float m_elapsedTime;

    Camera m_camera;
    float m_cameraForward = 0.0f;
    float m_cameraRight = 0.0f;
    float m_lastMousePos[2];

    float m_fov = 60.0f;
    float m_previousFov = m_fov;
    float m_moveSpeed = 18.0f;
    bool m_mouseLocked = true;

    float m_dirLightDirection[3] = { -0.85f, -1.0f, -0.7f };
    float m_dirLightIntensity = 1.0;

    float m_defaultLightConstAttenuation = 0.65f;
    float m_defaultLightLinearAttenuation = 0.1f;
    float m_defaultLightQuadraticAttenuation = 0.02f;

    WaterConstantBuffer m_waterSettings;
    SSAOConstantBuffer m_ssaoSettings;

    int m_shadowMapResolution = 2048;
    bool m_enableShadows = true;
    bool m_enableSSAO = true;
    bool m_enableSkyBox = true;
    bool m_enablePointLights = false;
    bool m_renderTransparentObjects = true;
    bool m_renderWater = true;
    bool m_vsync = false;

    ImGuizmo::OPERATION m_gizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE m_gizmoMode = ImGuizmo::WORLD;
    int m_viewMode;

    ImVec2 m_viewportCachedSize;
};