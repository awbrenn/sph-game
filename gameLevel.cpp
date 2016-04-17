//
// Created by awbrenn on 4/13/16.
//

#include "gameLevel.h"

gameLevel::gameLevel(unsigned int number_of_particles, float lower_bound, float upper_bound, vector2 x_offsets, vector2 y_offsets,
                     float h, int river_width, int river_height, char * _background_texture, char * _collision_texture) {
  sph_fluid = new SPHSolver(number_of_particles, lower_bound, upper_bound, x_offsets, y_offsets, h);
  sph_fluid->update_function = LEAP_FROG;
  sph_fluid->party_mode = false;

  cfd_fluid = new CFDSolver(river_width, river_height, 1.0, (float)(1.0/24.0), 1, 1);

  // setting force parameters
  sph_fluid->force.density_base = 141.471060526f;
  sph_fluid->force.beta = 1.0f;
  sph_fluid->force.gamma = 3.0f;
  sph_fluid->force.viscosity = 1.0f;
  sph_fluid->force.epsilon = 0.1f;
  sph_fluid->max_velocity = 8.0f;

  background_texture = _background_texture;
  collision_texture = _collision_texture;
}

