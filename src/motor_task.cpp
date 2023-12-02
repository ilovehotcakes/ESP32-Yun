#include "motor_task.h"


MotorTask::MotorTask(const uint8_t task_core) : Task{"Motor", 4096, 1, task_core} {
    motor_command_queue_ = xQueueCreate(1, sizeof(int));
    assert("Failed to create motor_command_queue_" && motor_command_queue_ != NULL);
}


MotorTask::~MotorTask() {
    vQueueDelete(motor_command_queue_);
}


void MotorTask::run() {
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
    assert("Failed to instantiate FastAccelStepper. Please try a different GPIO for STEP_PIN." && stepper);
    stepper->setEnablePin(EN_PIN);
    stepper->setDirectionPin(DIR_PIN);
    stepper->setSpeedInHz(maxSpeed);
    stepper->setAcceleration(acceleration);
    stepper->setAutoEnable(true);
    stepper->setDelayToDisable(200);

    // Load current position and maximum position from motor_setting_
    loadSettings();

    while (1) {
        int command = -50;
        if (xQueueReceive(motor_command_queue_, (void*) &command, 0) == pdTRUE) {
            LOGD("MotorTask received command from motor_command_queue_: %i", command);
            
            if (command >= 0) move(command);
            else if (command == COVER_STOP) stop();
            else if (command == COVER_OPEN) min();
            else if (command == COVER_CLOSE) max();
            else if (command == COVER_SET_MAX) setMax();
            else if (command == COVER_SET_MIN) setMin();
            else if (command == SYS_RESET) resetSettings();
            else if (command == SYS_REBOOT) ESP.restart();
        }
        if (is_motor_running_ && !stepper->isRunning()) {
            is_motor_running_ = false;

            updatePosition();

            // sendMqtt((String) motorCurrentPercentage()
        }
    }
}


// For StallGuard
#ifdef DIAG_PIN
void IRAM_ATTR MotorTask::stallguardInterrupt() {
    stepper->forceStop();
    LOGE("Motor stalled");
}
#endif


void MotorTask::setMotorState(MotorState newState) {
    previous_state_ = current_state_;
    current_state_ = newState;
}


void MotorTask::loadSettings() {
    motor_setting_.begin("local", false);
    max_pos_ = motor_setting_.getInt("max_pos_", 30000);
    previous_pos_ = motor_setting_.getInt("previous_pos_", 0);
    stepper->setCurrentPosition(previous_pos_);
    LOGI("Motor settings loaded(curr/max): %d/%d", previous_pos_, max_pos_);
}


void MotorTask::resetSettings() {
    motor_setting_.clear();
    ESP.restart();
}


int MotorTask::percentToSteps(int percent) const {
    float x = (float) percent * (float) max_pos_ / 100.0;
    return (int) round(x);
}


// Motor must move first before is_motor_running_==true, else run will excute first before stepper stops
void MotorTask::moveTo(int newPos) {
    if ((previous_state_ == MOTOR_MAX && current_state_ == MOTOR_MIN)
    || (previous_state_ == MOTOR_MIN && current_state_ == MOTOR_MAX))
        stepper->stopMove();
    
    if (newPos != stepper->getCurrentPosition() && newPos <= max_pos_) {
        stepper->moveTo(newPos);
        is_motor_running_ = true;
        LOGD("Motor moving(tar/curr/max): %d/%d/%d", newPos, stepper->getCurrentPosition(), max_pos_);
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
    moveTo(max_pos_);
}


void MotorTask::setMin() {
    setMotorState(MOTOR_SET_MIN);
    driver.rms_current(openingRMS);
    stepper->setSpeedInHz(maxSpeed / 4);
    current_pos_ = stepper->getCurrentPosition();
    stepper->setCurrentPosition(INT_MAX);
    LOGD("Motor setting new min position");
    moveTo(0);
}


void MotorTask::setMax() {
    setMotorState(MOTOR_SET_MAX);
    driver.rms_current(closingRMS);
    stepper->setSpeedInHz(maxSpeed / 4);
    max_pos_ = INT_MAX;
    LOGD("Motor setting new max position");
    moveTo(INT_MAX);
}


// Returns current rounded position percentage. 0 is open; 100 is closed.
// TODO: add inaccuracy mode
int MotorTask::currentPercentage() {
    if (previous_pos_ == 0)
        return 0;
    else
        return (int) round((float) previous_pos_ / (float) max_pos_ * 100);
}


void MotorTask::stop() {
    stepper->forceStop();
    stepper->moveTo(stepper->getCurrentPosition());
}


void MotorTask::updatePosition() {
    if (current_state_ == MOTOR_SET_MAX) {
        max_pos_ = stepper->getCurrentPosition();
        motor_setting_.putInt("max_pos_", max_pos_);
        LOGD("Set max position, new max position: %d", max_pos_);
    } else if (current_state_ == MOTOR_SET_MIN) {
        int distanceTraveled = INT_MAX - stepper->getCurrentPosition();
        max_pos_ = max_pos_ + distanceTraveled - current_pos_;
        motor_setting_.putInt("max_pos_", max_pos_);
        stepper->setCurrentPosition(0);
        LOGD("Set min position, new max position: %d", max_pos_);
    }

    setMotorState(MOTOR_IDLE);
    previous_pos_ = stepper->getCurrentPosition();
    motor_setting_.putInt("previous_pos_", previous_pos_);
    LOGD("Motor stopped(curr/max): %d/%d", previous_pos_, max_pos_);

    int current_precentage = currentPercentage();
    if (xQueueSend(wireless_message_queue_, (void*) &current_precentage, 10) != pdTRUE) {
        LOGE("Failed to send to wireless_message_queue_.");
    }
}


void MotorTask::addListener(QueueHandle_t queue) {
    wireless_message_queue_ = queue;
}

QueueHandle_t MotorTask::getMotorCommandQueue() {
    return motor_command_queue_;
}