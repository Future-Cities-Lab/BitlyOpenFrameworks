#version 150
in  vec3 in_Position;
void main() 
{
    gl_Position = vec4(in_Position.x, in_Position.y, in_Position.z, 1.0);
}
