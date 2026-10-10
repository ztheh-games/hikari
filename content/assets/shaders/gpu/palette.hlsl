Texture2D<float4> sourceTexture : register(t0, space2);
Texture2D<float4> paletteTexture : register(t1, space2);
SamplerState sourceSampler : register(s0, space2);
SamplerState paletteSampler : register(s1, space2);
cbuffer Palette : register(b0, space3) {
    float paletteIndex;
    float tableWidth;
    float tableHeight;
    float padding;
};
float4 main(float2 uv : TEXCOORD0, float4 color : TEXCOORD1) : SV_Target0 {
    float4 source = sourceTexture.Sample(sourceSampler, uv);
    if (source.a == 0.0) discard;
    return paletteTexture.Sample(paletteSampler, float2(source.r * 256.0 / tableWidth, paletteIndex / tableHeight));
}
