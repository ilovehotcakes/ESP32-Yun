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
    motor = engine.stepperConnectToPin(STEP_PIN);
    assert("Failed to initialize FastAccelStepper" && motor);
    motor->setEnablePin(EN_PIN);
    motor->setDirectionPin(DIR_PIN);
    motor->setSpeedInHz(maxSpeed);
    motor->setAcceleration(acceleration);
    motor->setAutoEnable(true);
    motor->setDelayToDisable(200);

    // AS5600 rotary encoder setup
    encoder.begin(SDA_PIN, SCL_PIN);
    assert("Failed to initialize AS5600 rotary encoder" && encoder.isConnected());

    loadSettings();

    while (1) {
        motor->setCurrentPosition(positionToSteps(encoder.getCumulativePosition()));

        if (xQueueReceive(motor_command_queue_, (void*) &command, 0) == pdTRUE) {
            switch (command) {
                case COVER_STOP:
                    break;
                case COVER_OPEN:
                    moveToPercent(0);
                    break;
                case COVER_CLOSE:
                    moveToPercent(100);
                    break;
                case COVER_SET_MAX:
                    setMax();
                    break;
                case COVER_SET_MIN:
                    setMin();
                    break;
                // case SYS_RESET:
                //     motor_settings_.clear();
                //     ESP.restart();
                //     break;
                // case SYS_REBOOT:
                //     ESP.restart();
                //     break;
                default:
                    moveToPercent(command);
            }
        }

        if (!motor->isRunning()) {
            // Don't send message to wireless task if percent change is less than 2 to reduce
            // wireless queue overhead.
            int current_percent = getPercent();
            if (abs(current_percent - last_updated_percent_) > 1
                    && current_percent >= 0
                    && current_percent <= 100) {
                last_updated_percent_ = current_percent;
                if (xQueueSend(wireless_message_queue_, (void*) &current_percent, 10) != pdTRUE) {
                    LOGE("Failed to send to wireless_message_queue_");
                }
            }
        }
    }
}


// For StallGuard
#ifdef DIAG_PIN
void MotorTask::stallguardInterrupt() {
    motor->forceStop();
    LOGE("Motor stalled");
}
#endif


void MotorTask::loadSettings() {
    motor_settings_.begin("local", false);

    // encod_max_pos_ = motor_settings_.getInt("encod_max_pos_", 455000);
    encod_max_pos_ = motor_settings_.getInt("encod_max_pos_", 4096 * 10);
    int32_t encod_curr_pos = motor_settings_.getInt("encod_curr_pos_", 0);
    encoder.resetCumulativePosition(encod_curr_pos);  // Set as5600 to previous encoder position
    motor->setCurrentPosition(positionToSteps(encod_curr_pos));

    LOGI("Encoder settings loaded(curr/max): %d/%d", encod_curr_pos, encod_max_pos_);
}


void MotorTask::moveToPercent(int percent) {
    if (motor->isRunning()) {
        stop();
        return;
    }

    int32_t new_position = static_cast<int>(percent * encod_max_pos_ / 100 + 0.5);
    if (abs(new_position - encoder.getCumulativePosition()) < 100) {
        return;
    }

    motor->setSpeedInHz(maxSpeed);

    if (percent > getPercent()) {
        tmc2099.rms_current(closingRMS);
    } else {
        tmc2099.rms_current(openingRMS);
    }

    motor->moveTo(positionToSteps(new_position));

    LOGD("Motor moving(curr/max -> tar): %d/%d -> %d", encoder.getCumulativePosition(), encod_max_pos_, new_position);
}


void MotorTask::stop() {
    motor->forceStop();
    LOGD("Motor stopped(curr/max): %d/%d", encoder.getCumulativePosition(), encod_max_pos_);
}


void MotorTask::setMin() {
    encod_max_pos_ -= encoder.getCumulativePosition();
    motor_settings_.putInt("encod_max_pos_", encod_max_pos_);
    encoder.resetCumulativePosition(0);
    motor->setCurrentPosition(0);
    last_updated_percent_ = -100;  // Needed to trigger send message
    LOGD("Motor new min(curr/max): %d/%d", 0, encod_max_pos_);
}


void MotorTask::setMax() {
    encod_max_pos_ = encoder.getCumulativePosition();
    motor_settings_.putInt("encod_max_pos_", encod_max_pos_);
    motor->setCurrentPosition(positionToSteps(encod_max_pos_));
    last_updated_percent_ = -100;
    LOGD("Motor new max(curr/max): %d/%d", encod_max_pos_, encod_max_pos_);
}


void MotorTask::addListener(QueueHandle_t queue) {
    wireless_message_queue_ = queue;
}

QueueHandle_t MotorTask::getMotorCommandQueue() {
    return motor_command_queue_;
}


// 0 is open; 100 is closed.
inline int MotorTask::getPercent() {
    return static_cast<int>(static_cast<float>(encoder.getCumulativePosition()) / encod_max_pos_ * 100 + 0.5);
}


inline int MotorTask::positionToSteps(int encoder_position) {
    return static_cast<int>(motor_encoder_ratio_ * encoder_position + 0.5);
}
