struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    uint id : ID;
};

cbuffer cb2 : register(b2) {
    int selected;
}

float4 main(PSInput input) : SV_TARGET
{
    if (input.id / 24 == selected - 1) {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}