#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;
in vec2 vScreenPos;

uniform sampler2D uDiffuse; 
uniform sampler2D uNormal;

uniform bool uUseNormalMap;
uniform vec2 uLightPos;  
uniform vec2 uResolution; 

void main()
{
    // 1) Base color
    vec3 color = texture(uDiffuse, vTexCoord).rgb;

    // 2) Normal map or fallback
    vec3 normalSample = vec3(0.0, 0.0, 1.0);
    if(uUseNormalMap)
    {
        vec3 nm = texture(uNormal, vTexCoord).rgb;
        normalSample = normalize(nm * 2.0 - 1.0);
    }

    // 3) Screen space position
    vec2 fragPos = gl_FragCoord.xy;

    // 4) Light direction + distance
    vec2 lightDir2D = uLightPos - fragPos;
    float dist = length(lightDir2D);
    vec3 lightDir3D = normalize(vec3(lightDir2D, 0.0));

    // 5) Diffuse
    float diff = max(dot(normalSample, lightDir3D), 0.0);

    // 6) Brighten these lines:
    float ambient   = 0.4;     // higher ambient
    float radius    = 500.0;   // bigger range
    float diffBoost = 1.5;     // stronger direct lighting

    // Attenuation
    float att = clamp(1.0 - dist / radius, 0.0, 1.0);

    // Combine
    float lighting = ambient + diff * att * diffBoost;
    
    vec3 finalColor = color * lighting;
    FragColor = vec4(finalColor, 1.0);
}
