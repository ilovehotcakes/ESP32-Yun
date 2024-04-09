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
    pinMode(STBY_PIN, OUTPUT);
    digitalWrite(STBY_PIN, LOW);
    if (stallguard_enable_) {
        pinMode(DIAG_PIN, INPUT);
        attachInterrupt(DIAG_PIN, std::bind(&MotorTask::stallguardInterrupt, this), RISING);
    }

    // TMC2209 stepper motor driver setup using UART mode + STEP/DIR
    // For quick configuration guide, please refer to p70-72 of TMC2209's datasheet rev1.09
    // TMC2209's UART interface automatically becomes enabled when correct UART data is sent. It
    // automatically adapts to uC's baud rate. Block until UART is finished initializing so ESP32
    // can send settings to the driver via UART.
    SERIAL_PORT.begin(115200, SERIAL_8N1, RXD1, TXD1);
    while(!SERIAL_PORT);

    driverStartup();

    // FastAccelStepper setup
    engine_.init(1);
    motor_ = engine_.stepperConnectToPin(STEP_PIN);
    assert("Failed to initialize FastAccelStepper" && motor_);
    motor_->setEnablePin(100, false);
    motor_->setExternalEnableCall(std::bind(&MotorTask::driverEnable, this, std::placeholders::_1, std::placeholders::_2));
    motor_->setDirectionPin(DIR_PIN);
    motor_->setSpeedInHz(velocity_);
    motor_->setAcceleration(acceleration_);
    motor_->setAutoEnable(true);     // Automatically enable motor output when moving and vice versa
    motor_->setDelayToDisable(200);  // 200ms off delay

    // AS5600 rotary encoder setup
    encoder_.begin(SDA_PIN, SCL_PIN);
    // assert("Failed to initialize AS5600 rotary encoder" && encoder_.isConnected());
    encoder_.setWatchDog(1);    // Enable automatic low power (sleep) mode 6.5mA -> 1.5mA
    encoder_.setHysteresis(3);  // Reduce sensitivity when in sleep mode
    encoder_.setSlowFilter(0);  // Reduce noise especially when stopping
    encoder_.setFastFilter(7);

    loadSettings();

    while (1) {
        motor_->setCurrentPosition(positionToSteps(encoder_.getCumulativePosition()));

        if (xQueueReceive(motor_command_queue_, (void*) &command_, 0) == pdTRUE) {
            switch (command_) {
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
                case STBY_ON:
                    digitalWrite(STBY_PIN, HIGH);
                    break;
                case STBY_OFF:
                    driverStartup();
                    break;
                default:
                    moveToPercent(command_);
                    break;
            }
        }

        if (stalled_) {
            stop();
            stalled_ = false;
        } else if (motor_->isRunning()) {
            continue;
        } else if (getPercent() == last_updated_percent_) {
            continue;
        }

        // Don't send message to wireless task if percent change is less than 2 to reduce
        // wireless queue overhead.
        int current_percent = getPercent();
        if (current_percent >= 0 && current_percent <= 100) {
            last_updated_percent_ = current_percent;
            if (xQueueSend(wireless_message_queue_, (void*) &current_percent, 0) != pdTRUE) {
                LOGE("Failed to send to wireless_message_queue_");
            }
        }
    }
}


// For StallGuard
#ifdef DIAG_PIN
void IRAM_ATTR MotorTask::stallguardInterrupt() {
    portENTER_CRITICAL(&stalled_mux_);
    stalled_ = true;
    portEXIT_CRITICAL(&stalled_mux_);
}
#endif


void MotorTask::loadSettings() {
    motor_settings_.begin("local", false);

    encod_max_pos_ = motor_settings_.getInt("encod_max_pos_", 4096 * 20);
    int32_t encod_curr_pos = motor_settings_.getInt("encod_curr_pos_", 0);
    encoder_.resetCumulativePosition(encod_curr_pos);  // Set encoder to previous position
    motor_->setCurrentPosition(positionToSteps(encod_curr_pos));  // Set motor to previous position

    LOGI("Encoder settings loaded(curr/max): %d/%d", encod_curr_pos, encod_max_pos_);
}


void MotorTask::moveToPercent(int percent) {
    if (motor_->isRunning()) {
        stop();
        return;
    }

    if (percent == getPercent()) {
        return;
    }

    motor_->setSpeedInHz(velocity_);

    if (percent > getPercent()) {
        driver_.rms_current(closing_current_);
    } else {
        driver_.rms_current(opening_current_);
    }

    int32_t new_position = static_cast<int>(percent * encod_max_pos_ / 100.0 + 0.5);
    motor_->moveTo(positionToSteps(new_position));

    LOGD("Motor moving(curr/max -> tar): %d/%d -> %d", encoder_.getCumulativePosition(), encod_max_pos_, new_position);
}


void MotorTask::stop() {
    motor_->forceStop();
    // Without a slight delay, sometimes fastaccelstepper restarts after forceStop() is called.
    // It is uncertain why but it might be due to the constant resetting of fastaccelstepper's
    // position in the while loop. Author of fastaccelstepper recommends for ESP32 to only call
    // setCurrentPosition when motor is in standstill.
    vTaskDelay(2 / portTICK_PERIOD_MS);
    LOGD("Motor stopped(curr/max): %d/%d", encoder_.getCumulativePosition(), encod_max_pos_);
}


void MotorTask::setMin() {
    if (encoder_.getCumulativePosition() >= encod_max_pos_) {
        return;
    }
    encod_max_pos_ -= encoder_.getCumulativePosition();
    motor_settings_.putInt("encod_max_pos_", encod_max_pos_);
    encoder_.resetCumulativePosition(0);
    motor_->setCurrentPosition(0);
    last_updated_percent_ = -100;  // Needed to trigger send message
    LOGD("Motor new min(curr/max): %d/%d", 0, encod_max_pos_);
}


void MotorTask::setMax() {
    if (encoder_.getCumulativePosition() < 0) {
        return;
    }
    encod_max_pos_ = encoder_.getCumulativePosition();
    motor_settings_.putInt("encod_max_pos_", encod_max_pos_);
    motor_->setCurrentPosition(positionToSteps(encod_max_pos_));
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
    return static_cast<int>(static_cast<float>(encoder_.getCumulativePosition()) / encod_max_pos_ * 100 + 0.5);
}


inline int MotorTask::positionToSteps(int encoder_position) {
    return static_cast<int>(motor_encoder_ratio_ * encoder_position + 0.5);
}


bool MotorTask::driverEnable(uint8_t enable_pin, uint8_t value) {
    if (value == HIGH) {
        driver_.toff(4);
    } else {
        driver_.toff(0);
    }
    return value;
}


void MotorTask::driverStartup() {
    digitalWrite(STBY_PIN, LOW);

    // Sets pdn_disable=1: disables automatic standstill current reduction, needed for UART; also
    // sets mstep_reg_select=1: use UART to change microstepping settings.
    driver_.begin();

    // Use voltage reference from internal 5VOut instead of analog Vref for current scaling
    driver_.I_scale_analog(0);

    // Set motor RMS current via UART, higher torque requires more current. The default holding
    // current (ihold) is 50% of irun but the ratio be adjusted with optional second argument, i.e.
    // rms_current(1000, 0.3).
    driver_.rms_current(opening_current_);

    // 1=SpreadCycle only; 0=StealthChop PWM mode (below velocity threshold) + SpreadCycle (above
    // velocity threshold); set register TPWMTHRS to determine the velocity threshold
    // SpreadCycle for high velocity but is audible; StealthChop is quiet and more torque.
    driver_.en_spreadCycle(false);
    driver_.TPWMTHRS(33);  // Based on 9V, 200mA

    // Enable StealthChop voltage PWM mode: automatic scaling current control taking into account
    // of the motor back EMF and velocity.
    driver_.pwm_autoscale(true);
    driver_.pwm_autograd(true);

    // Number of microsteps [0, 2, 4, 8, 16, 32, 64, 126, 256] per full step
    // Set MRES register via UART
    driver_.microsteps(microsteps_);

    // 0=disable driver; 1-15=enable driver in StealthChop
    // Sets the slow decay time (off time) [1... 15]. This setting also limit the maximum chopper
    // frequency. For operation with StealthChop, this parameter is not used, but it is required to
    // enable the motor. In case of operation with StealthChop only, any setting is OK.
    driver_.toff(0);

    // Comparator blank time to [16, 24, 32, 40] clocks. The time needed to safely cover switching
    // events and the duration of ringing on sense resistor. For most applications, a setting of 16
    // or 24 is good. For highly capacitive loads, a setting of 32 or 40 will be required.
    driver_.blank_time(24);
    // driver_.hstrt(4);
    // driver_.hend(12);

    // Inverse motor direction
    driver_.shaft(direction_);

    // StallGuard setup; refer to p73 of TMC2209's datasheet rev1.09 for tuning SG.
    if (stallguard_enable_) {
        // 0=disable CoolStep
        // CoolStep lower threshold [0... 15].
        // If SG_RESULT goes below this threshold, CoolStep increases the current to both coils.
        driver_.semin(4);

        // CoolStep upper threshold [0... 15].
        // If SG is sampled equal to or above this threshold enough times, CoolStep decreases the
        // current to both coils.
        driver_.semax(0);

        // Lower threshold velocity for switching on CoolStep and StallGuard to DIAG output
        driver_.TCOOLTHRS((3089838.00 * pow(float(velocity_), -1.00161534)));

        // StallGuard threshold [0... 255] level for stall detection. It compensates for motor
        // specific characteristics and controls sensitivity. A higher value makes StallGuard more
        // sensitive and requires less torque to stall. The double of this value is compared to
        // SG_RESULT. The stall output becomes active if SG_RESULT fall below this value.
        driver_.SGTHRS(stallguard_threshold_);
    }
}