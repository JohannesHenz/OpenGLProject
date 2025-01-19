#version 330 core

out vec4 FragColor;

in vec2 vTexCoord;
in vec2 vScreenPos;

uniform sampler2D uDiffuse; // color texture
uniform sampler2D uNormal;  // normal map (optional)

uniform bool uUseNormalMap; // if true, sample normal map
uniform vec2 uLightPos;     // The ball's position in screen space
uniform vec2 uResolution;   // screen resolution (width, height)

// Basic 2D normal mapping approach
void main()
{
    // 1. Fetch base color
    vec3 color = texture(uDiffuse, vTexCoord).rgb;

    // 2. If using normal map, fetch it. Otherwise default to a "flat" normal
    //    pointing out of the screen (0, 0, 1).
    vec3 normalSample = vec3(0.0, 0.0, 1.0);
    if(uUseNormalMap)
    {
        vec3 nm = texture(uNormal, vTexCoord).rgb;
        normalSample = normalize(nm * 2.0 - 1.0);
    }

    // 3. Compute fragment position in screen space
    //    We'll just use gl_FragCoord for an accurate fragment location.
    vec2 fragPos = gl_FragCoord.xy; // in [0..resolution], bottom-left origin

    // 4. Light direction in 2D
    vec2 lightDir2D = uLightPos - fragPos;
    float dist = length(lightDir2D);
    vec3 lightDir3D = normalize(vec3(lightDir2D, 0.0));

    // 5. Dot product with normal for diffuse
    vec3 N = normalSample;  // in 2D space, the “z” axis is out of screen
    float diff = max(dot(N, lightDir3D), 0.0);

    // 6. Simple radial attenuation
    float radius = 300.0; // tweak this to change the light falloff
    float att = clamp(1.0 - dist / radius, 0.0, 1.0);

    // 7. Combine with a bit of ambient
    float ambient = 0.2;
    float lighting = ambient + diff * att;

    vec3 finalColor = color * lighting;
    FragColor = vec4(finalColor, 1.0);
}
