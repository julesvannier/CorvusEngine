cbuffer CBuf : register(b0)
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
    float3 Padding2;
};

cbuffer WaterCBuf : register(b2)
{
    float4 WaterColor;
    float4 WaterColor2;
    float WavesScalar;
    float Roughness;
    float Reflectance;
    float NormalScrollSpeed;
    float NormalTilingFactor;
    float NormalTilingFactor2;
    float SSRIntensity;
    float EnviroIntensity;
    float DistortionFactor;
    float SpecularIntensity;
    float SpecularNoiseTilingFactor;
    float Padding3;
    float4 SSRSettings;
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
    float4 posWS : TEXCOORD0;
    float4 posView : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

#define NUM_WAVES 3

[domain("quad")]
DomainOut Main(PatchTess patchTess, float2 uv : SV_DomainLocation, const OutputPatch<HullOut, 4> quad)
{
    // We need to transform pos from world space to view/projection here, can't do it in the vertex shader
    // (see : https://thedemonthrone.ca/projects/rendering-terrain/rendering-terrain-part-8-adding-tessellation/)

    float3 v1 = lerp(quad[0].pos, quad[1].pos, uv.x).xyz;
    float3 v2 = lerp(quad[2].pos, quad[3].pos, uv.x).xyz;
    float3 p = lerp(v1, v2, uv.y);

    // implementation of : https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models sum of sines
    
    float t = 0.0f;
    float2 dx_sum = float2(0.0f, 0.0f); // sum of the derivatives in X and Z directions

    for (int i = 1; i < NUM_WAVES + 1; i++)
    {
        float l = 1.2f * i; // wavelength
        float a = WavesScalar * i; // amplitude
        float s = 0.8f * i; // speed
    
        float angle = (float)i * 0.5f;
        float2 direction = float2(cos(angle), sin(angle));
    
        float w = 2.0f / l; // frequency
        float phase = s * w;
    
        float wave_pos = dot(p.xz, direction);
    
        t += a * sin(wave_pos * w + Time * phase);
    
        float derivative = a * w * cos(wave_pos * w + Time * phase);
        dx_sum += derivative * direction;
    }

    p.y = p.y + t;

    float3 tangent_x = normalize(float3(1.0f, dx_sum.x, 0.0f));
    float3 tangent_z = normalize(float3(0.0f, dx_sum.y, 1.0f));

    float3 normal = normalize(cross(tangent_z, tangent_x));

    DomainOut dout;
    dout.pos = float4(p, 1.0f);
    dout.posWS = dout.pos;
    dout.posView = mul(dout.pos, View); // To view space and storing it for SSR later
    dout.pos = mul(dout.posView, Proj);
    dout.normal = normal;
    dout.tangent = tangent_x;
    dout.binormal = tangent_z;
    dout.uv = uv;
    
    return dout;
}