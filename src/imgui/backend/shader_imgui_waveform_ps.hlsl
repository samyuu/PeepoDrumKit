struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

#define WAVEFORM_PIXELS_PER_CHUNK 256
cbuffer WaveformConstantBuffer : register(b1)
{
    float4 CB_PerVertex[6];
    float4 CB_RectSize;
    float4 CB_Color;
    float4 CB_Amplitudes[WAVEFORM_PIXELS_PER_CHUNK/4];
};

float4 PS_main(VS_OUTPUT input) : SV_Target
{
    // NOTE: Unpack tighly packed floats and sample as a point-clamp 1D "texture"
    uint sliceIndex = uint(input.TexCoord.x * WAVEFORM_PIXELS_PER_CHUNK);
    float amplitude = CB_Amplitudes[sliceIndex >> 2][sliceIndex & 3];
    
    // NOTE: In normalized (1.0f <-> 0.0f) range
    float centerDistance = abs((input.TexCoord.y * 2.0f) - 1.0f);
    
    // NOTE: Mask off amplitude and out of bounds region
    float alpha = float(centerDistance <= amplitude) * float(amplitude > 0.0f);
    
    // NOTE: Fade out towards the edge
    alpha *= (1.0f - centerDistance);

    return float4(CB_Color.rgb, CB_Color.a * alpha);
}
