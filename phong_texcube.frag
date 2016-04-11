// Phong lighting in eye coordinates with texture and pure Phong specular.

// These are set by the .vert code, interpolated.
varying vec3 ec_vnormal, ec_vposition;

// This is set by the .c code.
uniform sampler2D mytexture;

void main()
{
    vec3 P, N, L, V, R, tcolor;
    vec4 diffuse_color = gl_FrontMaterial.diffuse;
    vec4 specular_color = gl_FrontMaterial.specular;
    float shininess = gl_FrontMaterial.shininess;

    P = ec_vposition;
    N = normalize(ec_vnormal);
    L = normalize(gl_LightSource[0].position - P);
    V = normalize(-P);				// eye position is (0,0,0)!
    R = -L + 2.0*dot(L,N)*N;

    // perspective correction:
    tcolor = vec3(texture2D(mytexture,gl_TexCoord[0].st/gl_TexCoord[0].q));
    diffuse_color = 0.1*diffuse_color + 0.9*vec4(tcolor,1.0);
    diffuse_color *= max(dot(N,L),0.0);
    specular_color *= pow(max(dot(R,V),0.0),shininess);
    gl_FragColor = diffuse_color + specular_color;
}