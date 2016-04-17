// This is set by the .c code.
uniform sampler2D river_texture;

void main()
{
    vec3 tcolor;
    float alpha;

    // perspective correction:
    tcolor = vec3(texture2D(mytexture,gl_TexCoord[1].st/gl_TexCoord[1].q));

    alpha = tcolor.x + tcolor.y + tcolor.z;
    if (alpha > 1.0f) { alpha = 1.0; }

    gl_FragColor = vec4(tcolor, alpha);
}