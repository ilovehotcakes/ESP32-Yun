def modify_fastaccelstepper_h():
    result = ""
    with open("./.pio/libdeps/esp32dev/FastAccelStepper/src/FastAccelStepper.h", 'r') as file:
        for line in file:
            if line == "#include <stdint.h>\n":
                result += line
                result += "#include <FunctionalInterrupt.h>\n"
            elif line == "  void setExternalEnableCall(bool (*func)(uint8_t enablePin, uint8_t value));\n":
                result += "  void setExternalEnableCall(std::function<bool(uint8_t, uint8_t)> func);\n"
            elif line == "  bool (*_externalEnableCall)(uint8_t enablePin, uint8_t value);\n":
                result += "  std::function<bool(uint8_t, uint8_t)> _externalEnableCall;\n"
            else:
                result += line

    with open("./.pio/libdeps/esp32dev/FastAccelStepper/src/FastAccelStepper.h", 'w') as output:
        output.write(result)


def modify_fastaccelstepper_cpp():
    result = ""
    with open("./.pio/libdeps/esp32dev/FastAccelStepper/src/FastAccelStepper.cpp", 'r') as file:
        for line in file:
            if line == "void FastAccelStepper::setExternalEnableCall(bool (*func)(uint8_t enablePin,\n":
                result += "void FastAccelStepper::setExternalEnableCall(std::function<bool(uint8_t, uint8_t)> func) {\n"
            elif line == "                                                          uint8_t value)) {\n":
                continue
            else:
                result += line

    with open("./.pio/libdeps/esp32dev/FastAccelStepper/src/FastAccelStepper.cpp", 'w') as output:
        output.write(result)


def modify_stpperisr_esp32_cpp():
    result = ""
    with open("./.pio/libdeps/esp32dev/FastAccelStepper/src/StepperISR_esp32.cpp", 'r') as file:
        for line in file:
            if line == "#define STACK_SIZE 1000\n":
                result += "#define STACK_SIZE 2000\n"
            else:
                result += line

    with open("./.pio/libdeps/esp32dev/FastAccelStepper/src/StepperISR_esp32.cpp", 'w') as output:
        output.write(result)


modify_fastaccelstepper_h()
modify_fastaccelstepper_cpp()
modify_stpperisr_esp32_cpp()