//
// Created by awbrenn on 4/14/16.
//

#ifndef SPH_PARTICLE_GAMECONTROLLER_H
#define SPH_PARTICLE_GAMECONTROLLER_H

#include <stdlib.h>
#include "gameLevel.h"
#include "textScreen.h"

typedef enum mode {
  screen,
  level
} mode;

class gameController {
  public:
    unsigned int level_index = 0;
    unsigned int screen_index = 0;
    float level_completion = 0.0f;
    gameLevel * current_level;
    std::vector<gameLevel> levels;
    std::vector<textScreen> screens;
    mode game_mode;
};


#endif //SPH_PARTICLE_GAMECONTROLLER_H
