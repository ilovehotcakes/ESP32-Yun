#include "motor.h"


Motor::Motor() {}


void Motor::init() {
    maxPos = 0;
    currPos = 0;
    prevPos = 0;
    currState = MOTOR_IDLE;
    prevState = MOTOR_IDLE;
    isMotorRunning = false;

    // Driver setup
    pinMode(EN_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(STEP_PIN, OUTPUT);
    SERIAL_PORT.begin(115200, SERIAL_8N1, 16, TXD2); // Initialize HardwareSerial for hardware UART driver; remapped TXD2 from GPIO 17 to GPIO 22
    driver.begin();                 // Begin sending data
    driver.toff(4);                 // Not used in StealthChop but required to enable the motor, 0=off
    driver.pdn_disable(true);       // PDN_UART input disabled; set this bit when using the UART interface
    driver.rms_current(openingRMS); // Motor RMS current "rms_current will by default set ihold to 50% of irun but you can set your own ratio with additional second argument; rms_current(1000, 0.3)."
    driver.pwm_autoscale(true);     // Needed for StealthChop
    driver.en_spreadCycle(false);   // Disable SpreadCycle; SpreadCycle is faster but louder
    driver.blank_time(24);          // Comparator blank time. Needed to safely cover the switching event and the duration of the ringing on the sense resistor.
    driver.microsteps(microsteps);
    driver.shaft(flipDir);

    // Use StallGuard if user specifies connection to DIAG_PIN && RXD2
    #ifdef DIAG_PIN
    if (enableSG) {
        pinMode(DIAG_PIN, INPUT);
        driver.semin(4);              // CoolStep/SmartEnergy 4-bit uint that sets lower threshold, 0=disable
        driver.semax(0);              // Refer to p58 of the datasheet
        driver.TCOOLTHRS((3089838.00 * pow(float(maxSpeed), -1.00161534)) * 1.5);  // Lower threshold velocity for switching on CoolStep and StallGuard to DIAG
        driver.SGTHRS(sgThreshold);   // [0..255] the higher the more sensitive to stall
        attachInterrupt(DIAG_PIN, std::bind(&Motor::stallguardInterrupt, this), RISING);
    }
    #endif

    // Stepper motor acceleration library setup
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

    // Load current position and maximum position from motorSettings
    loadSettings();
    // LOGI("Motor setup complete");
}


// For StallGuard
#ifdef DIAG_PIN
void IRAM_ATTR Motor::stallguardInterrupt() {
    stepper->forceStop();
    // LOGE("Motor stalled");
}
#endif


void Motor::setMotorState(MotorState newState) {
    prevState = currState;
    currState = newState;
}


void Motor::loadSettings() {
    motorSettings.begin("local", false);
    maxPos = motorSettings.getInt("maxPos", 30000);
    currPos = motorSettings.getInt("currPos", 0);
    stepper->setCurrentPosition(currPos);
    // LOGI("Motor settings loaded(curr/max): %d/%d", currPos, maxPos);
}


void Motor::resetSettings() {
    motorSettings.clear();
    ESP.restart();
}


int Motor::percentToSteps(int percent) const {
    float x = (float) percent * (float) maxPos / 100.0;
    return (int) round(x);
}


// Motor must move first before isMotorRunning==true, else motorRun will excute first before stepper stops
void Motor::moveTo(int newPos) {
    if ((prevState == MOTOR_MAX && currState == MOTOR_MIN)
    || (prevState == MOTOR_MIN && currState == MOTOR_MAX))
        stepper->stopMove();
    
    if (newPos != stepper->getCurrentPosition() && newPos <= maxPos) {
        stepper->moveTo(newPos);
        isMotorRunning = true;
        // LOGD("Motor moving(tar/curr/max): %d/%d/%d", newPos, stepper->getCurrentPosition(), maxPos);
    }
}


void Motor::move(int percent) {
    // if openingRMS vs closingRMS
    moveTo(percentToSteps(percent));
}


void Motor::min() {
    setMotorState(MOTOR_MIN);
    driver.rms_current(openingRMS);
    stepper->setSpeedInHz(maxSpeed);
    moveTo(0);
}


void Motor::max() {
    setMotorState(MOTOR_MAX);
    driver.rms_current(closingRMS);
    stepper->setSpeedInHz(maxSpeed);
    moveTo(maxPos);
}


void Motor::setMin() {
    setMotorState(MOTOR_SET_MIN);
    driver.rms_current(openingRMS);
    stepper->setSpeedInHz(maxSpeed / 4);
    prevPos = stepper->getCurrentPosition();
    stepper->setCurrentPosition(INT_MAX);
    // LOGD("Motor setting new min position");
    moveTo(0);
}


void Motor::setMax() {
    setMotorState(MOTOR_SET_MAX);
    driver.rms_current(closingRMS);
    stepper->setSpeedInHz(maxSpeed / 4);
    maxPos = INT_MAX;
    // LOGD("Motor setting new max position");
    moveTo(INT_MAX);
}


// Returns current rounded position percentage. 0 is open; 100 is closed.
// TODO: add inaccuracy mode
int Motor::currentPercentage() {
    if (currPos == 0)
        return 0;
    else
        return (int) round((float) currPos / (float) maxPos * 100);
}


void Motor::stop() {
    stepper->forceStop();
    stepper->moveTo(stepper->getCurrentPosition());
}


void Motor::updatePosition() {
    if (currState == MOTOR_SET_MAX) {
        maxPos = stepper->getCurrentPosition();
        motorSettings.putInt("maxPos", maxPos);
        // LOGD("Set max position, new max position: %d", maxPos);
    } else if (currState == MOTOR_SET_MIN) {
        int distanceTraveled = INT_MAX - stepper->getCurrentPosition();
        maxPos = maxPos + distanceTraveled - prevPos;
        motorSettings.putInt("maxPos", maxPos);
        stepper->setCurrentPosition(0);
        // LOGD("Set min position, new max position: %d", maxPos);
    }

    setMotorState(MOTOR_IDLE);
    currPos = stepper->getCurrentPosition();
    motorSettings.putInt("currPos", currPos);
    // LOGD("Motor stopped(curr/max): %d/%d", currPos, maxPos);
}


void Motor::run() {
    if (isMotorRunning) {
        vTaskDelay(1);
        if (!stepper->isRunning()) {
            isMotorRunning = false;

            updatePosition();

            // sendMqtt((String) motorCurrentPercentage());
        }
    }
}