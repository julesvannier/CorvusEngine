RWTexture2D<float> OutSSAOTexture : register(u0);
Texture2D Depth : register(t1);
SamplerState Sampler : register(s2);

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    float depthWidth, depthHeight;
    Depth.GetDimensions(depthWidth, depthHeight);

    float2 st = ThreadID.xy / float2(depthWidth, depthHeight);
    float2 uv = 2.0 * float2(st.x, st.y) - 1.0;

    OutSSAOTexture[ThreadID.xy] = Depth.Sample(Sampler, uv).x;
}