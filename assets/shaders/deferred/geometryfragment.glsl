#version 300 es
#extension GL_EXT_shader_pixel_local_storage : require

precision highp float;
uniform sampler2D s_Albedo;
uniform sampler2D s_Normal;

uniform vec3    u_SpecularColor;
uniform float   u_SpecularPower;
uniform float   u_SpecularCoefficient;

in vec3 v_NormalVS;
in vec3 v_TangentVS;
in vec3 v_BitangentVS;
in vec2 v_TexCoord;

vec2 encode (vec3 normal)
{
    float p = sqrt(normal.z*8.0+8.0);
    return normal.xy/p;
}

__pixel_local_outEXT FragDataLocal
{
    layout(rgb10_a2) highp vec4 albedo;
    layout(r11f_g11f_b10f) highp vec3 normal;
    layout(r32f) highp float depth;
} fragData;

void main(void) {
    /** Load texture values
     */
    vec3 albedo = texture(s_Albedo, v_TexCoord).rgb;
    vec3 normal = normalize(texture(s_Normal, v_TexCoord).rgb*2.0 - 1.0);
    vec3 specular_color = u_SpecularCoefficient * u_SpecularColor;
    
    vec3 N = normalize(v_NormalVS);
    vec3 T = normalize(v_TangentVS);
    vec3 B = normalize(v_BitangentVS);

    mat3 TBN = mat3(T, B, N);
    normal = normalize(TBN*normal);

    /** GBuffer format
     *  [0] RGB: Albedo
     *  [1] RGB: VS Normal
     *  [2] R: Depth
     */
    fragData.albedo = vec4(albedo, 1.0);
    fragData.normal = normal;
    fragData.depth = gl_FragCoord.z;
}
