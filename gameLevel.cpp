//
// Created by awbrenn on 4/13/16.
//

#include "gameLevel.h"

gameLevel::gameLevel(unsigned int number_of_particles, float lower_bound, float upper_bound, vector2 x_offsets, vector2 y_offsets,
                     float h, char * _background_texture, char * _collision_texture) {
  fluid = new SPHSolver(number_of_particles, lower_bound, upper_bound, x_offsets, y_offsets, h);
  fluid->update_function = LEAP_FROG;
  fluid->party_mode = false;

  // setting force parameters
  fluid->force.density_base = 141.471060526f;
  fluid->force.beta = 1.0f;
  fluid->force.gamma = 3.0f;
  fluid->force.viscosity = 1.0f;
  fluid->force.epsilon = 0.1f;
  fluid->max_velocity = 8.0f;

  background_texture = _background_texture;
  collision_texture = _collision_texture;
}

