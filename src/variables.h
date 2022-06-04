// Define connections to BTT TMC2209 V1.2 UART
#define EN_PIN           23 // Enable
#define STEP_PIN         22 // Steps
#define DIR_PIN          21 // Direction
#define SERIAL_PORT Serial2 // TMC2209 HardwareSerial port
#define R_SENSE       0.11f // Match to your driver, SilentStepStick series use 0.11
#define DRIVER_ADDR    0b00 // 0b00 is slave, since there're no other drivers
#define LED_PIN           2 // LED tied to GPIO2 on HiLetGo board

// For connections
const char ssid[]      = "NalaSecretBase_2.4";  // Network SSID (name)
const char pass[]      = "2063832037s";         // Network password
const char mqttID[]    = "shade1";
const char mqttUser[]  = "mqtt-user";
const char mqttPass[]  = "jnkjnk37";
const char brokerIP[]  = "192.168.1.26";
int   brokerPort       = 1883;
const char inTopic[]   = "/server/shades/1";
const char outTopic[]  = "/client/blinds/1";


enum MotorState {
  MOTOR_DISABLE = 0,
  MOTOR_ENABLE  = 1,
};

enum CoverState {
  COVER_CLOSE   = -1,
  COVER_STOP    = -2,
  COVER_OPEN    = -3,
  COVER_SET_MAX = -4,
  COVER_SET_MIN = -5
};

const float gear_ratio = 5.18;  // Use 1 if stepper motor doesn't have a gearbox
const uint16_t microsteps = 8;
const int steps_per_rev = 200 * gear_ratio * microsteps;
int velocity = steps_per_rev;
int acceleration = steps_per_rev * 10;
MotorState motor = MOTOR_DISABLE;
CoverState cover = COVER_STOP;
uint32_t previous_position = 0;
bool set_max = false;
bool set_min = false;