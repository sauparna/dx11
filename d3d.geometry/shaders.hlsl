cbuffer kd3d11_constants : register(b0)
{
    float4x4 model_view_proj_matrix;
};

struct VS_Input
{
    float3 pos : POS;
};

struct VS_Output
{
	float4 pos : SV_POSITION;
    float3 color : COLOR;
};

VS_Output vs_main(VS_Input input)
{
	VS_Output output;
	output.pos = mul(float4(input.pos, 1.0f), model_view_proj_matrix);
    output.color = input.pos + float3(0.5f, 0.5f, 0.5f); // Meaningless; just to derive colors.
	return output;
}

float4 ps_main(VS_Output input) : SV_TARGET
{
	return float4(abs(input.color), 1.0);
}
