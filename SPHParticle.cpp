/*
 * Author:         Austin Brennan
 * University:     Clemson University
 * Course:         2D Fluid Simulation
 * Professor:      Dr. Jerry Tessendorf
 * Due Date:       3/8/2016
 */

#include <iostream>
#include "SPHParticle.h"

SPHParticle::SPHParticle(const vector2 _position, const vector2 _velocity, const float _radius) {
  position = _position;
  prev_position = _position;
  velocity = _velocity;
  radius = _radius;
  mass = 1.0f;
  color = {1.0, 0.0, 0.0};
  prev_carea = above_below;
  carea = above_below;
}

fcolor SPHParticle::getColor(unsigned char *collision_texture, int index) {
  fcolor color;

  color.r = collision_texture[index    ];
  color.g = collision_texture[index + 1];
  color.b = collision_texture[index + 2];

  return color;
}

void SPHParticle::setCollisionArea(unsigned char *collision_texture, int width, int height) {
  int x, y, size, index;
  fcolor color;
  collision_area new_carea = carea;

  size = width * height * 3;
  x = (int) ((position.x / 2.0f) * width);
  y = (int) ((position.y / 2.0f) * height);

  index = size - ((y * width * 3) + (((width - 1) - x) * 3));

  color = getColor(collision_texture, index);


  if (color.r == 0 and color.g == 0 and color.b == 0) {
    prev_carea = carea;
    new_carea = inside_obstruction;
  }
  else if (color.r == 255 and color.g == 255 and color.b == 255) {
    prev_carea = carea;
    new_carea = above_below;
  }
  else if (color.r == 255 and color.g == 0 and color.b == 0) {
    prev_carea = carea;
    new_carea = left_right;
  }

  // set new attributes;
  prev_position = position;
  prev_carea = carea;
  carea = new_carea;
}


