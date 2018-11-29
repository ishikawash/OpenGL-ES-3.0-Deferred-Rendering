#version 300 es
#extension GL_EXT_shader_pixel_local_storage : require

precision highp float;
uniform sampler2D s_GBuffer[3];

uniform mat4    u_InvProj;

uniform vec2    u_Viewport;

uniform vec3    u_LightColor;
uniform vec3    u_LightPosition;
uniform float   u_LightSize;

vec3 decode(vec2 encoded)
{
    vec2 fenc = encoded*4.0 - 2.0;
    float f = dot(fenc,fenc);
    float g = sqrt(1.0 - f/4.0);
    vec3 normal;
    normal.xy = fenc*g;
    normal.z = 1.0 - f/2.0;
    return normal;
}

layout(location = 0) out vec4 fragColor;

__pixel_local_inEXT FragDataLocal
{
    layout(rgb10_a2) highp vec4 albedo;
    layout(r11f_g11f_b10f) highp vec3 normal;
    layout(r32f) highp float depth;
} fragData;

/** GBuffer format
 *  [0] RGB: Albedo
 *  [1] RGB: VS Normal
 *  [2] R: Depth
 */
#if 0
void main(void)
{
    // fragColor = fragData.albedo;
    // fragColor = vec4(0.5*fragData.normal + 0.5, 1.0);
    fragColor = vec4(fragData.depth, fragData.depth, fragData.depth, 1.0);
}
#else
void main(void)
{
    /** Load texture values
     */
    vec2 tex_coord = gl_FragCoord.xy/u_Viewport; // map to [0..1]

    /* Calculate the pixel's position in view space */
    vec4 view_pos = vec4(tex_coord*2.0-1.0, fragData.depth*2.0 - 1.0, 1.0);
    view_pos = u_InvProj * view_pos;
    view_pos /= view_pos.w;

    vec3 light_dir = u_LightPosition - view_pos.xyz;
    float dist = length(light_dir);
    float size = u_LightSize;
    float attenuation = 1.0 - pow( clamp(dist/size, 0.0, 1.0), 2.0);
    light_dir = normalize(light_dir);

    /* Calculate diffuse lighting */
    float n_dot_l = clamp(dot(light_dir, fragData.normal), 0.0, 1.0);
    vec3 diffuse = u_LightColor * n_dot_l;

    vec3 final_lighting = attenuation * (diffuse);

    fragColor = vec4(final_lighting * fragData.albedo.rgb, 1.0);
}
#endif
