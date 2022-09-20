struct VS_INPUT
{
    float2 Position : POSITION;
    float4 Color    : COLOR;
    float2 TexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR;
    float2 TexCoord : TEXCOORD;
};

cbuffer MatrixConstantBuffer : register(b0)
{
    matrix CB_ViewProjection;
};

VS_OUTPUT VS_main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Position = mul(CB_ViewProjection, float4(input.Position, 0.0, 1.0));
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
    return output;
}
