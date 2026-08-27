#pragma once
#include "Core.h"

#define MAX_LIGHTS 150

#define MAX_INSTANCES 300

#define MAX_TRANSPARENT_OBJECTS 50

struct PointLight
{
    DirectX::XMFLOAT3 Position;
    float Radius = 20.0f;
    // 16 bytes boundary 
    DirectX::XMFLOAT4 Color = { 1.0, 1.0, 1.0, 1.0 };
    // 16 bytes boundary 
    float ConstantAttenuation = 1.0f;
    float LinearAttenuation = 0.2f;
    float QuadraticAttenuation = 0.1f;
    float Padding3 = 0.0f;
};

struct PointLightsConstantBuffer
{
    PointLight PointLights[MAX_LIGHTS];
};

struct WaterConstantBuffer
{
    DirectX::XMFLOAT4 WaterColor = { 13.0f/255.0f, 154.0f/255.0f, 182.0f/255.0f, 0.65f };
    DirectX::XMFLOAT4 WaterColor2 = { 0.0f/255.0f, 255.0f/255.0f, 199.0f/255.0f, 0.65f };
    float WavesScalar = 0.042f;
    float Roughness = 0.08f;
    float Reflectance = 0.55f;
    float NormalScrollSpeed = 0.02f;
    // 16 bytes boundary 
    float NormalTilingFactor = 2.2f;
    float NormalTilingFactor2 = 1.8f;
    float SSRIntensity = 1.05f;
    float EnviroIntensity = 0.18f;
    // 16 bytes boundary
    float DistortionFactor = 0.06f;
    float SpecularIntensity = 125.0f;
    float SpecularNoiseTilingFactor = 0.32f;
    float Padding;
    // 16 bytes boundary
    DirectX::XMFLOAT4 SSRSettings = { 40.0f /* forward steps max */, 20.0f /* backward steps max */, 0.2f /* forward step length */, 60.0f /* depth fade */ };
};

struct SceneConstantBuffer
{
    DirectX::XMFLOAT4X4 View;
    // 16 bytes boundary
    DirectX::XMFLOAT4X4 Proj;
    // 16 bytes boundary
    float Time;
    DirectX::XMFLOAT3 CameraPosition;
    // 16 bytes boundary
    int Mode;
    DirectX::XMFLOAT3 DirLightDirection;
    // 16 bytes boundary
    float DirLightIntensity;
    float ScreenDimensions[2];
    float Padding;
    // 16 bytes boundary
    DirectX::XMFLOAT4X4 InvViewProj;
    // 16 bytes boundary
    DirectX::XMFLOAT4X4 ShadowTransform;
    // 16 bytes boundary
    int ShadowEnabled;
    int AOEnabled;
    float Paddin2[2];
};

struct ScreenQuadVertex
{
    DirectX::XMFLOAT4 Position;
    DirectX::XMFLOAT2 TexCoord;
};

struct InstanceData
{
    DirectX::XMFLOAT4X4 WorldMat;
    int HasAlbedo;
    int HasNormalMap;
    int HasMetallicRoughness;
};

struct SkyBoxConstantBuffer
{
    DirectX::XMFLOAT4X4 ViewProj;
    DirectX::XMFLOAT3 CameraPosition;
};

struct PrefilterMapFilterSettings
{
    float Roughness;
    float Padding[3];
};

struct ShadowMapConstantBuffer
{
    DirectX::XMFLOAT4X4 ViewProj;
};

struct SSAOConstantBuffer
{
    float Radius = 0.35f;
    float Padding[3];
};

struct TonemapConstantBuffer
{
    float Exposure = 1.0f;
    float Padding[3];
};