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
    float3 posWS : TEXCOORD0;
    float3 normalWS : NORMAL;
};

[domain("quad")]
DomainOut Main(PatchTess patchTess, float2 uv : SV_DomainLocation, const OutputPatch<HullOut, 4> quad)
{
    // We need to transform pos from world space to view/projection here, can't do it in the vertex shader
    // (see : https://thedemonthrone.ca/projects/rendering-terrain/rendering-terrain-part-8-adding-tessellation/)

    float3 v1 = lerp(quad[0].pos, quad[1].pos, uv.x).xyz;
    float3 v2 = lerp(quad[2].pos, quad[3].pos, uv.x).xyz;
    float3 p = lerp(v1, v2, uv.y);

    // p.y = sin(p.x);

    // implementation of : https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models sum of sines
    float l = 1.2f; // wavelength
    float a = WavesScalar; // amplitude
    float s = 2.0f; // speed

    float w = 2/l;
    float phase = s * w;

    p.y = a * sin(p.x * w + Time * phase);

    float dx = a * w * cos(p.x * w + Time * phase);

    float3 tangent = float3(1.0f, dx, 0.0f);
    float3 binormal = float3(0.0f, 0.0f, 1.0f);

    float3 normal = normalize(-cross(tangent, binormal)); 

    DomainOut dout;
    dout.pos = float4(p, 1.0f);
    dout.pos = mul(dout.pos, ViewProj);
    dout.posWS = p;
    dout.normalWS = normal;
    
    return dout;
}