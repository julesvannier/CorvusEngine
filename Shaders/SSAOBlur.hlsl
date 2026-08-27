RWTexture2D<float> OutBlurTexture : register(u0);
Texture2D SSAOInput : register(t1);

static const int BlurSize = 4;

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    float width, height;
    SSAOInput.GetDimensions(width, height);
    if (ThreadID.x >= (uint)width || ThreadID.y >= (uint)height)
        return;

    int halfSize = BlurSize / 2;
    float sum = 0.0f;

    for (int y = -halfSize; y < halfSize; y++)
    {
        for (int x = -halfSize; x < halfSize; x++)
        {
            int2 samplePixel = clamp(int2(ThreadID.xy) + int2(x, y), int2(0, 0), int2(width - 1, height - 1));
            sum += SSAOInput.Load(int3(samplePixel, 0)).r;
        }
    }

    OutBlurTexture[ThreadID.xy] = sum / (float)(BlurSize * BlurSize);
}
