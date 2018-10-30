#version 300 es
#extension GL_EXT_shader_framebuffer_fetch : require

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

vec4 encode (vec3 normal)
{
    float p = sqrt(normal.z*8.0+8.0);
    return vec4(normal.xy/p + 0.5,0,0);
}

layout(location = 0) out vec4 albedoBuffer;
layout(location = 1) out vec4 normalBuffer;

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
    albedoBuffer = vec4(albedo, 1.0);
    normalBuffer = encode(normal);
}
