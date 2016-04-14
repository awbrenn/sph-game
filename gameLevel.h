//
// Created by awbrenn on 4/13/16.
//

#ifndef SPH_PARTICLE_GAMELEVEL_H
#define SPH_PARTICLE_GAMELEVEL_H

#include "SPHSolver.h"

class gameLevel {
  public:
    SPHSolver *fluid;
    int number_of_particles;
    vector2 x_offsets;
    vector2 y_offsets;
    char * background_texture;
    char * collision_texture;

    gameLevel(unsigned int number_of_particles, float lower_bound, float upper_bound, vector2 x_offsets, vector2 y_offsets,
              float h, char * background_texture, char * collision_texture);
};


#endif //SPH_PARTICLE_GAMELEVEL_H
