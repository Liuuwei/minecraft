cbuffer cb0 : register(b0) {
    float4x4 model;
}

cbuffer cb1 : register(b1) {
    float aspect;
}

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    uint id : ID;
};

PSInput main(float2 pos : POSITION, float2 uv : TEXCOORD, uint id : SV_VertexID)
{
    PSInput result;
    pos.y *= aspect;
    result.position = mul(float4(pos, 0.0, 1.0), model);
    result.position.z = 0.0;
    result.uv = uv;
    result.id = id;

    return result;
}