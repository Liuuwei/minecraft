struct PSInput
{
    float4 position : SV_POSITION;
    float3 uv : UV;
    int textureIndex : TEXTURE_INDEX;
};

cbuffer cb0 : register(b0)
{
    float4x4 m;
}

cbuffer cb1 : register(b1)
{
    float4x4 v;
}

cbuffer cb2 : register(b2)
{
    float4x4 p;
}

cbuffer cb3 : register(b3) {
    float4x4 mvp;
}

PSInput main(float3 position : POSITION, float3 uv : TEXCOORD, int textureIndex : TEXTURE_INDEX)
{
    PSInput result;
    result.position = mul(float4(position, 1.0), mvp);
	result.uv = uv;
    result.textureIndex = textureIndex;

    return result;
}