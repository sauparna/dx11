cbuffer vs_constants : register(b0)
{
    float4x4 mvp_matrix;
    float4x4 mv_matrix;
    float3x3 normal_matrix;
};

struct DirectionalLight
{
    float4 eye_dir; // Direction towards the light.
    float4 color;
};

struct PointLight
{
    float4 eye_pos;
    float4 color;
};

// Constant buffer for the Blinn-Phong vertex shader.
cbuffer fs_constants : register(b0)
{
    DirectionalLight Ld;
    PointLight Lp;
};

struct VSIn
{
    float3 pos : POS;
    float2 uv : TEX;
    float3 norm : NORM;
};

struct VSOut
{
    float4 pos : SV_POSITION;
    float3 eye_pos : POSITION;
    float3 eye_norm : NORMAL;
    float2 uv : TEXCOORD;
};

Texture2D ktexture : register(t0);
SamplerState ksampler : register(s0);

VSOut vs_main(VSIn vsin)
{
    VSOut vsout;
    vsout.pos = mul(float4(vsin.pos, 1.0f), mvp_matrix);
    vsout.eye_pos = mul(float4(vsin.pos, 1.0f), mv_matrix).xyz;
    vsout.eye_norm = mul(vsin.norm, normal_matrix);
    vsout.uv = vsin.uv;
    return vsout;
}

float4 ps_main(VSOut psin) : SV_Target
{
    float3 diffuse_color = ktexture.Sample(ksampler, psin.uv).xyz;
    float3 frag_to_camera_dir = normalize(-psin.eye_pos);

    // Directional light.

    float3 I_dl;
    {
        float Ka = 0.1;
        float Ks = 0.9;
        float e = 100;
        float3 light_dir = Ld.eye_dir.xyz;
        float3 light_color = Ld.color.xyz;

        float3 Ia = Ka;

        float diffuse = max(0.0, dot(psin.eye_norm, light_dir));
        float3 Id = diffuse;

        float3 eye_half = normalize(frag_to_camera_dir + light_dir);
        float specular = max(0.0, dot(eye_half, psin.eye_norm));
        float Is = Ks * pow(specular, 2 * e);

        I_dl = (Ia + Id + Is) * light_color;
    }

    // Point light.
    float3 I_pl = float3(0, 0, 0);
    {
        float Ka = 0.1;
        float Ks = 0.9;
        float e = 100;

        float3 light_dir = Lp.eye_pos.xyz - psin.eye_pos;
        float inv_dist = 1 / length(light_dir);
        light_dir *= inv_dist; // normalize

        float3 light_color = Lp.color.xyz;

        float3 Ia = Ka;

        float3 diffuse = max(0.0, dot(psin.eye_norm, light_dir));
        float3 Id = diffuse;

        float3 eye_half = normalize(frag_to_camera_dir + light_dir);
        float specular = max(0.0, dot(eye_half, psin.eye_norm));
        float3 Is = Ks * pow(specular, 2 * e);

        I_pl = (Ia + Id + Is) * light_color * inv_dist;
    }

    float3 I = (I_dl + I_pl) * diffuse_color;

    return float4(I, 1.0);
}
