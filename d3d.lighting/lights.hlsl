cbuffer constants : register(b0)
{
    float4x4 mvp_matrix;
    float4 color;
};

struct VSIn
{
    float3 pos : POS;
};

struct VSOut
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

VSOut vs_main(VSIn vsin)
{
    VSOut vsout;
    vsout.pos = mul(float4(vsin.pos, 1.0f), mvp_matrix);
    vsout.color = color;
    return vsout;
}

float4 ps_main(VSOut psin) : SV_Target
{
    return psin.color;
}
