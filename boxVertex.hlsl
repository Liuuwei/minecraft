cbuffer cb0 : register(b0) {
    float4x4 world;
}

cbuffer cb1 : register(b1) {
    float4x4 view;
}

cbuffer cb2 : register(b2) {
    float4x4 proj;
}

cbuffer cb3 : register(b3) {
    float3 offset;
}

float4 main(uint id : SV_VertexID) : SV_POSITION
{
    float3 positions[32] =
    {
        float3(-0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5),
        float3(0.5, 0.5, 0.5), float3(0.5, -0.5, 0.5),
        float3(0.5, -0.5, 0.5), float3(-0.5, -0.5, 0.5),
        float3(-0.5, -0.5, 0.5), float3(-0.5, 0.5, 0.5),

        float3(-0.5, 0.5, -0.5), float3(0.5, 0.5, -0.5),
        float3(0.5, 0.5, -0.5), float3(0.5, -0.5, -0.5),
        float3(0.5, -0.5, -0.5), float3(-0.5, -0.5, -0.5),
        float3(-0.5, -0.5, -0.5), float3(-0.5, 0.5, -0.5),

        float3(0.5, 0.5, 0.5), float3(0.5, 0.5, -0.5),
        float3(0.5, 0.5, -0.5), float3(0.5, -0.5, -0.5),
        float3(0.5, -0.5, -0.5), float3(0.5, -0.5, 0.5),
        float3(0.5, -0.5, 0.5), float3(0.5, 0.5, 0.5),

        float3(-0.5, 0.5, 0.5), float3(-0.5, 0.5, -0.5),
        float3(-0.5, 0.5, -0.5), float3(-0.5, -0.5, -0.5),
        float3(-0.5, -0.5, -0.5), float3(-0.5, -0.5, 0.5),
        float3(-0.5, -0.5, 0.5), float3(-0.5, 0.5, 0.5)
    };

    float4 pos = float4(positions[id] + offset, 1.0);
    pos.x += pos.x > 0 ? 0.001 : -0.001;
    pos.y += pos.y > 0 ? 0.001 : -0.001;
    pos.z += pos.z > 0 ? 0.001 : -0.001;
    pos = mul(pos, world);
    pos = mul(pos, view);
    pos = mul(pos, proj);

    return pos;
}