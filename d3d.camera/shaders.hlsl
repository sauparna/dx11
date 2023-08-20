cbuffer kd3d11_constants : register(b0)
{
    float4x4 model_view_proj;
};

struct VS_Input
{
    float2 pos : POS;
    float2 uv : TEX;
};

struct VS_Output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

Texture2D ktexture : register(t0);
SamplerState ksampler : register(s0);

VS_Output vs_main(VS_Input input)
{
	VS_Output output;
	output.pos = mul(float4(input.pos, 0.0f, 1.0f), model_view_proj);
	output.uv = input.uv;
	return output;
}

float4 ps_main(VS_Output input) : SV_TARGET
{
	return ktexture.Sample(ksampler, input.uv);
}

