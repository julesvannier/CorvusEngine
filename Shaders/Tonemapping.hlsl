RWTexture2D<float4> OutTexture : register(u0);
Texture2D<float4> SceneColor : register(t1);

cbuffer CBufTonemap : register(b2)
{
    float Exposure;
    float3 Padding;
};

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    float width, height;
    OutTexture.GetDimensions(width, height);
    if (ThreadID.x >= (uint)width || ThreadID.y >= (uint)height)
        return;

    float3 color = SceneColor.Load(int3(ThreadID.xy, 0)).rgb;
    color *= Exposure;
    color = color / (color + 1.0);
    color = pow(color, 1.0 / 2.2);

    OutTexture[ThreadID.xy] = float4(color, 1.0);
}
