RWTexture2D<float> OutSSAOTexture : register(u0);
Texture2D Depth : register(t1);
SamplerState Sampler : register(s2);

cbuffer SSAOCbuf : register(b3)
{
    int Mode;
    float3 Padding;
};

Texture2D Normal : register(t4);

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    float depthWidth, depthHeight;
    Depth.GetDimensions(depthWidth, depthHeight);

    float2 st = ThreadID.xy / float2(depthWidth, depthHeight);
    float2 depthUV = 2.0 * float2(st.x, st.y) - 1.0;

    float normalWidth, normalHeight;
    Normal.GetDimensions(normalWidth, normalHeight);

    float2 st2 = ThreadID.xy / float2(normalWidth, normalHeight);
    float2 normalUV = 2.0 * float2(st2.x, st2.y) - 1.0;

    switch (Mode)
    {
    case 0:
        OutSSAOTexture[ThreadID.xy] = Depth.Sample(Sampler, depthUV).x;
        break;

    case 1:
        OutSSAOTexture[ThreadID.xy] = Normal.Sample(Sampler, normalUV).x;
        break;
        
    default:
        OutSSAOTexture[ThreadID.xy] = 1.0f;
        break;
    }
}