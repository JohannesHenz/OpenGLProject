#version 330 core

in vec2 TexCoord;

uniform sampler2D texture1;  // Basistexur
uniform sampler2D normalMap; // Normalmap

uniform vec3 lightPos; // Position der Lichtquelle
uniform vec3 lightColor; // Farbe des Lichts
uniform vec3 viewPos;  // Position des Betrachters (kann in 2D auf 0, 0, 1 gesetzt werden)

out vec4 FragColor;

void main() {
    // Basisfarben und Normalen
    vec3 color = texture(texture1, TexCoord).rgb;
    vec3 normal = texture(normalMap, TexCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0); // Normalmap von [0,1] auf [-1,1] skalieren

    // Beleuchtungsvektoren
    vec3 lightDir = normalize(lightPos - vec3(TexCoord, 0.0));
    float diff = max(dot(normal, lightDir), 0.0);

    // Diffuse Beleuchtung
    vec3 diffuse = diff * lightColor;

    // Endfarbe
    vec3 result = (diffuse + vec3(0.1)) * color; // Ambient + Diffuse
    FragColor = vec4(result, 1.0);
}
