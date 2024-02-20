struct PSInput
{
    float4 position : SV_POSITION;
    float3 uv : UV;
};

cbuffer cb0 : register(b0)
{
    float4x4 m;
}

cbuffer cb1 : register(b1)
{
    float4x4 v_;
}

cbuffer cb2 : register(b2)
{
    float4x4 p;
}


PSInput main(float3 position : POSITION, float3 uv : TEXCOORD)
{
    PSInput result;
    float4x4 v = v_;
    v[3][0] = 0.0;
    v[3][1] = 0.0;
    v[3][2] = 0.0;
    result.position = mul(float4(position, 1.0), m);
    result.position = mul(result.position, v);
    result.position = mul(result.position, p);
    result.position.z = result.position.w;
    result.uv = uv;

    return result;
}