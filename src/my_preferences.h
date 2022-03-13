// Define macros for commands
# define MOVE 2
# define STOP 3

#include <Preferences.h>

Preferences preferences_local;

//
int command = STOP;
float open_percent;
int max_steps;
int current_position;

void load_preference() {
  current_position = preferences_local.getInt("current_position", 0);
  max_steps = preferences_local.getInt("max_steps", 100000);
}