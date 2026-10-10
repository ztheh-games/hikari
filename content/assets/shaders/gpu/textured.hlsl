Texture2D<float4> sourceTexture : register(t0, space2);
SamplerState sourceSampler : register(s0, space2);
float4 main(float2 uv : TEXCOORD0, float4 color : TEXCOORD1) : SV_Target0 {
    return sourceTexture.Sample(sourceSampler, uv) * color;
}
