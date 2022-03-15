#include <Preferences.h>

Preferences preferences_local;

// Global variables
static uint8_t open_percent;
static uint32_t max_steps;
static uint32_t current_position;

void load_preference() {
  max_steps = preferences_local.getInt("max_steps", 100000);
  current_position = preferences_local.getInt("current_position", 0);
}