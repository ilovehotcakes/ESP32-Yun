#include "motor_task.h"


MotorTask::MotorTask(const uint8_t task_core) : Task{"Motor", 8192, 1, task_core} {
    motor_command_queue_ = xQueueCreate(1, sizeof(int));
    assert(motor_command_queue_ != NULL);
}


MotorTask::~MotorTask() {
    vQueueDelete(motor_command_queue_);
}


void MotorTask::run() {
    pinMode(DIR_PIN, OUTPUT);
    pinMode(STEP_PIN, OUTPUT);

    // TMC2209 stepper motor driver setup using UART mode + STEP/DIR
    // TMC2209's UART interface automatically becomes enabled when correct UART data is sent. It
    // automatically adapts to uC's baud rate. Block until UART is finished initializing so ESP32
    // can send settings to the driver via UART.
    SERIAL_PORT.begin(115200, SERIAL_8N1, RXD1, TXD1);
    while(!SERIAL_PORT);

    // Sets pdn_disable=1: disables automatic standstill current reduction, needed for UART; also
    // sets mstep_reg_select=1: use UART to change microstepping settings.
    driver.begin();

    // 0=disable driver; 1-15=enable driver in StealthChop
    // Sets the slow decay time (off time) [1... 15]. This setting also limit the maximum chopper
    // frequency. For operation with StealthChop, this parameter is not used, but it is required to
    // enable the motor. In case of operation with StealthChop only, any setting is OK.
    driver.toff(0);

    // Set motor RMS current via UART, higher torque requires more current. The default holding
    // current (ihold) is 50% of irun but the ratio be adjusted with optional second argument, i.e.
    // rms_current(1000, 0.3).
    driver.rms_current(openingRMS);

    // Needed for StealthChop instead of manually setting PWM scaling factor
    driver.pwm_autoscale(true);

    // true=enable SpreadCycle
    // SpreadCycle for high velocity but is audible; StealthChop is quiet and more torque. They can
    // be used together by setting a threshold when it switches from StealthChop to SpreadCycle.
    driver.en_spreadCycle(false);

    // Number of microsteps [0, 2, 4, 8, 16, 32, 64, 126, 256] per full step
    // Set MRES register via UART
    driver.microsteps(16);

    // Comparator blank time to [16, 24, 32, 40] clocks. The time needed to safely cover switching
    // events and the duration of ringing on sense resistor. For most applications, a setting of 16
    // or 24 is good. For highly capacitive loads, a setting of 32 or 40 will be required.
    driver.blank_time(24);
    driver.shaft(flipDir);

    // StallGuard setup
    #ifdef DIAG_PIN
    if (enableSG) {
        pinMode(DIAG_PIN, INPUT);

        // 0=disable CoolStep
        // CoolStep lower threshold [0... 15].
        // If SG_RESULT goes below this threshold, CoolStep increases the current to both coils.
        driver.semin(4);

        // CoolStep upper threshold [0... 15].
        // If SG is sampled equal to or above this threshold enough times, CoolStep decreases the
        // current to both coils.
        driver.semax(0);

        // Lower threshold velocity for switching on CoolStep and StallGuard to DIAG output
        driver.TCOOLTHRS((3089838.00 * pow(float(maxSpeed), -1.00161534)) * 1.5);

        // StallGuard threshold [0... 255] level for stall detection. It compensates for motor
        // specific characteristics and controls sensitivity. A higher value makes StallGuard more
        // sensitive and requires less torque to stall. The double of this value is compared to
        // SG_RESULT. The stall output becomes active if SG_RESULT fall below this value.
        driver.SGTHRS(sgThreshold);
        attachInterrupt(DIAG_PIN, std::bind(&MotorTask::stallguardInterrupt, this), RISING);
    }
    #endif

    // FastAccelStepper setup
    engine.init();
    motor = engine.stepperConnectToPin(STEP_PIN);
    assert("Failed to initialize FastAccelStepper" && motor);
    motor->setEnablePin(100, false);
    motor->setExternalEnableCall(std::bind(&MotorTask::enableDriver, this, std::placeholders::_1, std::placeholders::_2));
    motor->setDirectionPin(DIR_PIN);
    motor->setSpeedInHz(maxSpeed);
    motor->setAcceleration(acceleration);
    motor->setAutoEnable(true);     // Automatically enable motor output when moving and vice versa
    motor->setDelayToDisable(200);  // 200ms off delay

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
void IRAM_ATTR MotorTask::stallguardInterrupt() {
    motor->forceStop();
    vTaskDelay(1 / portTICK_PERIOD_MS);  // Added delay for motor to fully stop
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
        driver.rms_current(closingRMS);
    } else {
        driver.rms_current(openingRMS);
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


bool MotorTask::enableDriver(uint8_t enable_pin, uint8_t value) {
    if (value == HIGH) {
        driver.toff(4);
    } else {
        driver.toff(0);
    }
    return value;
}