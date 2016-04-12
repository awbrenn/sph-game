// Phong lighting in eye coordinates with texture and pure Phong specular.

// These are set by the .vert code, interpolated.
varying vec3 ec_vnormal, ec_vposition;

// This is set by the .c code.
uniform sampler2D mytexture;

void main()
{
    vec3 tcolor;
    tcolor = vec3(texture2D(mytexture,gl_TexCoord[0].st/gl_TexCoord[0].q));

    gl_FragColor = vec4(tcolor, 0.0f);
}