RWTexture2D<float> OutSSAOTexture : register(u0);
Texture2D Depth : register(t1);
SamplerState Sampler : register(s2);

cbuffer CBuf : register(b3)
{
    row_major float4x4 View;
    row_major float4x4 Proj;
    float Time;
    float3 CameraPosition;
    int Mode;
    float3 DirLightDirection;
    float DirLightIntensity;
    float3 Padding;
    row_major float4x4 InvViewProj;
    row_major float4x4 ShadowTransform;
    bool ShadowEnabled;
    bool AOEnabled;
    float2 Padding2;
};

Texture2D Normal : register(t4);

cbuffer CBufSSAO : register(b5)
{
    float Radius;
    float3 SSAOPadding;
};

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    float3 kernel[16] =
    {
        normalize(float3( 0.53,  0.12, 0.71)),
        normalize(float3(-0.34,  0.78, 0.22)),
        normalize(float3( 0.11, -0.62, 0.45)),
        normalize(float3(-0.81, -0.27, 0.13)),
        normalize(float3( 0.29,  0.94, 0.63)),
        normalize(float3(-0.63,  0.41, 0.08)),
        normalize(float3( 0.76, -0.18, 0.36)),
        normalize(float3(-0.09, -0.87, 0.91)),
        normalize(float3( 0.48,  0.55, 0.19)),
        normalize(float3(-0.97,  0.03, 0.58)),
        normalize(float3( 0.22,  0.31, 0.77)),
        normalize(float3(-0.15, -0.44, 0.05)),
        normalize(float3( 0.68, -0.72, 0.83)),
        normalize(float3(-0.51,  0.66, 0.39)),
        normalize(float3( 0.05,  0.99, 0.24)),
        normalize(float3(-0.88, -0.09, 0.66)),
    };

    float normalWidth, normalHeight;
    Normal.GetDimensions(normalWidth, normalHeight);
    if (ThreadID.x >= normalWidth || ThreadID.y >= normalHeight)
        return;
    
    float3 normal = normalize(Normal.Load(int3(ThreadID.x, ThreadID.y, 0)).xyz);

    float depthWidth, depthHeight;
    Depth.GetDimensions(depthWidth, depthHeight);
    if (ThreadID.x >= depthWidth || ThreadID.y >= depthHeight)
        return;
    
    float sceneDepth = Depth.Load(int3(ThreadID.x, ThreadID.y, 0)).r;
    if (sceneDepth >= 1.0f)
    {
        OutSSAOTexture[ThreadID.xy] = 1.0f;
        return;
    }
    
    float2 uv = float2((float)ThreadID.x / depthWidth, (float)ThreadID.y / depthHeight);
    float4 clipSpacePosition = float4(uv * 2.0 - 1.0, sceneDepth, 1.0);
    clipSpacePosition.y *= -1.0;
    float4 worldSpacePosition = mul(clipSpacePosition, InvViewProj); // NDC to World
    worldSpacePosition /= worldSpacePosition.w;
    
    float3 viewSpacePosition = mul(float4(worldSpacePosition.xyz, 1.0), View).xyz;
    
    float3 v = abs(normal.x) < 0.9 ? float3(1.0, 0.0, 0.0) : float3(0.0, 1.0, 0.0);
    float3 tangent = normalize(cross(v, normal));
    float3 bitangent = normalize(cross(tangent, normal));
    
    float3x3 tbn = float3x3(tangent, bitangent, normal);
    
    float radius = Radius;
    float occlusion = 0.0f;
    for (int i = 0; i < 16; i++)
    {   
        float3 pos = mul(kernel[i], tbn); // pos from tangent to world space
        pos = pos * radius + worldSpacePosition.xyz;
        
        float3 posView = mul(float4(pos, 1.0), View).xyz;

        float4 offset = mul(mul(float4(pos, 1.0), View), Proj);
        offset.xy /= offset.w;
        offset.y *= -1.0;
        offset.xy = offset.xy * 0.5 + 0.5;

        float sampledDepth = Depth.Load(int3(offset.x * depthWidth, offset.y * depthHeight, 0)).r;

        float4 sampledClipPosition = float4(offset.xy * 2.0 - 1.0, sampledDepth, 1.0);
        sampledClipPosition.y *= -1.0;
        float4 sampledWorldPosition = mul(sampledClipPosition, InvViewProj);
        sampledWorldPosition /= sampledWorldPosition.w;
        float sampledViewZ = mul(sampledWorldPosition, View).z;
        
        float rangeCheck = abs(viewSpacePosition.z - sampledViewZ) < radius ? 1.0 : 0.0;
        occlusion += (posView.z > sampledViewZ ? 1.0 : 0.0) * rangeCheck;
    }
    
    occlusion = 1.0 - (occlusion / 16.0);
    
    OutSSAOTexture[ThreadID.xy] = occlusion;
    if(Mode < 0)
        OutSSAOTexture[ThreadID.xy] = Depth.Sample(Sampler, float2(0.0, 0.0)).r;

}