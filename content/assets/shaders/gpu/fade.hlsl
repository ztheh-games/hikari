Texture2D<float4> sourceTexture : register(t0, space2);
SamplerState sourceSampler : register(s0, space2);
cbuffer Fade : register(b0, space3) { float fadePercent; float3 padding; };
float4 main(float2 uv : TEXCOORD0, float4 color : TEXCOORD1) : SV_Target0 {
    float factor = fadePercent <= 0.0 ? 1.0 : fadePercent < 25.0 ? 95.0 / 255.0
                 : fadePercent < 50.0 ? 47.0 / 255.0 : fadePercent < 75.0 ? 31.0 / 255.0 : 15.0 / 255.0;
    return sourceTexture.Sample(sourceSampler, uv) * float4(factor, factor, factor, 1.0);
}
