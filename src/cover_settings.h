// Define connections to BTT TMC2209 V1.2 UART
#define EN_PIN           23 // Enable
#define STEP_PIN         22 // Steps
#define DIR_PIN          21 // Direction
#define SERIAL_PORT Serial2 // TMC2209 HardwareSerial port
#define R_SENSE       0.11f // Match to your driver, SilentStepStick series use 0.11
#define DRIVER_ADDR    0b00 // 0b00 is slave, since there're no other drivers

const float gear_ratio = 5.18;  // Use 1 if stepper motor doesn't have a gearbox
const uint16_t microsteps = 8;
const int steps_per_rev = 200 * gear_ratio * microsteps;
const int velocity = steps_per_rev;
const int acceleration = steps_per_rev * 10;
