#version 330 core

// Fragment shader de pós-processamento com tone mapping e bloom
// Aplica tone mapping exponencial ao HDR e combina highlights para efeito bloom
// Usa dois buffers de entrada: cor da cena e highlights extraídos

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D sceneColor;      // Buffer de cor da cena renderizada (HDR)
uniform sampler2D sceneHighlights; // Buffer de highlights brilhantes (para bloom)
uniform float exposure;            // Exposição para tone mapping
uniform float bloomIntensity;      // Intensidade do efeito bloom

// Tone mapping exponencial: converte HDR para LDR
vec3 ToneMap(vec3 hdrColor, float exposureValue)
{
    return vec3(1.0) - exp(-hdrColor * exposureValue);
}

void main()
{
    // Carrega cor base e highlights do framebuffer
    vec3 baseColor = texture(sceneColor, TexCoord).rgb;
    vec3 highlightColor = texture(sceneHighlights, TexCoord).rgb;
    
    // Aplica tone mapping separadamente
    vec3 toneMapped = ToneMap(baseColor, exposure);
    vec3 bloom = ToneMap(highlightColor, exposure);
    
    // Combina cor base com bloom e garante valores válidos
    vec3 result = clamp(toneMapped + bloom * bloomIntensity, 0.0, 1.0);
    FragColor = vec4(result, 1.0);
}

