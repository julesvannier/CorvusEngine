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

struct PixelIn
{
    float4 Position : SV_POSITION;
    float3 posWS : TEXCOORD0;
    float3 normalWS : NORMAL;
};

cbuffer WaterCBuf : register(b2)
{
    float4 WaterColor;
    float WavesScalar;
    float Paddin2[3];
}

float remap(float value, float inMin, float inMax, float outMin, float outMax)
{
    return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}

float4 Main(PixelIn Input) : SV_Target
{
    float3 n = normalize(Input.normalWS);
    float3 l = normalize(DirLightDirection) * -1.0f;
    float ndotl = dot(n, l);
    float3 color = WaterColor.xyz * ndotl;

    float3 view = normalize(CameraPosition - Input.posWS.xyz);
    float t = 1.0f - pow(saturate(dot(n, view)), 2.0f);
    t = remap(t, 0.0f, 1.0f, 0.25f, 0.9f);

    return float4(color.x, color.y, color.z, t);

    // return WaterColor;
    // return float4(0.0f, 0.0f, 255.0f, 0.6f);
}