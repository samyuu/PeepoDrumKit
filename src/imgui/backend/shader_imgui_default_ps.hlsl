struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR;
    float2 TexCoord : TEXCOORD;
};

SamplerState Sampler : register(s0);
Texture2D Texture : register(t0);

float4 PS_main(VS_OUTPUT input) : SV_Target
{
    return input.Color * Texture.Sample(Sampler, input.TexCoord);
}
