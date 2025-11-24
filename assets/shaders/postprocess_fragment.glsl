#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D sceneColor;
uniform sampler2D sceneHighlights;
uniform float exposure;
uniform float bloomIntensity;

vec3 ToneMap(vec3 hdrColor, float exposureValue)
{
    return vec3(1.0) - exp(-hdrColor * exposureValue);
}

void main()
{
    vec3 baseColor = texture(sceneColor, TexCoord).rgb;
    vec3 highlightColor = texture(sceneHighlights, TexCoord).rgb;
    vec3 toneMapped = ToneMap(baseColor, exposure);
    vec3 bloom = ToneMap(highlightColor, exposure);
    vec3 result = clamp(toneMapped + bloom * bloomIntensity, 0.0, 1.0);
    FragColor = vec4(result, 1.0);
}

