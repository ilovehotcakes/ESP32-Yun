#include "motor_task.h"


MotorTask::MotorTask(const uint8_t task_core) : Task{"Motor", 8192, 1, task_core} {
    motor_command_queue_ = xQueueCreate(1, sizeof(int));
    assert(motor_command_queue_ != NULL);
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
    tmc2099.begin();                 // Begin sending data
    tmc2099.toff(4);                 // Not used in StealthChop but required to enable the motor, 0=off
    tmc2099.pdn_disable(true);       // PDN_UART input disabled; set this bit when using the UART interface
    tmc2099.rms_current(openingRMS); // Motor RMS current "rms_current will by default set ihold to 50% of irun but you can set your own ratio with additional second argument; rms_current(1000, 0.3)."
    tmc2099.pwm_autoscale(true);     // Needed for StealthChop
    tmc2099.en_spreadCycle(false);   // Disable SpreadCycle; SpreadCycle is faster but louder
    tmc2099.blank_time(24);          // Comparator blank time. Needed to safely cover the switching event and the duration of the ringing on the sense resistor.
    tmc2099.microsteps(microsteps);
    tmc2099.shaft(flipDir);

    // TMCStepper StallGuard setup
    #ifdef DIAG_PIN
    if (enableSG) {
        pinMode(DIAG_PIN, INPUT);
        tmc2099.semin(4);              // CoolStep/SmartEnergy 4-bit uint that sets lower threshold, 0=disable
        tmc2099.semax(0);              // Refer to p58 of the datasheet
        tmc2099.TCOOLTHRS((3089838.00 * pow(float(maxSpeed), -1.00161534)) * 1.5);  // Lower threshold velocity for switching on CoolStep and StallGuard to DIAG
        tmc2099.SGTHRS(sgThreshold);   // [0..255] the higher the more sensitive to stall
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

    // AS5600 rotary encoder setup
    as5600.begin(SDA_PIN, SCL_PIN);
    assert("Failed to initialize AS5600 rotary encoder." && as5600.isConnected());

    loadSettings();

    while (1) {
        // if (xQueueReceive(motor_command_queue_, (void*) &command, 0) == pdTRUE) {
        //     LOGD("MotorTask received command from motor_command_queue_: %i", command);
        //     // TODO fix consecutive commands while motor is running
        //     // Try updating encod_curr_pos before accepting another command
        //     // if (command != COVER_STOP && is_motor_running_) {
        //     //     stop();
        //     //     vTaskDelay(1000 / portTICK_PERIOD_MS);
        //     // }
        //     switch (command) {
        //         case COVER_STOP:
        //             stop();
        //             break;
        //         case COVER_OPEN:
        //             moveToPercent(0);
        //             break;
        //         case COVER_CLOSE:
        //             moveToPercent(100);
        //             break;
        //         case COVER_SET_MAX:
        //             setMax();
        //             break;
        //         case COVER_SET_MIN:
        //             setMin();
        //             break;
        //         case SYS_RESET:
        //             resetSettings();
        //             break;
        //         case SYS_REBOOT:
        //             ESP.restart();
        //             break;
        //         default:
        //             moveToPercent(command);
        //     }
        // }

        updatePosition();
    }
}


// For StallGuard
#ifdef DIAG_PIN
void MotorTask::stallguardInterrupt() {
    stepper->forceStop();
    LOGE("Motor stalled");
}
#endif


void MotorTask::loadSettings() {
    motor_setting_.begin("local", false);

    encod_curr_pos_ = motor_setting_.getInt("encod_curr_pos_", 0);
    encod_prev_pos_ = encod_curr_pos_;  // Must initialize else would trigger readEncoderPosition()
    as5600.resetCumulativePosition(encod_curr_pos_);  // Set as5600 to previous encoder position

    encod_max_pos_ = motor_setting_.getInt("encod_max_pos_", 455000);

    stepper->setCurrentPosition(positionToSteps(encod_curr_pos_));

    LOGI("Encoder settings loaded(curr/max): %d/%d", encod_curr_pos_, encod_max_pos_);
}


// TODO move setting to it's own class and Interface Task
void MotorTask::resetSettings() {
    motor_setting_.clear();
    ESP.restart();
}


void MotorTask::moveToPercent(int percent) {
    int32_t new_position = (int) round((float) percent * (float) encod_max_pos_ / 100.0);
    if (new_position == encod_curr_pos_) {
        return;
    }

    stepper->setSpeedInHz(maxSpeed);

    if (percent > getPercentage()) {
        tmc2099.rms_current(closingRMS);
    } else {
        tmc2099.rms_current(openingRMS);
    }

    // Motor must move first before is_motor_running_==true, else run will excute first before stepper stops
    stepper->moveTo(positionToSteps(new_position));
    is_motor_running_ = true;

    LOGD("Motor moving(curr/max -> tar): %d/%d -> %d", encod_curr_pos_, encod_max_pos_, new_position);
}


// Returns current rounded position percentage. 0 is open; 100 is closed.
int MotorTask::getPercentage() {
    return (int) round((float) as5600.getCumulativePosition() / (float) encod_max_pos_ * 100);
}


void MotorTask::stop() {
    // is_motor_running_ = false;
    stepper->forceStop();
    encod_curr_pos_ = as5600.getCumulativePosition();
    stepper->setCurrentPosition(encod_curr_pos_);

    if (set_min_) {
        set_min_ = false;
        encod_max_pos_ -= encod_curr_pos_;
        as5600.resetCumulativePosition(0);
        stepper->setCurrentPosition(0);
        motor_setting_.putInt("encod_max_pos_", encod_max_pos_);
        LOGD("Motor new min(curr/max): %d/%d", encod_curr_pos_, encod_max_pos_);
    }

    if (set_max_) {
        set_max_ = false;
        encod_max_pos_ = as5600.getCumulativePosition();
        stepper->setCurrentPosition(positionToSteps(encod_curr_pos_));
        motor_setting_.putInt("encod_max_pos_", encod_max_pos_);
        LOGD("Motor new max(curr/max): %d/%d", encod_curr_pos_, encod_max_pos_);
    }
}


void MotorTask::readEncoderPosition() {
    // encod_prev_pos_ = encod_curr_pos_;
    // encod_curr_pos_ = as5600.getCumulativePosition();
    // stepper->setCurrentPosition(positionToSteps(encod_curr_pos_));
}


int MotorTask::positionToSteps(int encoder_position) {
    return (int) round(encoder_position * motor_encoder_ratio_);
}


void MotorTask::updatePosition() {
    // if (is_motor_running_) return;
    // motor_setting_.putInt("encod_curr_pos_", encod_curr_pos_);
    // LOGD("Motor stopped(curr/max): %d/%d", encod_curr_pos_, encod_max_pos_);

    // Don't send message to wireless task if percentage change is < 3 because it will
    // overflow the wireless message queue. Also, the resolution for the UI graphic most
    // is most likely in 1% increments.
    int current_precentage = getPercentage();
    if (abs(current_precentage - last_updated_percentage_) > 0
            && current_precentage >= 0
            && current_precentage <= 100) {
        last_updated_percentage_ = current_precentage;
        if (xQueueSend(wireless_message_queue_, (void*) &current_precentage, 10) != pdTRUE) {
            LOGE("Failed to send to wireless_message_queue_.");
        }
    }
}


void MotorTask::setMin() {
    LOGD("Motor setting new minimum position");

    set_min_ = true;
    tmc2099.rms_current(openingRMS);
    stepper->setSpeedInHz(maxSpeed / 4);

    stepper->runBackward();
    is_motor_running_ = true;
}


void MotorTask::setMax() {
    LOGD("Motor setting new maximum position");

    set_max_ = true;
    tmc2099.rms_current(closingRMS);
    stepper->setSpeedInHz(maxSpeed / 4);

    stepper->runForward();
    is_motor_running_ = true;
}


void MotorTask::addListener(QueueHandle_t queue) {
    wireless_message_queue_ = queue;
}

QueueHandle_t MotorTask::getMotorCommandQueue() {
    return motor_command_queue_;
}