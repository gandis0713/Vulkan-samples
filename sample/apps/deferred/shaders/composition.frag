#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSamplerPosition;
layout(binding = 1) uniform sampler2D texSamplerNormal;
layout(binding = 2) uniform sampler2D texSamplerAlbedo;

struct Light
{
    vec3 position;
    vec3 color;
};

#define lightCount 7
#define ambient 0.0

// std140 for only uniform, consider alignment such as vec3
layout(std140, binding = 3) uniform UBO
{
    Light lights[lightCount];
    vec3 cameraPosition;
}
ubo;

void main()
{
    if (inTexCoord.x < 0.5 && inTexCoord.y < 0.5)
    {
        // Get G-Buffer values
        vec3 position = texture(texSamplerPosition, inTexCoord * 2.0f).rgb;
        vec3 normal = texture(texSamplerNormal, inTexCoord * 2.0f).rgb;
        vec4 albedo = texture(texSamplerAlbedo, inTexCoord * 2.0f);

        // Ambient part
        vec3 color = albedo.rgb * ambient;

        for (int i = 0; i < lightCount; ++i)
        {

            // Vector to light
            vec3 L = ubo.lights[i].position.xyz - position;
            // Distance from light to fragment position
            float dist = length(L);

            // Viewer to fragment
            vec3 V = ubo.cameraPosition - position;
            // vec3 V = vec3(0.0f, 0.0f, 300.0f) - position;
            V = normalize(V);

            float range = 25000.0f;
            if (dist < range)
            {
                // Light to fragment
                L = normalize(L);

                // Attenuation
                float atten = range / (pow(dist, 2.0) + 1.0);

                // Diffuse part
                vec3 N = normalize(normal);
                float NdotL = max(0.0, dot(N, L));
                vec3 diff = ubo.lights[i].color * albedo.rgb * NdotL * atten;

                // Specular part
                // Specular map values are stored in alpha of albedo mrt
                vec3 R = reflect(-L, N);
                float NdotR = max(0.0, dot(R, V));
                vec3 spec = ubo.lights[i].color * albedo.a * pow(NdotR, 16.0) * atten;

                color += diff + spec;
            }
        }

        outColor = vec4(color, 1.0f);
    }
    else if (inTexCoord.x < 0.5 && inTexCoord.y >= 0.5)
    {
        outColor = texture(texSamplerNormal, vec2(inTexCoord.x * 2.0f, inTexCoord.y * 2.0f - 1.0f));
    }
    else if (inTexCoord.x >= 0.5 && inTexCoord.y < 0.5)
    {
        outColor = texture(texSamplerAlbedo, vec2(inTexCoord.x * 2.0f - 1.0f, inTexCoord.y * 2.0f));
    }
    else
    {
        outColor = texture(texSamplerPosition, vec2(inTexCoord.x * 2.0f - 1.0f, inTexCoord.y * 2.0f - 1.0f));
    }
}