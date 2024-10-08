/* quad vertex shader */
@vs vs
in vec4 position;
in vec4 color0;
// in vec2 texcoord0;
out vec4 color;
out vec2 uv;

void main() {
    gl_Position = position;
    color = vec4(1.0, 1.0, 1.0, 1.0);//color0;
    // uv = texcoord0;
    uv = vec2(
        position.x + 0.5,
        -position.y + 0.5
    );
}
@end

/* quad fragment shader */
@fs fs
uniform texture2D tex;
uniform sampler smp;

in vec4 color;
in vec2 uv;
out vec4 frag_color;

void main() {
    // frag_color = color;
    vec4 tex0 = texture(sampler2D(tex, smp), uv).rgba;
    frag_color = vec4(tex0.rgb, 1.0);
}
@end

/* quad shader program */
@program quad vs fs
