#include <Preferences.h>

Preferences preferences_local;

//
int open_percent;
int max_steps;
int current_position;
bool motor_is_running = false;

void load_preference() {
  max_steps = preferences_local.getInt("max_steps", 100000);
  current_position = preferences_local.getInt("current_position", 0);
  open_percent = (int) ((float) current_position / (float) max_steps * 100);
}