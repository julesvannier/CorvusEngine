struct VertexOut
{
    float4 Position : SV_POSITION;
};

struct PatchTess
{
    float EdgeTessFactor[4] : SV_TessFactor;
    float InsideTessFactor[2] : SV_InsideTessFactor;
};
 
#define NUM_CONTROL_POINTS 4
#define TESS_FACTOR 64;
 
PatchTess ConstantHS(InputPatch<VertexOut, NUM_CONTROL_POINTS> patch, uint PatchID : SV_PrimitiveID)
{
    PatchTess output;
 
    output.EdgeTessFactor[0] = TESS_FACTOR;
    output.EdgeTessFactor[1] = TESS_FACTOR;
    output.EdgeTessFactor[2] = TESS_FACTOR;
    output.EdgeTessFactor[3] = TESS_FACTOR;
    
    output.InsideTessFactor[0] = TESS_FACTOR; 
    output.InsideTessFactor[1] = TESS_FACTOR; 
 
    return output;
}

struct HullOut
{
    float4 pos : POSITION0;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut Main(InputPatch<VertexOut, NUM_CONTROL_POINTS> ip, uint i : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID)
{
    HullOut output;
    output.pos = ip[i].Position;
    return output;
}