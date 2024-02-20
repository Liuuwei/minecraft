struct GSOutput
{
	float4 pos : SV_POSITION;
};

cbuffer cb0 : register(b0) {
    float4x4 scale;
}

[maxvertexcount(4)]
void main(
	point float4 input[1] : SV_POSITION, 
	inout LineStream< GSOutput > output
)
{
    GSOutput e1, e2, e3, e4;
    e1.pos = input[0];
    e2.pos = input[0];
    e3.pos = input[0];
    e4.pos = input[0];
    
    e1.pos.y += 1.0f;
    e2.pos.y -= 1.0f;
    e3.pos.x += 1.0f;
    e4.pos.x -= 1.0f;
    
    e1.pos = mul(e1.pos, scale);
    e2.pos = mul(e2.pos, scale);
    e3.pos = mul(e3.pos, scale);
    e4.pos = mul(e4.pos, scale);


    output.Append(e1);
    output.Append(e2);

    output.RestartStrip();

    output.Append(e3);
    output.Append(e4);
}