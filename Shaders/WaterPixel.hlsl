struct PixelIn
{
    float4 Position : SV_POSITION;
};

float4 Main(PixelIn Input) : SV_Target
{
    return float4(0.0f, 0.0f, 255.0f, 0.6f);
}