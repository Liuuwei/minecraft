struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    uint id : ID;
};

SamplerState g_sampler : register(s0);

Texture2D grassTexture : register(t0);
Texture2D iceTexture : register(t1);
Texture2D planksTexture : register(t2);
Texture2D stoneTexture : register(t3);
Texture2D woolBlueTexture : register(t4);
Texture2D sandTexture : register(t5);


float4 main(PSInput input) : SV_TARGET
{
    int index = input.id / 6;
    if (index == 0)
    {
        return grassTexture.Sample(g_sampler, input.uv);
    }
    if (index == 1)
    {
        return iceTexture.Sample(g_sampler, input.uv);
    }
    if (index == 2)
    {
        return planksTexture.Sample(g_sampler, input.uv);
    }
    if (index == 3)
    {
        return stoneTexture.Sample(g_sampler, input.uv);
    }
    if (index == 4)
    {
        return woolBlueTexture.Sample(g_sampler, input.uv);
    }
    if (index == 5)
    {
        return sandTexture.Sample(g_sampler, input.uv);
    }

    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}