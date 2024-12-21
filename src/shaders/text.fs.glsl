#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{
    float distance = texture(text, TexCoords).r;
    float aaf = fwidth(distance);
    float alpha = smoothstep(0.5 - aaf, 0.5 + aaf, distance);
    color = vec4(textColor.rgb, alpha);
}