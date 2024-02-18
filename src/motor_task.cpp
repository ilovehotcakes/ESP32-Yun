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
    pinMode(ENN_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(STEP_PIN, OUTPUT);

    // Initialize HardwareSerial; RX: 16, RX: TXD2
    // TMC2209's uart interface automatically becomes enabeld when correct uart data is sent. It
    // automatically adapts to ESP32's baud rate.
    Serial1.begin(115200, SERIAL_8N1, 16, TXD2);

    // Automatcially sets pdn_disable to 1: disables automatic standstill current reduction, needed
    // for uart; also sets mstep_reg_select to 1: use uart to set microsteps
    tmc2209.begin();
    tmc2209.toff(4);
    tmc2209.rms_current(openingRMS); // Motor RMS current "rms_current will by default set ihold to 50% of irun but you can set your own ratio with additional second argument; rms_current(1000, 0.3)."
    tmc2209.pwm_autoscale(true);     // Needed for StealthChop, instead of manually setting
    tmc2209.en_spreadCycle(false);   // Disable SpreadCycle; SpreadCycle is faster but louder
    tmc2209.blank_time(24);          // Comparator blank time. Needed to safely cover the switching event and the duration of the ringing on the sense resistor.
    tmc2209.microsteps(microsteps);
    tmc2209.shaft(flipDir);

    // TMCStepper StallGuard setup
    #ifdef DIAG_PIN
    if (enableSG) {
        pinMode(DIAG_PIN, INPUT);
        tmc2209.semin(4);            // CoolStep/SmartEnergy 4-bit uint that sets lower threshold, 0=disable
        tmc2209.semax(0);            // Refer to p58 of the datasheet
        tmc2209.TCOOLTHRS((3089838.00 * pow(float(maxSpeed), -1.00161534)) * 1.5);  // Lower threshold velocity for switching on CoolStep and StallGuard to DIAG
        tmc2209.SGTHRS(sgThreshold); // [0..255] the higher the more sensitive to stall
        attachInterrupt(DIAG_PIN, std::bind(&MotorTask::stallguardInterrupt, this), RISING);
    }
    #endif

    // FastAccelStepper setup
    engine.init();
    motor = engine.stepperConnectToPin(STEP_PIN);
    assert("Failed to initialize FastAccelStepper" && motor);
    motor->setEnablePin(ENN_PIN);
    motor->setDirectionPin(DIR_PIN);
    motor->setSpeedInHz(maxSpeed);
    motor->setAcceleration(acceleration);
    motor->setAutoEnable(true);
    motor->setDelayToDisable(200);

    // AS5600 rotary encoder setup
    encoder.begin(SDA_PIN, SCL_PIN);
    encoder.setWatchDog(1);    // Enable automatic low power (sleep) mode 6.5mA -> 1.5mA
    encoder.setHysteresis(3);  // Reduce sensitivity when in sleep mode
    encoder.setSlowFilter(0);  // Reduce noise especially when stopping
    encoder.setFastFilter(7);
    assert("Failed to initialize AS5600 rotary encoder" && encoder.isConnected());

    loadSettings();

    while (1) {
        motor->setCurrentPosition(positionToSteps(encoder.getCumulativePosition()));

        if (xQueueReceive(motor_command_queue_, (void*) &command, 0) == pdTRUE) {
            switch (command) {
                case COVER_STOP:
                    stop();
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
                case SYS_RESET:
                    motor_settings_.clear();
                    ESP.restart();
                    break;
                case SYS_REBOOT:
                    ESP.restart();
                    break;
                default:
                    moveToPercent(command);
                    break;
            }
        }

        if (motor->isRunning()) {
            continue;
        }

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


// For StallGuard
#ifdef DIAG_PIN
void MotorTask::stallguardInterrupt() {
    motor->forceStop();
    LOGE("Motor stalled");
}
#endif


void MotorTask::loadSettings() {
    motor_settings_.begin("local", false);

    encod_max_pos_ = motor_settings_.getInt("encod_max_pos_", 4096 * 20);
    int32_t encod_curr_pos = motor_settings_.getInt("encod_curr_pos_", 0);
    encoder.resetCumulativePosition(encod_curr_pos);  // Set encoder to previous position
    motor->setCurrentPosition(positionToSteps(encod_curr_pos));  // Set motor to previous position

    LOGI("Encoder settings loaded(curr/max): %d/%d", encod_curr_pos, encod_max_pos_);
}


void MotorTask::moveToPercent(int percent) {
    if (motor->isRunning()) {
        stop();
        return;
    }

    if (abs(percent - getPercent()) < 2) {
        return;
    }

    motor->setSpeedInHz(maxSpeed);

    if (percent > getPercent()) {
        tmc2209.rms_current(closingRMS);
    } else {
        tmc2209.rms_current(openingRMS);
    }

    int32_t new_position = static_cast<int>(percent * encod_max_pos_ / 100 + 0.5);
    motor->moveTo(positionToSteps(new_position));

    LOGD("Motor moving(curr/max -> tar): %d/%d -> %d", encoder.getCumulativePosition(), encod_max_pos_, new_position);
}


void MotorTask::stop() {
    motor->forceStop();
    // Without a slight delay, sometimes fastaccelstepper restarts after forceStop() is called.
    // It is uncertain why but it might be due to the constant resetting of fastaccelstepper's
    // position in the while loop. Author of fastaccelstepper recommends for ESP32 to only call
    // setCurrentPosition when motor is in standstill.
    vTaskDelay(2 / portTICK_PERIOD_MS);
    LOGD("Motor stopped(curr/max): %d/%d", encoder.getCumulativePosition(), encod_max_pos_);
}


void MotorTask::setMin() {
    if (encoder.getCumulativePosition() >= encod_max_pos_) {
        return;
    }
    encod_max_pos_ -= encoder.getCumulativePosition();
    motor_settings_.putInt("encod_max_pos_", encod_max_pos_);
    encoder.resetCumulativePosition(0);
    motor->setCurrentPosition(0);
    last_updated_percent_ = -100;  // Needed to trigger send message
    LOGD("Motor new min(curr/max): %d/%d", 0, encod_max_pos_);
}


void MotorTask::setMax() {
    if (encoder.getCumulativePosition() < 0) {
        return;
    }
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
