/*
 * Author:         Austin Brennan
 * University:     Clemson University
 * Course:         2D Fluid Simulation
 * Professor:      Dr. Jerry Tessendorf
 * Due Date:       3/8/2016
 */

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glut.h>
#include <OpenImageIO/imageio.h>
#include "SPHSolver.h"
#include "CmdLineFind.h"
#include <fcntl.h>
#include <unistd.h>


using namespace std;
using namespace lux;
OIIO_NAMESPACE_USING

#define MAX_TIME_STEP 0.04166667f // cap the maximum time step to 24 frames per second

struct point {
  float x, y, z;
};

SPHSolver *fluid;
unsigned int iwidth = 512;
unsigned int iheight = 512;
unsigned int frame_count = 0;
timespec start_time;
string output_path;
bool write_to_output;
bool simulation_paused = false;
float *color_source;
unsigned int *display_map;
unsigned int shader_program;


//----------------------------------------------------
//
//  Error handler
//
//----------------------------------------------------


void handleError(const char* error_message, int kill)
{
  fprintf(stderr, "Error: %s\n\n", error_message);

  if (kill == 1)
    exit(-1);
}


//----------------------------------------------------
//
//  Read & Write images
//
//----------------------------------------------------


void convertToDisplay()
{
  for (int i = 0; i < iwidth*iheight*3; ++i) { display_map[i] = 250; }//(unsigned char) (color_source[i] * 255.0f); }
}


int readOIIOImage( const char* fname)
{
  int channels;
  ImageInput *in = ImageInput::create (fname);
  if (! in) { return -1; }
  ImageSpec spec;
  in->open (fname, spec);
  iwidth = (unsigned int) spec.width; // note iwidth and iheight are set to to image size
  iheight = (unsigned int) spec.height;
  channels = spec.nchannels;
  float* pixels = new float[iwidth*iheight*channels];
  color_source = new float[iwidth*iheight*channels]; // allocate appropriate space for image
  display_map = new unsigned int[iwidth*iheight*channels];

  in->read_image (TypeDesc::FLOAT, pixels);
  long index = 0;
  for( int j=0;j<iheight;j++)
  {
    for( int i=0;i<iwidth;i++ )
    {
      for( int c=0;c<channels;c++ )
      {
        color_source[ (i + iwidth*(iheight - j - 1))*channels + c ] = pixels[index++];
      }
    }
  }
  delete pixels;

  in->close ();
  delete in;

  return 0;
}


void writeImage() {
  char buffer[256];

  if (output_path.back() != '/') { output_path.push_back('/'); }
  if (sprintf(buffer, "%sfluid_simulator_%04d.jpg", output_path.c_str(), frame_count++) < 0) {
    handleError((const char *) "creating filename in writeImage() failed", 0);
    return;
  }
  const char *filename = buffer;
  const unsigned int channels = 3; // RGB
  float *write_pixels = new float[iwidth * iheight * channels];
  float *window_pixels = new float[iwidth * iheight * channels];
  ImageOutput *out = ImageOutput::create(filename);
  if (!out) {
    handleError((const char *) "creating output file in writeImage() failed", 0);
    return;
  }

  glReadPixels(0, 0, iwidth, iheight, GL_RGB, GL_FLOAT, window_pixels);
  long index = 0;
  for (unsigned int j = 0; j < iheight; j++) {
    for (unsigned int i = 0; i < iwidth; i++) {
      for (unsigned int c = 0; c < channels; c++) {
        write_pixels[(i + iwidth * (iheight - j - 1)) * channels + c] = window_pixels[index++]; //color[index++];
      }
    }
  }

  ImageSpec spec (iwidth, iheight, channels, TypeDesc::FLOAT);
  out->open (filename, spec);
  out->write_image (TypeDesc::FLOAT, write_pixels);
  out->close ();
  delete out;
  delete write_pixels;
  delete window_pixels;
}


//----------------------------------------------------
//
//  Setting up camera for OpenGL
//
//----------------------------------------------------


void setupTheViewVolume()
{
  struct point eye, view, up;

  // specify size and shape of view volume
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.0,1.0,-1.0,1.0,0.0,20.0);

  // specify position for view volume
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  eye.x = 1.0; eye.y = 1.0; eye.z = 1.0;
  view.x = 1.0; view.y = 1.0; view.z = 0.0;
  up.x = 0.0; up.y = 1.0; up.z = 0.0;

  gluLookAt(eye.x,eye.y,eye.z,view.x,view.y,view.z,up.x,up.y,up.z);
}


//----------------------------------------------------
//
//  OpenGL drawing commands
//
//----------------------------------------------------


void drawParticles() {
  glUseProgram((GLuint) 0);
  // draw particles
  glPointSize(2.5f);
  glBegin(GL_POINTS);
  vector<SPHParticle>::iterator pi = fluid->particles.begin();
  while(pi != fluid->particles.end()) {
    vector3 color = pi->color;
    glColor3f(color.x, color.y, color.z);
    vector2 position = pi->position;
    glVertex3f(position.x, position.y, 0.0f);
    ++pi;
  }
  glEnd();
  glFlush();
}



void set_texture() {
  convertToDisplay();
  glBindTexture(GL_TEXTURE_2D,1);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,iwidth,iheight,0,GL_RGB,
               GL_UNSIGNED_BYTE, display_map);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
}


void drawScene()
{
  struct point front[4] = {
      {0.0f,0.0f,-0.1f}, {0.0f,2.0f,-0.1f}, {2.0f,2.0f,-0.1f}, {2.0f,0.0f,-0.1f} };
  float mytexcoords[4][2] = {{0.0,0.0},{1.0,0.0},{1.0,1.0},{0.0,1.0}};


  set_texture();
  glClearColor(0.0,0.0,0.0,0.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glUseProgram((GLuint) shader_program);		// THIS IS IT!
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,1);
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glNormal3f(0.0,0.0,1.0);

  for(int i=0; i<4; i++) {
    glTexCoord2fv(mytexcoords[i]);
    glVertex3f(front[i].x,front[i].y,front[i].z);
  }
  glEnd();

  drawParticles();
}


//----------------------------------------------------
//
//  Setup Shader
//
//----------------------------------------------------

char *read_shader_program(char *filename)
{
  FILE *fp;
  char *content = NULL;
  int fd, count;
  fd = open(filename,O_RDONLY);
  count = (int) lseek(fd,0,SEEK_END);
  close(fd);
  content = (char *)calloc(1,(size_t)(count+1));
  fp = fopen(filename,"r");
  count = fread(content,sizeof(char),count,fp);
  content[count] = '\0';
  fclose(fp);
  return content;
}


unsigned int setShaders()
{
  char *vs, *fs;
  GLuint v, f, p;

  v = glCreateShader(GL_VERTEX_SHADER);
  f = glCreateShader(GL_FRAGMENT_SHADER);
  vs = read_shader_program((char *) "/home/awbrenn/Documents/workspace/fluid2D/final_project/sim_tex.vert");
  fs = read_shader_program((char *) "/home/awbrenn/Documents/workspace/fluid2D/final_project/sim_tex.frag");
  glShaderSource(v,1,(const char **)&vs,NULL);
  glShaderSource(f,1,(const char **)&fs,NULL);
  free(vs);
  free(fs);
  glCompileShader(v);
  glCompileShader(f);
  p = glCreateProgram();
  glAttachShader(p,f);
  glAttachShader(p,v);
  glLinkProgram(p);
  return(p);
}


void set_uniform_parameters(unsigned int p)
{
  int location;
  location = glGetUniformLocation(p,"mytexture");
  glUniform1i(location,0);
}


//----------------------------------------------------
//
//  Initialize Particles
//
//----------------------------------------------------


void initParticleSim(int number_of_particles, UPDATE_FUNCTION update_function, bool party_mode, float density_base,
                     float beta, float gamma, float viscosity, float epsilon, const float h) {

  srand (static_cast <unsigned> (time(0)));

  fluid = new SPHSolver(number_of_particles, 0.0f, 2.0f, h);
  fluid->update_function = update_function;
  fluid->party_mode = party_mode;

  // setting force parameters
  fluid->force.density_base = density_base;
  fluid->force.beta = beta;
  fluid->force.gamma = gamma;
  fluid->force.viscosity = viscosity;
  fluid->force.epsilon = epsilon;
}


//----------------------------------------------------
//
//  OpenGL Callbacks
//
//----------------------------------------------------


void callbackDisplay( void )
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  drawScene();
  glutSwapBuffers();
}


// animate and display new result
void callbackIdle() {
  float delta_time = (1.0f/48.0f);
  if (!simulation_paused) { fluid->update(delta_time); }
  if (write_to_output) { writeImage(); }
  glutPostRedisplay();
}


void callbackKeyboard( unsigned char key, int x, int y )
{
  float new_dampening;
  switch (key)
  {
    case ',' : case '<':
      new_dampening = fluid->dampening - 0.01f;
      fluid->dampening = new_dampening >= 0.0f ? new_dampening : 0.0f;
      cout << "Setting dampening. Bounce energy is " << fluid->dampening * 100 << "% of original energy" << endl;
      break;

    case '.': case '>':
      fluid->dampening += 0.01f;
    cout << "Setting dampening. Bounce energy is " << fluid->dampening * 100 << "% of original energy" << endl;
      break;

    case 'q':
    cout << "Exiting Program" << endl;
    exit(0);

    case 'w':
      fluid->force.gravity = {0.0f, 9.8f};
      cout << "Gravity is now up" << endl;
      break;

    case 'a':
      fluid->force.gravity = {-9.8f, 0.0f};
      cout << "Gravity is now left" << endl;
      break;

    case 's':
      fluid->force.gravity = {0.0f, -9.8f};
      cout << "Gravity is now down" << endl;
      break;

    case 'd':
      fluid->force.gravity = {9.8f, 0.0f};
      cout << "Gravity is now right" << endl;
      break;

    case 'p':
      fluid->party_mode = !fluid->party_mode;
      cout << (fluid->party_mode ? "Party mode is on" : "Party mode is off") << endl;
      break;

    case ' ':
      simulation_paused = !simulation_paused;
      cout << (simulation_paused ? "Simulation paused" : "Simulation un-paused") << endl;
      break;

    case 'o':
      write_to_output = !write_to_output;
      cout << (write_to_output ? "Starting writing to output" : "Ending writing to output") << endl;
      break;

    default:
    break;
  }
}


//----------------------------------------------------
//
//  Program usage
//
//----------------------------------------------------


void PrintUsage()
{
  cout << "sph_fluid_simulator keyboard choices\n";
  cout << "./,      increase/decrease % of energy retained after bounce\n";
  cout << "p        turn on party mode. Randomizes particle color on collison\n";
  cout << "w/a/s/d  switches gravity to point up/left/down/right\n";
  cout << "spacebar paused the simulation. pressing it again un-pauses the simulation\n";
  cout << "o        toggle writing to output_path\n";
  cout << "q        exits the program\n";
  cout << endl;
}


//----------------------------------------------------
//
//  Main
//
//----------------------------------------------------

void setNbCores( int nb )
{
  omp_set_num_threads( nb );
}

int main(int argc, char** argv) {
  CmdLineFind clf(argc, argv);
  output_path = clf.find("-output_path", "./output_images", "Output path for writing image sequence");
  int number_of_particles = clf.find("-particles", 250, "Number of particles in sim");
  int write_on_start = clf.find("-write_on_start", 0, "Flag for whether to start writing output images at the start of "
                                "the program or not");
  int party_mode = clf.find("-party_mode", 0, "Flag for starting in party mode or not");
  string update_function_str = clf.find("-update_function", "S", "Function in update (options 'LF' for leap frog or 'S'"
                                        " for sixth)");

  float density_base = clf.find("-density_base", 141.471060526f, "Base density for pressure calculation");
  float beta = clf.find("-beta", 1.0f, "Constant for pressure calculation");
  float gamma = clf.find("-gamma", 3.0f, "Gamma for pressure calculation");
  float viscosity = clf.find("-viscosity", 1.0f, "Viscosity of the fluid");
  float epsilon = clf.find("-epsilon", 0.1f, "Another factor used in the denominator of the viscosity calculation");

  // validate flags
  if (party_mode != 0 && party_mode != 1) { handleError("Invalid usage of party mode flag", true); }
  if (write_on_start != 0 && write_on_start != 1) { handleError("Invalid usage of write on start flag", true); }
  if (update_function_str.compare("S") != 0 && update_function_str.compare("LF") != 0)
    { handleError("Invalid usage of update_function flag", true); }

  // print key control usage
  PrintUsage();
  clf.usage("-h");
  clf.printFinds();

  // initialize particle simulation
  UPDATE_FUNCTION update_function;
  if (update_function_str.compare("S") == 0) { update_function = SIXTH; }
  else {update_function = LEAP_FROG; }

  float h = 0.15;

  initParticleSim(number_of_particles, update_function, party_mode != 0, density_base, beta, gamma, viscosity, epsilon, h);

  // decide whether to write to output or not
  write_to_output = write_on_start != 0;

  // get starting clock time for dynamic timestep in callbackIdle func
  clock_gettime(CLOCK_REALTIME, &start_time);

  setNbCores(4);

  readOIIOImage("/home/awbrenn/Documents/workspace/fluid2D/final_project/background.jpg");

  // initialize glut window
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_MULTISAMPLE);

  glutInitWindowSize(iwidth, iheight);
  glutInitWindowPosition(100, 50);
  glutCreateWindow("2D SPH Particle Simulation");
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE_ARB);
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  setupTheViewVolume();

  set_texture();
  shader_program = setShaders();
  set_uniform_parameters(shader_program);


  glutDisplayFunc(callbackDisplay);
  glutKeyboardFunc(&callbackKeyboard);
  glutIdleFunc(&callbackIdle);
  glutMainLoop();
  return 0;
}