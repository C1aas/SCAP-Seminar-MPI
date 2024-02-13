#pragma once
#include "game_of_life.h"

#include <stdbool.h>

void quitWithHelpMessage(char* name);
bool parseBool(char* value, char* name);
long parseLong(char* value, char* name, long min, long max);
GameConfig parseArguments(int argc, char** argv);
