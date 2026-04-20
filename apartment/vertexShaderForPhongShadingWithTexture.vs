#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 VertexColor;  // For vertex-blended texture mode

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// For vertex color blending (Gouraud-style color passed to fragment)
uniform vec3 objectColor;

void main()
{
    FragPos   = vec3(model * vec4(aPos, 1.0));
    Normal    = mat3(transpose(inverse(model))) * aNormal;
    TexCoord  = aTexCoord;
    VertexColor = objectColor;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
