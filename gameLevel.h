//
// Created by awbrenn on 4/13/16.
//

#ifndef SPH_PARTICLE_GAMELEVEL_H
#define SPH_PARTICLE_GAMELEVEL_H

#include"SPHSolver.h"
#include "CFDSolver.h"

struct point {
  float x, y, z;
};

class gameLevel {
  public:
    SPHSolver *sph_fluid;
    CFDSolver *cfd_fluid;
    char * background_texture;
    char * collision_texture;
    char * river_texture;

    gameLevel(unsigned int number_of_particles, float lower_bound, float upper_bound, vector2 x_offsets, vector2 y_offsets,
              float h, int river_width, int river_height, char * background_texture, char * collision_texture);
};


#endif //SPH_PARTICLE_GAMELEVEL_H
