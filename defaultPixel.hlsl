struct PSInput
{
    float4 position : SV_POSITION;
    float3 uv : UV;
};

TextureCube g_texture : register(t0);
TextureCube caoTexture : register(t1);
SamplerState g_sampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    float3 uv = input.uv;
    return caoTexture.Sample(g_sampler, uv);
    // return pow(caoTexture.Sample(g_sampler, uv), 1.0 / 2.2);
}