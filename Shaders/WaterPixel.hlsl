struct PixelIn
{
    float4 Position : SV_POSITION;
};

float4 Main(PixelIn Input) : SV_Target
{
    return float4(1.0, 1.0, 1.0, 1.0);
}