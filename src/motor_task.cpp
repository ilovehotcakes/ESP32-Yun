#include "motor_task.h"


MotorTask::MotorTask(const uint8_t task_core) : Task{"MotorTask", 8192, 1, task_core, 2} {}
MotorTask::~MotorTask() {}


void MotorTask::addWirelessTaskQueue(QueueHandle_t queue) {
    wireless_task_queue_ = queue;
}


void MotorTask::addSystemSleepTimer(xTimerHandle timer) {
    system_sleep_timer_ = timer;
}


void MotorTask::run() {
    pinMode(DIR_PIN, OUTPUT);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(STBY_PIN, OUTPUT);
    pinMode(DIAG_PIN, INPUT);

    // Using UART to read/write data to/from TMC2209 stepper motor driver
    Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
    while(!Serial1);

    // FastAccelStepper setup
    engine_.init(1);
    motor_ = engine_.stepperConnectToPin(STEP_PIN);
    assert(motor_ && "Failed to initialize FastAccelStepper");
    motor_->setEnablePin(100, false);
    motor_->setExternalEnableCall(std::bind(&MotorTask::motorEnable, this, std::placeholders::_1, std::placeholders::_2));
    motor_->setDirectionPin(DIR_PIN);
    motor_->setSpeedInHz(velocity_);
    motor_->setAcceleration(acceleration_);
    motor_->setAutoEnable(true);     // Automatically enable/disable motor output when moving
    motor_->setDelayToDisable(200);  // 200ms off delay

    driverStandby();

    // AS5600 rotary encoder setup
    encoder_.begin(SDA_PIN, SCL_PIN);
    // assert(encoder_.isConnected() && "Failed to initialize AS5600 rotary encoder");
    Wire.setClock(1000000);         // Increase I2C bus speed to 1MHz which is AS5600's max bus speed
    encoder_.setConfigure(0x2904);  // Hysteresis=1LSB, fast filter=10LSBs, slow filter=8x, WD=ON
    LOGI("Encoder automatic gain control: %d/128", encoder_.readAGC());  // 56-68 is preferable

    loadSettings();

    while (1) {
        encod_pos_ = encoder_.getCumulativePosition();
        motor_->setCurrentPosition(positionToSteps(encod_pos_));

        if (xQueueReceive(queue_, (void*) &inbox_, 0) == pdTRUE) {
            LOGD("Motor task received command: %i, %i", inbox_.command, inbox_.parameter);
            switch (inbox_.command) {
                case COVER_STOP:
                    stop();
                    break;
                case COVER_SET_MAX:
                    setMax();
                    break;
                case COVER_SET_MIN:
                    setMin();
                    break;
                case STBY_ON:
                    driverStandby();
                    break;
                case STBY_OFF:
                    driverStartup();
                    break;
                default:
                    moveToPercent(inbox_.command);
                    break;
            }
        }

        if (stalled_) {
            stop();
            stalled_ = false;
            LOGD("Motor stalled");
        }

        if (motor_->isRunning()) {
            xTimerStart(system_sleep_timer_, 0);
            vTaskDelay(1);
            continue;
        }

        if (last_updated_percent_ == getPercent()) {
            vTaskDelay(1);
            continue;
        }

        // Send new position % if it has changed
        int current_percent = getPercent();
        if (current_percent >= 0 && current_percent <= 100) {
            last_updated_percent_ = current_percent;
            Message new_position(current_percent);
            if (xQueueSend(wireless_task_queue_, (void*) &new_position, 0) != pdTRUE) {
                LOGE("Failed to send to wireless_task queue_");
            }
        }

        vTaskDelay(1);  // Finished all task within loop, handing control back to scheduler
    }
}


// For StallGuard
void IRAM_ATTR MotorTask::stallguardInterrupt() {
    portENTER_CRITICAL(&stalled_mux_);
    stalled_ = true;
    portEXIT_CRITICAL(&stalled_mux_);
}


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

    if (driver_standby_) {
        driverStartup();
        vTaskDelay(5);  // Wait for driver to startup
    }

    motor_->setSpeedInHz(velocity_);

    if (percent > getPercent()) {
        driver_.rms_current(closing_current_);
    } else {
        driver_.rms_current(opening_current_);
    }

    int32_t new_position = static_cast<int>(percent * encod_max_pos_ / 100.0 + 0.5);
    motor_->moveTo(positionToSteps(new_position));

    LOGD("Motor moving(curr/max -> tar): %d/%d -> %d", encod_pos_, encod_max_pos_, new_position);
}


void MotorTask::stop() {
    motor_->forceStop();
    // Without a slight delay, sometimes fastaccelstepper restarts after forceStop() is called.
    // It is uncertain why but it might be due to the constant resetting of fastaccelstepper's
    // position in the while loop. Author of fastaccelstepper recommends for ESP32 to only call
    // setCurrentPosition when motor is in standstill.
    vTaskDelay(2 / portTICK_PERIOD_MS);
    LOGD("Motor stopped(curr/max): %d/%d", encod_pos_, encod_max_pos_);
}


bool MotorTask::setMin() {
    if (encoder_.getCumulativePosition() >= encod_max_pos_) {
        return false;
    }
    encod_max_pos_ -= encoder_.getCumulativePosition();
    motor_settings_.putInt("encod_max_pos_", encod_max_pos_);
    encoder_.resetCumulativePosition(0);
    motor_->setCurrentPosition(0);
    last_updated_percent_ = -100;  // Needed to trigger send message
    LOGD("Motor new min(curr/max): %d/%d", 0, encod_max_pos_);
    return true;  // TODO: check new max_pos_
}


bool MotorTask::setMax() {
    if (encoder_.getCumulativePosition() < 0) {
        return false;
    }
    encod_max_pos_ = encoder_.getCumulativePosition();
    motor_settings_.putInt("encod_max_pos_", encod_max_pos_);
    motor_->setCurrentPosition(positionToSteps(encod_max_pos_));
    last_updated_percent_ = -100;
    LOGD("Motor new max(curr/max): %d/%d", encod_max_pos_, encod_max_pos_);
    return true;  // TODO: check new max_pos_
}


// 0 is open; 100 is closed.
inline int MotorTask::getPercent() {
    return static_cast<int>(static_cast<float>(encod_pos_) / encod_max_pos_ * 100 + 0.5);
}


inline int MotorTask::positionToSteps(int encoder_position) {
    return static_cast<int>(motor_encoder_ratio_ * encoder_position + 0.5);
}


bool MotorTask::motorEnable(uint8_t enable_pin, uint8_t value) {
    if (value == HIGH) {
        driver_.toff(4);
    } else {
        driver_.toff(0);
    }
    return value;
}


void MotorTask::driverStartup() {
    if (!driver_standby_) {
        LOGD("Driver already started");
        return;
    }

    driver_standby_ = false;

    // Pull standby pin low to disable driver standby
    digitalWrite(STBY_PIN, LOW);

    // Sets pdn_disable=1: disables automatic standstill current reduction, needed for UART; also
    // sets mstep_reg_select=1: use UART to change microstepping settings.
    driver_.begin();

    // Use voltage reference from internal 5VOut instead of analog VRef for current scaling
    driver_.I_scale_analog(0);

    // Set motor RMS current via UART, higher torque requires more current. The default holding
    // current (ihold) is 50% of irun but the ratio be adjusted with optional second argument, i.e.
    // rms_current(1000, 0.3).
    driver_.rms_current(closing_current_);

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
    driver_.hstrt(4);
    driver_.hend(12);

    // Inverse motor direction
    driver_.shaft(direction_);

    // StallGuard setup; refer to p29 and p73 of TMC2209's datasheet rev1.09 for tuning SG.
    if (stallguard_enabled_) {
        // 0=disable CoolStep
        // CoolStep lower threshold [0... 15].
        // If SG_RESULT goes below this threshold, CoolStep increases the current to both coils.
        driver_.semin(4);

        // CoolStep upper threshold [0... 15].
        // If SG is sampled equal to or above this threshold enough times, CoolStep decreases the
        // current to both coils.
        driver_.semax(0);

        // Lower threshold velocity for switching on CoolStep and StallGuard to DIAG output
        driver_.TCOOLTHRS(3089838.00 * pow(float(velocity_), -1.00161534));

        // StallGuard threshold [0... 255] level for stall detection. It compensates for motor
        // specific characteristics and controls sensitivity. A higher value makes StallGuard more
        // sensitive and requires less torque to stall. The double of this value is compared to
        // SG_RESULT. The stall output becomes active if SG_RESULT fall below this value.
        driver_.SGTHRS(stallguard_threshold_);

        // Enable StallGuard or else it will stall the motor when starting the driver
        attachInterrupt(DIAG_PIN, std::bind(&MotorTask::stallguardInterrupt, this), RISING);
    }

    LOGI("Driver has started");
}


void MotorTask::driverStandby() {
    if (driver_standby_) {
        LOGD("Driver already in standby");
        return;
    } else if (motor_->isRunning()) {
        LOGD("Driver can't be put in standby while motor is running");
        return;
    }

    driver_standby_ = true;

    // Need to disable StallGuard or else it will stall the motor when disabling the driver
    detachInterrupt(DIAG_PIN);

    // Pull standby pin high to standby TMC2209 driver
    digitalWrite(STBY_PIN, HIGH);
    LOGI("Driver in standby");
}