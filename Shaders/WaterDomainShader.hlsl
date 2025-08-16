cbuffer CBuf : register(b0)
{
    row_major float4x4 ViewProj;
    float Time;
    float3 CameraPosition;
    int Mode;
    float3 DirLightDirection;
    float DirLightIntensity;
    float3 Padding;
    row_major float4x4 InvViewProj;
    row_major float4x4 ShadowTransform;
    bool ShadowEnabled;
    float3 Padding2;
};

cbuffer WaterCBuf : register(b2)
{
    float4 WaterColor;
    float WavesScalar;
    float Paddin2[3];
}

struct HullOut
{
    float4 pos : POSITION0;
};

struct PatchTess
{
    float EdgeTessFactor[4] : SV_TessFactor;
    float InsideTessFactor[2] : SV_InsideTessFactor;
};

struct DomainOut
{
    float4 pos : SV_POSITION;
};

[domain("quad")]
DomainOut Main(PatchTess patchTess, float2 uv : SV_DomainLocation, const OutputPatch<HullOut, 4> quad)
{
    // We need to transform pos from world space to view/projection here, can't do it in the vertex shader
    // (see : https://thedemonthrone.ca/projects/rendering-terrain/rendering-terrain-part-8-adding-tessellation/)

    float3 v1 = lerp(quad[0].pos, quad[1].pos, uv.x).xyz;
    float3 v2 = lerp(quad[2].pos, quad[3].pos, uv.x).xyz;
    float3 p = lerp(v1, v2, uv.y);

    float wave1 = 0.3f * sin(p.x - Time * 2.0f);  
    float wave2 = 0.1f * sin(p.x + Time * 4.0f);
    float wave3 = 0.2f * sin(p.x * 2.0f - Time * 3.0f);

    float t = (wave1 + wave2 + wave3) * WavesScalar;
    p.y = p.y + t;
    
    DomainOut dout;
    dout.pos = float4(p, 1.0f);
    dout.pos = mul(dout.pos, ViewProj);
    
    return dout;
}