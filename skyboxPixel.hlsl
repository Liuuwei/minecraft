struct PSInput
{
    float4 position : SV_POSITION;
    float3 uv : UV;
};

TextureCube g_texture : register(t0);
SamplerState g_sampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}