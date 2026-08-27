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

StructuredBuffer<float4> Kernel : register(t6);
Texture2D NoiseTex : register(t7);
SamplerState NoiseSampler : register(s8);

static const int KernelSize = 16;
static const float NoiseDim = 4.0;

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
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
    
    float2 noiseUV = float2(ThreadID.xy) / NoiseDim;
    float3 randomVec = normalize(float3(NoiseTex.SampleLevel(NoiseSampler, noiseUV, 0).xy, 0.0));

    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);

    float3x3 tbn = float3x3(tangent, bitangent, normal);

    float radius = Radius;
    float occlusion = 0.0f;
    for (int i = 0; i < KernelSize; i++)
    {
        float3 pos = mul(Kernel[i].xyz, tbn); // pos from tangent to world space
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
    
    occlusion = 1.0 - (occlusion / (float)KernelSize);
    
    OutSSAOTexture[ThreadID.xy] = occlusion;
    if(Mode < 0)
        OutSSAOTexture[ThreadID.xy] = Depth.Sample(Sampler, float2(0.0, 0.0)).r;

}