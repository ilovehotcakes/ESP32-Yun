#include "motor_task.h"


MotorTask::MotorTask(const uint8_t task_core) : Task{"Motor", 4096, 1, task_core} {
    max_position_ = 0;
    current_position_ = 0;
    previous_position_ = 0;
    current_state_ = MOTOR_IDLE;
    previous_state_ = MOTOR_IDLE;
    is_motor_running_ = false;

    // TMCStepper driver setup
    pinMode(EN_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(STEP_PIN, OUTPUT);
    SERIAL_PORT.begin(115200, SERIAL_8N1, 16, TXD2); // Initialize HardwareSerial for hardware UART driver; remapped TXD2 from GPIO 16 to GPIO 22
    driver.begin();                 // Begin sending data
    driver.toff(4);                 // Not used in StealthChop but required to enable the motor, 0=off
    driver.pdn_disable(true);       // PDN_UART input disabled; set this bit when using the UART interface
    driver.rms_current(openingRMS); // Motor RMS current "rms_current will by default set ihold to 50% of irun but you can set your own ratio with additional second argument; rms_current(1000, 0.3)."
    driver.pwm_autoscale(true);     // Needed for StealthChop
    driver.en_spreadCycle(false);   // Disable SpreadCycle; SpreadCycle is faster but louder
    driver.blank_time(24);          // Comparator blank time. Needed to safely cover the switching event and the duration of the ringing on the sense resistor.
    driver.microsteps(microsteps);
    driver.shaft(flipDir);

    // Use StallGuard if user specifies DIAG_PIN & RXD2
    #ifdef DIAG_PIN
    if (enableSG) {
        pinMode(DIAG_PIN, INPUT);
        driver.semin(4);              // CoolStep/SmartEnergy 4-bit uint that sets lower threshold, 0=disable
        driver.semax(0);              // Refer to p58 of the datasheet
        driver.TCOOLTHRS((3089838.00 * pow(float(maxSpeed), -1.00161534)) * 1.5);  // Lower threshold velocity for switching on CoolStep and StallGuard to DIAG
        driver.SGTHRS(sgThreshold);   // [0..255] the higher the more sensitive to stall
        attachInterrupt(DIAG_PIN, std::bind(&MotorTask::stallguardInterrupt, this), RISING);
    }
    #endif

    // FastAccelStepper setup
    engine.init();
    stepper = engine.stepperConnectToPin(STEP_PIN);
    if (stepper) {
        stepper->setEnablePin(EN_PIN);
        stepper->setDirectionPin(DIR_PIN);
        stepper->setSpeedInHz(maxSpeed);
        stepper->setAcceleration(acceleration);
        stepper->setAutoEnable(true);
        stepper->setDelayToDisable(200);
    } else {
        // LOGE("Please use a different GPIO pin for STEP_PIN. The current pin is incompatible..");
    }

    // Load current position and maximum position from motor_setting_
    // loadSettings();
    // LOGI("Motor setup complete");
}


// For StallGuard
#ifdef DIAG_PIN
void IRAM_ATTR MotorTask::stallguardInterrupt() {
    stepper->forceStop();
    // LOGE("Motor stalled");
}
#endif


void MotorTask::setMotorState(MotorState newState) {
    previous_state_ = current_state_;
    current_state_ = newState;
}


void MotorTask::loadSettings() {
    motor_setting_.begin("local", false);
    max_position_ = motor_setting_.getInt("max_position_", 30000);
    current_position_ = motor_setting_.getInt("current_position_", 0);
    stepper->setCurrentPosition(current_position_);
    // LOGI("Motor settings loaded(curr/max): %d/%d", current_position_, max_position_);
}


void MotorTask::resetSettings() {
    motor_setting_.clear();
    ESP.restart();
}


int MotorTask::percentToSteps(int percent) const {
    float x = (float) percent * (float) max_position_ / 100.0;
    return (int) round(x);
}


// Motor must move first before is_motor_running_==true, else run will excute first before stepper stops
void MotorTask::moveTo(int newPos) {
    if ((previous_state_ == MOTOR_MAX && current_state_ == MOTOR_MIN)
    || (previous_state_ == MOTOR_MIN && current_state_ == MOTOR_MAX))
        stepper->stopMove();
    
    if (newPos != stepper->getCurrentPosition() && newPos <= max_position_) {
        stepper->moveTo(newPos);
        is_motor_running_ = true;
        // LOGD("Motor moving(tar/curr/max): %d/%d/%d", newPos, stepper->getCurrentPosition(), max_position_);
    }
}


void MotorTask::move(int percent) {
    // if openingRMS vs closingRMS
    moveTo(percentToSteps(percent));
}


void MotorTask::min() {
    setMotorState(MOTOR_MIN);
    driver.rms_current(openingRMS);
    stepper->setSpeedInHz(maxSpeed);
    moveTo(0);
}


void MotorTask::max() {
    setMotorState(MOTOR_MAX);
    driver.rms_current(closingRMS);
    stepper->setSpeedInHz(maxSpeed);
    moveTo(max_position_);
}


void MotorTask::setMin() {
    setMotorState(MOTOR_SET_MIN);
    driver.rms_current(openingRMS);
    stepper->setSpeedInHz(maxSpeed / 4);
    previous_position_ = stepper->getCurrentPosition();
    stepper->setCurrentPosition(INT_MAX);
    // LOGD("Motor setting new min position");
    moveTo(0);
}


void MotorTask::setMax() {
    setMotorState(MOTOR_SET_MAX);
    driver.rms_current(closingRMS);
    stepper->setSpeedInHz(maxSpeed / 4);
    max_position_ = INT_MAX;
    // LOGD("Motor setting new max position");
    moveTo(INT_MAX);
}


// Returns current rounded position percentage. 0 is open; 100 is closed.
// TODO: add inaccuracy mode
int MotorTask::currentPercentage() {
    if (current_position_ == 0)
        return 0;
    else
        return (int) round((float) current_position_ / (float) max_position_ * 100);
}


void MotorTask::stop() {
    stepper->forceStop();
    stepper->moveTo(stepper->getCurrentPosition());
}


void MotorTask::updatePosition() {
    if (current_state_ == MOTOR_SET_MAX) {
        max_position_ = stepper->getCurrentPosition();
        motor_setting_.putInt("max_position_", max_position_);
        // LOGD("Set max position, new max position: %d", max_position_);
    } else if (current_state_ == MOTOR_SET_MIN) {
        int distanceTraveled = INT_MAX - stepper->getCurrentPosition();
        max_position_ = max_position_ + distanceTraveled - previous_position_;
        motor_setting_.putInt("max_position_", max_position_);
        stepper->setCurrentPosition(0);
        // LOGD("Set min position, new max position: %d", max_position_);
    }

    setMotorState(MOTOR_IDLE);
    current_position_ = stepper->getCurrentPosition();
    motor_setting_.putInt("current_position_", current_position_);
    // LOGD("Motor stopped(curr/max): %d/%d", current_position_, max_position_);
}


void MotorTask::run() {
    Serial.println(stepper->getCurrentPosition());
    stepper->move(20000);
    Serial.println(stepper->targetPos());

    while (1) {
        if (is_motor_running_) {
            vTaskDelay(1);
            if (!stepper->isRunning()) {
                is_motor_running_ = false;

                updatePosition();

                // sendMqtt((String) motorCurrentPercentage());
            }
        }
    }
}