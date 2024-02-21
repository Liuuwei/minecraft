struct PSInput
{
    float4 position : SV_POSITION;
    float3 uv : UV;
    int textureIndex : TEXTURE_INDEX;
};

TextureCube skyboxTexture : register(t0);
TextureCube grassTexture : register(t1);
TextureCube iceTexture : register(t2);
TextureCube planksTexture : register(t3);
TextureCube stoneTexture : register(t4);
TextureCube woolBlueTexture : register(t5);
TextureCube sandTexture : register(t6);
 
SamplerState g_sampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    if (input.textureIndex == 1) {
		return grassTexture.Sample(g_sampler, input.uv);
    }
    if (input.textureIndex == 2) {
        return iceTexture.Sample(g_sampler, input.uv);
    }
    if (input.textureIndex == 3) {
        return planksTexture.Sample(g_sampler, input.uv);
    }
    if (input.textureIndex == 4) {
        return stoneTexture.Sample(g_sampler, input.uv);
    }
    if (input.textureIndex == 5) {
        return woolBlueTexture.Sample(g_sampler, input.uv);
    }
    if (input.textureIndex == 6) {
        return sandTexture.Sample(g_sampler, input.uv);
    }
    return grassTexture.Sample(g_sampler, input.uv);
}