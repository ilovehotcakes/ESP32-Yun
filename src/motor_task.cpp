#include "motor_task.h"


MotorTask::MotorTask(const uint8_t task_core) : Task{"MotorTask", 8192, 1, task_core} {}
MotorTask::~MotorTask() {}


void MotorTask::run() {
    pinMode(DIR_PIN, OUTPUT);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(STBY_PIN, OUTPUT);
    pinMode(DIAG_PIN, INPUT);

    // Using UART(Serial1) to read/write data to/from TMC2209 stepper motor driver
    Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
    while(!Serial1);

    // FastAccelStepper setup
    engine_.init(1);
    motor_ = engine_.stepperConnectToPin(STEP_PIN);
    assert(motor_ != NULL);
    motor_->setEnablePin(100, false);
    motor_->setExternalEnableCall(std::bind(&MotorTask::motorEnable, this,
                                  std::placeholders::_1, std::placeholders::_2));
    motor_->setDirectionPin(DIR_PIN);
    motor_->setAutoEnable(true);     // Automatically enable/disable motor output when moving
    motor_->setDelayToDisable(200);  // 200ms off delay

    driverStandby();

    // AS5600 rotary encoder setup
    encoder_.begin(SDA_PIN, SCL_PIN);
    // assert(encoder_.isConnected());
    Wire.setClock(1000000);         // Increase I2C bus speed to 1MHz which is AS5600's max bus speed
    encoder_.setConfigure(0x2904);  // Hysteresis=1LSB, fast filter=10LSBs, slow filter=8x, WD=ON
    LOGI("Encoder automatic gain control(56-68 is preferable): %d/128", encoder_.readAGC());

    loadSettings();
    // int start = 0;
    while (1) {
        // start = micros();
        encod_pos_ = encoder_.getCumulativePosition() - encod_offset_;
        motor_->setCurrentPosition(positionToStep(encod_pos_));

        if (xQueueReceive(queue_, (void*) &inbox_, 0) == pdTRUE) {
            LOGI("MotorTask received message: %s", inbox_.toString().c_str());
            switch (inbox_.command) {
                case MOTOR_STOP:
                    stop();
                    break;
                case MOTOR_PERECENT:
                    moveToPercent(inbox_.parameter);
                    break;
                case MOTOR_STEP:
                    moveToStep(inbox_.parameter);
                    break;
                case MOTOR_FORWARD:
                    move(false);
                    break;
                case MOTOR_BACKWARD:
                    move(true);
                    break;
                case MOTOR_SET_MIN:
                    setMin();
                    break;
                case MOTOR_SET_MAX:
                    setMax();
                    break;
                case MOTOR_ZERO:
                    zeroEncoder();
                    break;
                case MOTOR_STANDBY:
                    if (inbox_.parameter == 1) driverStandby();
                    else driverStartup();
                    break;
                case MOTOR_SYNC_STTNG:
                    setAndSave(sync_settings_, static_cast<bool>(inbox_.parameter), "sync_settings_");
                    break;
                case MOTOR_VLCTY:
                    setAndSave(open_velocity_, inbox_.parameterf, "open_velocity_");
                    setAndSave(clos_velocity_, inbox_.parameterf, "clos_velocity_");
                    break;
                case MOTOR_OP_VLCTY:
                    setAndSave(open_velocity_, inbox_.parameterf, "open_velocity_");
                    break;
                case MOTOR_CL_VLCTY:
                    setAndSave(clos_velocity_, inbox_.parameterf, "clos_velocity_");
                    break;
                case MOTOR_ACCEL:
                    setAndSave(open_accel_, inbox_.parameterf, "open_accel_");
                    setAndSave(clos_accel_, inbox_.parameterf, "clos_accel_");
                    break;
                case MOTOR_OP_ACCEL:
                    setAndSave(open_accel_, inbox_.parameterf, "open_accel_");
                    break;
                case MOTOR_CL_ACCEL:
                    setAndSave(clos_accel_, inbox_.parameterf, "clos_accel_");
                    break;
                case MOTOR_CURRENT:
                    setAndSave(open_current_, inbox_.parameter, "open_current_");
                    setAndSave(clos_current_, inbox_.parameter, "clos_current_");
                    break;
                case MOTOR_OP_CURRENT:
                    setAndSave(open_current_, inbox_.parameter, "open_current_");
                    break;
                case MOTOR_CL_CURRENT:
                    setAndSave(clos_current_, inbox_.parameter, "clos_current_");
                    break;
                case MOTOR_DIRECTION:
                    setAndSave(direction_, static_cast<bool>(inbox_.parameter), "direction_");
                    break;
                case MOTOR_FULL_STEPS:
                    setAndSave(full_steps_, inbox_.parameter, "full_steps_");
                    calculateTotalSteps();
                    break;
                case MOTOR_MICROSTEPS:
                    setAndSave(microsteps_, inbox_.parameter, "microsteps_");
                    calculateTotalSteps();
                    break;
                case MOTOR_STALLGUARD:
                    setAndSave(stallguard_en_, static_cast<bool>(inbox_.parameter), "stallguard_en_");
                    break;
                case MOTOR_TCOOLTHRS:
                    setAndSave(coolstep_thrs_, inbox_.parameter, "coolstep_thrs_");
                    break;
                case MOTOR_SGTHRS:
                    setAndSave(stallguard_th_, inbox_.parameter, "stallguard_th_");
                    break;
                case MOTOR_SPREADCYCL:
                    setAndSave(spreadcycl_en_, static_cast<bool>(inbox_.parameter), "spreadcycl_en_");
                    break;
                case MOTOR_TPWMTHRS:
                    setAndSave(spreadcycl_th_, inbox_.parameter, "spreadcycl_th_");
                    break;
            }
        }

        if (stalled_) {
            stop();
            stalled_ = false;
            LOGE("Motor stalled");
        }

        if (motor_->isRunning()) {
            xTimerStart(system_sleep_timer_, 0);
            continue;
        }

        if (last_updated_percent_ == getPercent()) {
            // Serial.println(micros() - start);
            continue;
        }

        // Send new position % if it has changed
        int current_percent = getPercent();
        if (current_percent >= 0 && current_percent <= 100) {
            last_updated_percent_ = current_percent;
            sendTo(wireless_task_, Message(UPDATE_POSITION, current_percent), 0);
        }
    }
}


void IRAM_ATTR MotorTask::stallguardInterrupt() {
    portENTER_CRITICAL(&stalled_mux_);
    stalled_ = true;
    portEXIT_CRITICAL(&stalled_mux_);
}


void MotorTask::loadSettings() {
    bool load = readFromDisk();

    sync_settings_  = getOrDefault("sync_settings_", sync_settings_);
    open_velocity_  = getOrDefault("open_velocity_", open_velocity_);
    clos_velocity_  = getOrDefault("clos_velocity_", clos_velocity_);
    open_accel_     = getOrDefault("open_accel_", open_accel_);
    clos_accel_     = getOrDefault("clos_accel_", clos_accel_);
    open_current_   = getOrDefault("open_current_", open_current_);
    clos_current_   = getOrDefault("clos_current_",clos_current_);
    direction_      = getOrDefault("direction_", direction_);
    microsteps_     = getOrDefault("microsteps_", microsteps_);
    full_steps_     = getOrDefault("full_steps_", full_steps_);
    stallguard_en_  = getOrDefault("stallguard_en_", stallguard_en_);
    // coolstep_thrs_  = getOrDefault("stallguard_cs_", stallguard_cs_);
    stallguard_th_  = getOrDefault("stallguard_th_", stallguard_th_);
    spreadcycl_en_  = getOrDefault("spreadcycl_en_", spreadcycl_en_);
    spreadcycl_th_  = getOrDefault("spreadcycl_th_", spreadcycl_th_);

    encod_max_pos_  = getOrDefault("encod_max_pos_", encod_max_pos_);
    zeroEncoder();
    calculateTotalSteps();

    if (!load) {
        writeToDisk();
    }

    LOGI("Motor settings loaded(curr/max): %d/%d", encod_pos_, encod_max_pos_);
}


bool MotorTask::prepareToMove(bool check, bool direction) {
    if (motor_->isRunning()) {
        stop();
        return false;
    }

    if (check) {
        return false;
    }

    while (driver_stdby_) {
        driverStartup();
    }

    if (direction && !sync_settings_) {
        updateMotorSettings(open_velocity_, open_accel_, open_current_);
    } else {
        updateMotorSettings(clos_velocity_, clos_accel_, clos_current_);
    }

    return true;
}


void MotorTask::move(bool direction) {
    if (!prepareToMove(false, direction)) {
        return;
    }
    if (direction) {
        motor_->runBackward();
        LOGI("Motor running backward");
    } else {
        motor_->runForward();
        LOGI("Motor running forward");
    }
}


void MotorTask::moveToStep(int target_step) {
    int current_step = positionToStep(encod_pos_);
    if (!prepareToMove(target_step == current_step, target_step < current_step)) {
        return;
    }
    motor_->moveTo(target_step);
}


void MotorTask::moveToPercent(int target_percent) {
    if (!prepareToMove(target_percent == getPercent(), target_percent < getPercent())) {
        return;
    }
    int32_t new_position = static_cast<int32_t>(target_percent * encod_max_pos_ / 100.0 + 0.5);
    motor_->moveTo(positionToStep(new_position));
    LOGI("Motor moving(curr/max -> tar): %d/%d -> %d", encod_pos_, encod_max_pos_, new_position);
}


void MotorTask::stop() {
    motor_->forceStop();
    vTaskDelay(2 / portTICK_PERIOD_MS);
    LOGI("Motor stopped(curr/max): %d/%d", encod_pos_, encod_max_pos_);
}


bool MotorTask::setMin() {
    if (encod_pos_ >= encod_max_pos_ || motor_->isRunning()) {
        return false;
    }
    setAndSave(encod_max_pos_, encod_max_pos_ - encod_pos_, "encod_max_pos_");
    zeroEncoder();
    LOGI("Motor new min(curr/max): %d/%d", 0, encod_max_pos_);
    return true;
}


bool MotorTask::setMax() {
    if (encod_pos_ <= 0 || motor_->isRunning()) {
        return false;
    }
    setAndSave(encod_max_pos_, encod_pos_, "encod_max_pos_");
    LOGI("Motor new max(curr/max): %d/%d", encod_pos_, encod_max_pos_);
    return true;
}


bool MotorTask::zeroEncoder() {
    if (motor_->isRunning()) {
        LOGI("Encoder can't be zeroed while motor is running");
        return false;
    }
    encoder_.resetCumulativePosition(0);
    encod_offset_ = encoder_.getCumulativePosition();
    LOGI("Encoder zeroed");
    return true;
}


void MotorTask::calculateTotalSteps() {
    total_steps_ = full_steps_ * microsteps_;
    motor_encoder_ratio_ = total_steps_ / DEFAULT_ENCODER_POSITIONS;
    encoder_motor_ratio_ = DEFAULT_ENCODER_POSITIONS / total_steps_;
}


// 0 is open; 100 is closed.
inline int MotorTask::getPercent() {
    return static_cast<int>(static_cast<float>(encod_pos_) / encod_max_pos_ * 100 + 0.5);
}


inline int MotorTask::positionToStep(int encoder_position) {
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


void MotorTask::updateMotorSettings(float velocity, float acceleration, int current) {
    motor_->setSpeedInHz(static_cast<int>(total_steps_ * velocity));
    motor_->setAcceleration(static_cast<int>(total_steps_ * velocity * acceleration));

    // Set motor RMS current via UART, higher torque requires more current. The default holding
    // current (ihold) is 50% of irun but the ratio be adjusted with optional second argument, i.e.
    // rms_current(1000, 0.3).
    driver_.rms_current(current);

    // Inverse motor direction
    driver_.shaft(direction_);

    // Number of microsteps [0, 2, 4, 8, 16, 32, 64, 126, 256] per full step
    // Set MRES register via UART
    driver_.microsteps(microsteps_);

    // 1=SpreadCycle only; 0=StealthChop PWM mode (below velocity threshold) + SpreadCycle (above
    // velocity threshold); set register TPWMTHRS to determine the velocity threshold
    // SpreadCycle for high velocity but is audible; StealthChop is quiet and more torque.
    driver_.en_spreadCycle(spreadcycl_en_);
    driver_.TPWMTHRS(spreadcycl_th_);

    if (stallguard_en_) {
        // Lower threshold velocity for switching on CoolStep and StallGuard to DIAG output
        driver_.TCOOLTHRS(3089838.00 * pow(total_steps_ * velocity, -1.00161534));

        // StallGuard threshold [0... 255] level for stall detection. It compensates for motor
        // specific characteristics and controls sensitivity. A higher value makes StallGuard more
        // sensitive and requires less torque to stall. The double of this value is compared to
        // SG_RESULT. The stall output becomes active if SG_RESULT fall below this value.
        driver_.SGTHRS(stallguard_th_);

        // Enable StallGuard or else it will stall the motor when starting the driver
        attachInterrupt(DIAG_PIN, std::bind(&MotorTask::stallguardInterrupt, this), RISING);
    }
    vTaskDelay(5 / portTICK_PERIOD_MS);  // Wait for settings to be updated
}


void MotorTask::driverStartup() {
    if (!driver_stdby_) {
        LOGI("Driver already started");
        return;
    }

    // Pull standby pin low to disable driver standby
    digitalWrite(STBY_PIN, LOW);
    vTaskDelay(5 / portTICK_PERIOD_MS);  // Wait for driver to startup

    // Sets pdn_disable=1: disables automatic standstill current reduction, needed for UART; also
    // sets mstep_reg_select=1: use UART to change microstepping settings.
    driver_.begin();

    // Use voltage reference from internal 5VOut instead of analog VRef for current scaling
    driver_.I_scale_analog(0);

    // Enable StealthChop voltage PWM mode: automatic scaling current control taking into account
    // of the motor back EMF and velocity.
    driver_.pwm_autoscale(true);
    driver_.pwm_autograd(true);

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

    // StallGuard setup; refer to p29 and p73 of TMC2209's datasheet rev1.09 for tuning SG.
    if (stallguard_en_) {
        // 0=disable CoolStep
        // CoolStep lower threshold [0... 15].
        // If SG_RESULT goes below this threshold, CoolStep increases the current to both coils.
        driver_.semin(4);

        // CoolStep upper threshold [0... 15].
        // If SG is sampled equal to or above this threshold enough times, CoolStep decreases the
        // current to both coils.
        driver_.semax(0);
    }

    vTaskDelay(5 / portTICK_PERIOD_MS);  // Wait for driver to startup

    if (driver_.blank_time() == 24) {
        driver_stdby_ = false;
        LOGI("Driver has started");
    } else {
        driver_stdby_ = true;
    }
}


void MotorTask::driverStandby() {
    if (driver_stdby_) {
        LOGI("Driver already in standby");
        return;
    } else if (motor_->isRunning()) {
        LOGI("Driver can't be put in standby while motor is running");
        return;
    }

    driver_stdby_ = true;

    // Need to disable StallGuard or else it will stall the motor when disabling the driver
    detachInterrupt(DIAG_PIN);

    // Pull standby pin high to standby TMC2209 driver
    digitalWrite(STBY_PIN, HIGH);
    LOGI("Driver in standby");
}


void MotorTask::addWirelessTask(Task *task) {
    wireless_task_ = task;
}


void MotorTask::addSystemSleepTimer(xTimerHandle timer) {
    system_sleep_timer_ = timer;
}