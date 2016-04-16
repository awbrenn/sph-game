/*
 * Author:         Austin Brennan
 * University:     Clemson University
 * Course:         2D Fluid Simulation
 * Professor:      Dr. Jerry Tessendorf
 * Due Date:       3/8/2016
 */

#ifndef SPHPARTICLE_H
#define SPHPARTICLE_H
#include "vector2.h"

typedef struct fcolor {
  float r;
  float g;
  float b;
} fcolor;

typedef enum collision_area {
  left_right = 0,
  above_below = 1,
  inside_obstruction = 2,
  left_right_and_above_below = 3,
  finished = 4
} collision_area;


class SPHParticle {
  private:
  fcolor getColor(unsigned char *collision_texture, int index);

  public:
    vector2 position;
    vector2 prev_position;
    vector2 velocity;
    vector2 acceleration;
    fcolor color;
    float radius;
    float mass;
    float density;
    float pressure;
    collision_area prev_carea;
    collision_area carea;
    void setCollisionArea(unsigned char *collision_texture, int width, int height);
    void capVelocity(float max_velocity);
    SPHParticle(const vector2 _position, const vector2 _velocity, const float radius);
};


#endif //SPHPARTICLE_H
