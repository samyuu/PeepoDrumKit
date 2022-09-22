struct VS_INPUT
{
    uint VertexIndex : SV_VertexID;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

cbuffer MatrixConstantBuffer : register(b0)
{
    matrix CB_ViewProjection;
};

cbuffer WaveformConstantBuffer : register(b1)
{
    float4 CB_PerVertex[6];
    float4 CB_RectSize;
    float4 CB_Color;
    // float4 CB_Amplitudes[WAVEFORM_PIXELS_PER_CHUNK/4];
};

VS_OUTPUT VS_main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Position = mul(CB_ViewProjection, float4(CB_PerVertex[input.VertexIndex].xy, 0.0, 1.0));
    output.TexCoord = CB_PerVertex[input.VertexIndex].zw;
    return output;
}
