#include "motor_task.h"


MotorTask::MotorTask(const uint8_t task_core) : Task{"Motor", 8192, 1, task_core} {
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
    assert("Failed to initialize FastAccelStepper. Please try a different GPIO for STEP_PIN." && stepper);
    stepper->setEnablePin(EN_PIN);
    stepper->setDirectionPin(DIR_PIN);
    stepper->setSpeedInHz(maxSpeed);
    stepper->setAcceleration(acceleration);
    stepper->setAutoEnable(true);
    stepper->setDelayToDisable(200);

    // Load current position and maximum position from motor_setting_
    loadSettings();

    // AS5600 rotary encoder setup
    as5600.begin(SDA_PIN, SCL_PIN);
    assert("Failed to initialize AS5600 rotary encoder." && as5600.isConnected());
    encoder_curr_pos_ = (int) round((float) motor_curr_pos_ * encoder_motor_ratio_);
    encoder_prev_pos_ = encoder_curr_pos_;
    as5600.resetCumulativePosition(encoder_curr_pos_);  // Start encoder from 0, offset

    last_updated_percentage_ = currentPercentage();

    while (1) {
        int command = -50;
        if (xQueueReceive(motor_command_queue_, (void*) &command, 0) == pdTRUE) {
            LOGD("MotorTask received command from motor_command_queue_: %i", command);
            switch (command) {
                case COVER_STOP:
                    stop();
                    break;
                case COVER_OPEN:
                    min();
                    break;
                case COVER_CLOSE:
                    max();
                    break;
                case COVER_SET_MAX:
                    setMax();
                    break;
                case COVER_SET_MIN:
                    setMin();
                    break;
                case SYS_RESET:
                    resetSettings();
                    break;
                case SYS_REBOOT:
                    ESP.restart();
                    break;
                default:
                    move(command);
            }
        }

        readEncoderPosition();

        // Can't put this in stallguard ISR
        if (is_motor_running_ && !stepper->isRunning()) {
            is_motor_running_ = false;
            updatePosition();
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
    motor_max_pos_ = motor_setting_.getInt("motor_max_pos_", 30000);
    motor_curr_pos_ = motor_setting_.getInt("motor_curr_pos_", 0);
    stepper->setCurrentPosition(motor_curr_pos_);
    LOGI("Motor settings loaded(curr/max): %d/%d", motor_curr_pos_, motor_max_pos_);
}


void MotorTask::resetSettings() {
    motor_setting_.clear();
    ESP.restart();
}


int MotorTask::percentToSteps(int percent) const {
    return (int) round((float) percent * (float) motor_max_pos_ / 100.0);
}


// Motor must move first before is_motor_running_==true, else run will excute first before stepper stops
void MotorTask::moveTo(int new_position) {
    // TODO check if it's greater than x degree before moving motor
    if ((previous_state_ == MOTOR_MAX && current_state_ == MOTOR_MIN)
    || (previous_state_ == MOTOR_MIN && current_state_ == MOTOR_MAX))
        stepper->stopMove();
    
    if (new_position != stepper->getCurrentPosition() && new_position <= motor_max_pos_) {
        stepper->moveTo(new_position);
        is_motor_running_ = true;
        LOGD("Motor moving(curr/max -> tar): %d/%d -> %d", motor_curr_pos_, motor_max_pos_, new_position);
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
    moveTo(motor_max_pos_);
}


void MotorTask::setMin() {
    setMotorState(MOTOR_SET_MIN);
    driver.rms_current(openingRMS);
    stepper->setSpeedInHz(maxSpeed / 4);
    motor_curr_pos_ = stepper->getCurrentPosition();
    stepper->setCurrentPosition(INT_MAX);
    LOGD("Motor setting new min position");
    moveTo(0);
}


void MotorTask::setMax() {
    setMotorState(MOTOR_SET_MAX);
    driver.rms_current(closingRMS);
    stepper->setSpeedInHz(maxSpeed / 4);
    motor_max_pos_ = INT_MAX;
    LOGD("Motor setting new max position");
    moveTo(INT_MAX);
}


// Returns current rounded position percentage. 0 is open; 100 is closed.
// TODO: add inaccuracy mode
int MotorTask::currentPercentage() {
    if (motor_curr_pos_ == 0)
        return 0;
    else
        return (int) round((float) motor_curr_pos_ / (float) motor_max_pos_ * 100);
}


void MotorTask::stop() {
    stepper->forceStop();
    stepper->moveTo(stepper->getCurrentPosition());
}


void MotorTask::updatePosition() {
    if (current_state_ == MOTOR_SET_MAX) {
        motor_max_pos_ = stepper->getCurrentPosition();
        motor_setting_.putInt("motor_max_pos_", motor_max_pos_);
        LOGD("Set max position, new max position: %d", motor_max_pos_);
    } else if (current_state_ == MOTOR_SET_MIN) {
        int distanceTraveled = INT_MAX - stepper->getCurrentPosition();
        motor_max_pos_ = motor_max_pos_ + distanceTraveled - motor_curr_pos_;
        motor_setting_.putInt("motor_max_pos_", motor_max_pos_);
        stepper->setCurrentPosition(0);
        LOGD("Set min position, new max position: %d", motor_max_pos_);
    }

    setMotorState(MOTOR_IDLE);
    motor_curr_pos_ = stepper->getCurrentPosition();
    motor_setting_.putInt("motor_curr_pos_", motor_curr_pos_);
    LOGD("Motor stopped(curr/max): %d/%d", motor_curr_pos_, motor_max_pos_);

    int current_precentage = currentPercentage();
    if (abs(current_precentage - last_updated_percentage_) >= 2 && current_precentage >= 0 && current_precentage <= 100) {
        last_updated_percentage_ = current_precentage;
        if (xQueueSend(wireless_message_queue_, (void*) &current_precentage, 10) != pdTRUE)
            LOGE("Failed to send to wireless_message_queue_.");
    }
}


void MotorTask::addListener(QueueHandle_t queue) {
    wireless_message_queue_ = queue;
}

QueueHandle_t MotorTask::getMotorCommandQueue() {
    return motor_command_queue_;
}

void MotorTask::readEncoderPosition() {
    encoder_prev_pos_ = encoder_curr_pos_;
    encoder_curr_pos_ = as5600.getCumulativePosition();

    motor_curr_pos_ = (int) round((float) encoder_curr_pos_ * motor_encoder_ratio_);
    stepper->setCurrentPosition(motor_curr_pos_);

    if (!is_motor_running_) {
        // Filter out minor changes
        if (abs(encoder_curr_pos_ - encoder_prev_pos_) >= 5 && !waiting_) {
            waiting_ = true;
        }

        if (abs(encoder_curr_pos_ - encoder_prev_pos_) < 5 && waiting_) {
            waiting_ = false;
            updatePosition();
            // Serial.println(encoder_curr_pos_);
        }
    }
}