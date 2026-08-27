RWTexture2D<float4> OutTexture : register(u0);
Texture2D<float4> SceneColor : register(t1);

cbuffer CBufTonemap : register(b2)
{
    float Exposure;
    int Operator;
    float Saturation;
    float Hue;
};

float3 ReinhardTonemap(float3 color)
{
    return color / (color + 1.0);
}

float3 ACESFilmTonemap(float3 color)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

float3x3 HueRotationMatrix(float angleRadians)
{
    float c = cos(angleRadians);
    float s = sin(angleRadians);
    float a = 1.0 / sqrt(3.0);
    float k = (1.0 - c) / 3.0;

    return float3x3(
        c + k,        k - a * s,    k + a * s,
        k + a * s,    c + k,        k - a * s,
        k - a * s,    k + a * s,    c + k
    );
}

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    float width, height;
    OutTexture.GetDimensions(width, height);
    if (ThreadID.x >= (uint)width || ThreadID.y >= (uint)height)
        return;

    float3 color = SceneColor.Load(int3(ThreadID.xy, 0)).rgb;
    color *= Exposure;
    color = Operator == 0 ? ACESFilmTonemap(color) : ReinhardTonemap(color);
    color = pow(color, 1.0 / 2.2);

    color = mul(HueRotationMatrix(Hue), color);

    float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
    color = lerp(luminance.xxx, color, Saturation);

    OutTexture[ThreadID.xy] = float4(color, 1.0);
}
