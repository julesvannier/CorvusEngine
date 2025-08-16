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
 
PatchTess ConstantHS(InputPatch<VertexOut, NUM_CONTROL_POINTS> patch, uint PatchID : SV_PrimitiveID)
{
    PatchTess output;
 
    output.EdgeTessFactor[0] = 4;
    output.EdgeTessFactor[1] = 4;
    output.EdgeTessFactor[2] = 4;
    
    output.InsideTessFactor[0] = 4; 
    output.InsideTessFactor[1] = 4; 
 
    return output;
}

struct HullOut
{
    float4 pos : POSITION0;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut Main(InputPatch<VertexOut, NUM_CONTROL_POINTS> ip, uint i : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID)
{
    HullOut output;
    output.pos = ip[i].Position;
    return output;
}