struct PixelIn
{
    float4 Position : SV_POSITION;
};

cbuffer WaterCBuf : register(b2)
{
    float4 WaterColor;
    float WavesScalar;
    float Paddin2[3];
}

float4 Main(PixelIn Input) : SV_Target
{
    return WaterColor;
    
    // return float4(0.0f, 0.0f, 255.0f, 0.6f);
}