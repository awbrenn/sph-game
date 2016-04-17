#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#include "SPHSolver.h"
#include "gameLevel.h"
#include "textScreen.h"
#include "gameController.h"

using namespace std;

float eye[] = {1.0f,1.0f,1.0f};
float viewpt[] = {1.0,1.0,0.0};
float up[] = {0.0,1.0,0.0};
GLuint sprogram[2];
gameController game_controller;


char *read_shader_program(char *filename)
{
  FILE *fp;
  char *content = NULL;
  int fd, count;
  fd = open(filename,O_RDONLY);
  count = lseek(fd,0,SEEK_END);
  close(fd);
  content = (char *)calloc(1,(count+1));
  fp = fopen(filename,"r");
  count = fread(content,sizeof(char),count,fp);
  content[count] = '\0';
  fclose(fp);
  return content;
}

void load_background_texture(char *filename) {
  FILE *fptr;
  char buf[512], *parse;
  int im_size, im_width, im_height, max_color;
  unsigned char *texture_bytes;

  fptr=fopen(filename,"r");
  fgets(buf,512,fptr);
  do{
    fgets(buf,512,fptr);
  } while(buf[0]=='#');
  parse = (char *)strtok(buf," \t");
  im_width = atoi(parse);

  parse = (char *)strtok(NULL," \n");
  im_height = atoi(parse);

  fgets(buf,512,fptr);
  parse = (char *)strtok(buf," \n");
  max_color = atoi(parse);

  im_size = im_width*im_height;
  texture_bytes = (unsigned char *)calloc(3,im_size);
  fread(texture_bytes,3,im_size,fptr);
  fclose(fptr);

  glBindTexture(GL_TEXTURE_2D,1);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,im_width,im_height,0,GL_RGB,
               GL_UNSIGNED_BYTE,texture_bytes);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  free(texture_bytes);
}

void load_river_texture() {
  size_t texture_size;
  unsigned char *texture_bytes;

  texture_size = (size_t) game_controller.current_level->cfd_fluid->Nx * game_controller.current_level->cfd_fluid->Ny;
  texture_bytes = (unsigned char *)calloc(3, texture_size);

  game_controller.current_level->cfd_fluid->river_texture = texture_bytes;

  glBindTexture(GL_TEXTURE_2D,2);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB, game_controller.current_level->cfd_fluid->Nx,
               game_controller.current_level->cfd_fluid->Ny, 0, GL_RGB,
               GL_UNSIGNED_BYTE,game_controller.current_level->cfd_fluid->river_texture);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
}

void update_river_texture() {
  game_controller.current_level->cfd_fluid->convertColorToTexture();

  glBindTexture(GL_TEXTURE_2D,2);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB, game_controller.current_level->cfd_fluid->Nx,
               game_controller.current_level->cfd_fluid->Ny, 0, GL_RGB,
               GL_UNSIGNED_BYTE,game_controller.current_level->cfd_fluid->river_texture);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
}

void load_collision_texture(char *filename)
{
  FILE *fptr;
  char buf[512], *parse;
  int im_size, im_width, im_height, max_color;
  unsigned char *texture_bytes;

  fptr=fopen(filename,"r");
  fgets(buf,512,fptr);
  do{
    fgets(buf,512,fptr);
  } while(buf[0]=='#');
  parse = (char *)strtok(buf," \t");
  im_width = atoi(parse);

  parse = (char *)strtok(NULL," \n");
  im_height = atoi(parse);

  fgets(buf,512,fptr);
  parse = (char *)strtok(buf," \n");
  max_color = atoi(parse);

  im_size = im_width*im_height;
  texture_bytes = (unsigned char *)calloc(3,im_size);
  fread(texture_bytes,3,im_size,fptr);
  fclose(fptr);

  game_controller.current_level->sph_fluid->collision_texture = texture_bytes;
  game_controller.current_level->sph_fluid->ct_width = im_width;
  game_controller.current_level->sph_fluid->ct_height = im_height;
}

void view_volume()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.0,1.0,-1.0,1.0,0.0,20.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(eye[0],eye[1],eye[2],viewpt[0],viewpt[1],viewpt[2],up[0],up[1],up[2]);
}


void drawParticles() {
  glUseProgram(0);
  // draw particles
  glPointSize(7.5f);
  glBegin(GL_POINTS);
  std::vector<SPHParticle>::iterator pi = game_controller.current_level->sph_fluid->particles.begin();
  while(pi != game_controller.current_level->sph_fluid->particles.end()) {
    fcolor color = pi->color;
    glColor3f(color.r, color.g, color.b);
    //glColor3f(1.0f, 0.0f, 0.0f);

    vector2 position = pi->position;
    glVertex3f(position.x, position.y, 0.0f);
    ++pi;
  }
  glEnd();

}


void drawRiver() {
  update_river_texture();
  int i;
//  float river_tile[4][3]={{0.0,0.0,0.1f},{2.0,0.0,0.1f},{2.0,0.5,0.1f},{0.0,0.5,0.1f}};
  float river_tile[4][3]={{0.0,0.0,0.1f},{2.0,0.0,0.1f},{2.0,0.5,0.1f},{0.0,0.5,0.1f}};
  float mytexcoords[4][2] = {{0.0,1.0},{1.0,1.0},{1.0,0.0},{0.0,0.0}};

//  glEnable (GL_BLEND);
//  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glUseProgram(sprogram[1]);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D,2);
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glNormal3f(0.0,0.0,1.0);
  for(i=0;i<4;i++){
    glTexCoord2fv(mytexcoords[i]);
    glVertex3f(river_tile[i][0],river_tile[i][1],river_tile[i][2]);
  }
  glEnd();
//  glDisable(GL_BLEND);
}


void render_scene()
{
  int i;
  float front[4][3]={{0.0,0.0,-0.1f},{2.0,0.0,-0.1f},{2.0,2.0,-0.1f},{0.0,2.0,-0.1f}};
  float mytexcoords[4][2] = {{0.0,1.0},{1.0,1.0},{1.0,0.0},{0.0,0.0}};

  glClearColor(0.0,0.0,0.0,1.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glUseProgram(sprogram[0]);		// THIS IS IT!
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,1);
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glNormal3f(0.0,0.0,1.0);
  for(i=0;i<4;i++){
    glTexCoord2fv(mytexcoords[i]);
    glVertex3f(front[i][0],front[i][1],front[i][2]);
  }
  glEnd();

  if (game_controller.game_mode == level) {
    drawRiver();
    glDisable(GL_TEXTURE_2D);
    drawParticles();
  } else { glDisable(GL_TEXTURE_2D); }

  glFlush();
  glutSwapBuffers();
}


unsigned int set_shaders(char * vert, char * frag)
{
  char *vs, *fs;
  GLuint v, f, p;

  v = glCreateShader(GL_VERTEX_SHADER);
  f = glCreateShader(GL_FRAGMENT_SHADER);
  vs = read_shader_program(vert);
  fs = read_shader_program(frag);
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
// glUseProgram(p);
  return(p);
}

void set_uniform_parameters(unsigned int p, char * texture_name)
{
  int location;
  location = glGetUniformLocation(p, texture_name);
  glUniform1i(location,0);
}


void loadLevel(unsigned int level_index) {
  // update the game_controller
  game_controller.current_level = &game_controller.levels[level_index];
  game_controller.level_index = level_index;
  game_controller.game_mode = level;
  game_controller.level_index++;

  // load textures
  load_background_texture(game_controller.current_level->background_texture);
  load_collision_texture(game_controller.current_level->collision_texture);
  load_river_texture();
}

void loadScreen(unsigned int screen_index) {
  // update the game_controller
  game_controller.screen_index = screen_index;
  game_controller.game_mode = screen;
  game_controller.screen_index++;

  // load screen background texture
  load_background_texture(game_controller.screens[screen_index].background_texture);

  // check for game over condition
  if (screen_index == game_controller.levels.size()) {
    game_controller.game_over = true;
    game_controller.level_index = 0;
    game_controller.screen_index = 0;
  }
}

void storeLevels() {
  game_controller.levels.push_back(gameLevel(200, 0.0, 2.0, vector2(0.5f, 0.5f), vector2(0.5f, 0.2f), 0.15, 1024, 256,
                                             (char *) "textures/art/background.ppm",
                                             (char *) "textures/collision/background_ct.ppm"));
  game_controller.levels.push_back(gameLevel(200, 0.0, 2.0, vector2(1.3f, 0.15f), vector2(1.75f, 0.0f), 0.15, 1024, 256,
                                             (char *) "textures/art/background2.ppm",
                                             (char *) "textures/collision/background2_ct.ppm"));
  game_controller.levels.push_back(gameLevel(200, 0.0, 2.0, vector2(0.75f, 0.75f), vector2(1.75f, 0.0f), 0.15, 1024, 256,
                                             (char *) "textures/art/background3.ppm",
                                             (char *) "textures/collision/background3_ct.ppm"));
}

void storeTextScreens() {
  game_controller.screens.push_back(textScreen((char *)"textures/art/title.ppm"));
  game_controller.screens.push_back(textScreen((char *)"textures/art/screen1.ppm"));
  game_controller.screens.push_back(textScreen((char *)"textures/art/game-over.ppm"));
  game_controller.screens.push_back(textScreen((char *)"textures/art/game-over.ppm"));
}

void callbackIdle() {
  if (game_controller.game_mode == level) {
    float delta_time = (1.0f / 48.0f);
    game_controller.current_level->sph_fluid->update(delta_time);
    game_controller.current_level->cfd_fluid->update();
    game_controller.level_completion = 1.0f -
            ((float) (game_controller.current_level->sph_fluid->current_particles_count) /
             (float) (game_controller.current_level->sph_fluid->start_particles_count));
    if (game_controller.level_completion == 1.0f) { loadScreen(game_controller.screen_index); }
  }
  glutPostRedisplay();
}

void callbackKeyboard( unsigned char key, int x, int y )
{
  float gravity_magnitude = 2.5f;
  switch (key)
  {
    case 'q':
    cout << "Exiting Program" << endl;
    exit(0);

    case 'w':
      if (game_controller.game_mode == level) {
        game_controller.current_level->sph_fluid->force.gravity = {0.0f, gravity_magnitude};
        cout << "Gravity is now up" << endl;
      }
      break;

    case 'a':
      if (game_controller.game_mode == level) {
        game_controller.current_level->sph_fluid->force.gravity = {-1.0f * gravity_magnitude, 0.0f};
        cout << "Gravity is now left" << endl;
      }
      break;

    case 's':
      if (game_controller.game_mode == level) {
        game_controller.current_level->sph_fluid->force.gravity = {0.0f, -1.0f * gravity_magnitude};
        cout << "Gravity is now down" << endl;
      }
      break;

    case 'd':
      if (game_controller.game_mode == level) {
        game_controller.current_level->sph_fluid->force.gravity = {gravity_magnitude, 0.0f};
        cout << "Gravity is now right" << endl;
      }
      break;
    case (char) 13: // enter_pressed
      if (game_controller.game_mode == screen && !game_controller.game_over) {
        loadLevel(game_controller.level_index);
      }
      else if (game_controller.game_mode == screen && game_controller.game_over) { // quiting when done
        cout << "Thanks for playing!" << endl;
        exit(0);
      }
      break;
    case 'r':
      if (game_controller.game_over) { // replay
        game_controller.levels.clear();
        storeLevels();
        game_controller.game_over = false;
        loadScreen(game_controller.screen_index);
      }
    default:
    break;
  }
}


int main(int argc, char **argv)
{
  storeLevels();
  storeTextScreens();

  // debugging reasons
//  game_controller.level_index = 2;

  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA| GLUT_MULTISAMPLE);
  glutInitWindowPosition(100, 100);
  glutInitWindowSize(768,768);
  glutCreateWindow("sph-game");
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE_ARB);

  loadScreen(0);

  view_volume();
  sprogram[0] = set_shaders((char *) "background_shader.vert", (char *) "background_shader.frag");
  sprogram[1] = set_shaders((char *) "river.vert", (char *) "river.frag");
  set_uniform_parameters(sprogram[0], (char *) "mytexture");
  set_uniform_parameters(sprogram[1], (char *) "river_texture");
  glutDisplayFunc(render_scene);
  glutIdleFunc(&callbackIdle);
  glutKeyboardFunc(callbackKeyboard);
  glutMainLoop();
  return 0;
}