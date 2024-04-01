#include "LedModuleHW.h"
#include "LedModule.h"
#include "LightChannel.h"

#include "OpenKNX.h"

const std::string LedModule::name()
{
    return LEDMODULE_HARDWARE_NAME;
}

const std::string LedModule::version()
{
    // hides the module in the version output on the console, because the firmware version is sufficient.
    return "";
}

void LedModule::init()
{
    
    if (LEDMODULE_PWMDRIVER == "PCA9685PW") {
        logInfoP("PWM driver used: %s", LEDMODULE_PWMDRIVER);

        logInfoP("Define Wire1 SDA/SCL");
        Wire1.setSDA(LEDMODULE_WIRE1_SDA);
        Wire1.setSCL(LEDMODULE_WIRE1_SCL);
        logInfoP("Wire1.begin()");
        Wire1.begin(); // I2C1
        logInfoP("Configure Adafruit PWM lib");
        pwm = Adafruit_PWMServoDriver(0x40, Wire1);
        logInfoP("pwm.begin()");
        pwm.begin();
        logInfoP("pwm.setPWMFreq(1000)");
        pwm.setPWMFreq(1000);
        logInfoP("Wire1.setClock(1000000)");
        Wire1.setClock(1000000);
    } else {
        logErrorP("Unknown PWM driver %s ('RP2040' and 'PCA9685PW' are supported)", LEDMODULE_PWMDRIVER);
    }

}


void LedModule::setup(bool configured)
{

    delay(1000);
    logInfoP("Setup0");
    logIndentUp();

    setupCustomFlash();

    lights = new LedModuleHW(pwm);

    setupChannels();

    logIndentDown();

    logInfoP("Switch off all light on boot");
    for (int i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++) {
        pwm.setPWM(i, 0, 0);
    }

}

void LedModule::setupChannels()
{
    for (uint8_t i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
    {
        _channels[i] = new LightChannel(i);
        _channels[i]->Setup(lights);
    }
}

void LedModule::setupCustomFlash()
{
    logDebugP("initialize ledModule flash");
    OpenKNX::Flash::Driver _ledStorage;
#ifdef ARDUINO_ARCH_ESP32
    _ledStorage.init("ledModule");
#else
    _ledStorage.init("ledModule", LEDMODULE_FLASH_OFFSET, LEDMODULE_FLASH_SIZE);
#endif

    // logTraceP("write ledModule data");
    // _ledStorage.writeByte(0, 0x11);
    // _ledStorage.writeWord(1, 0xFFFF);
    // _ledStorage.writeInt(3, 6666666);
    // _ledStorage.commit();

    // logDebugP("read ledModule data");
    // logIndentUp();
    // logHexDebugP(_ledStorage.flashAddress(), 7);
    // logDebugP("byte: %02X", _ledStorage.readByte(0)); // UINT8
    // logDebugP("word: %i", _ledStorage.readWord(1));   // UINT16
    // logDebugP("int: %i", _ledStorage.readInt(3));     // UINT32
    // logIndentDown();
}

void LedModule::loop(bool configured)
{
    if (delayCheck(_timer1, 5100))
    {
        
        logInfoP("Loop0");

        _timer1 = millis();
    }

    if (delayCheck(_timerCheckConnection, 500)) {

        //If PWM side of the Adum1251 has no power and power returns, the PWM lib is not initialized
        if (checkConnection() && doResetPwm) {
            logInfoP("Reset PWM as connection is back");
            pwm = Adafruit_PWMServoDriver(0x40, Wire1);
            logInfoP("pwm.begin()");
            pwm.begin();
            logInfoP("pwm.setPWMFreq(1000)");
            pwm.setPWMFreq(1000);
            doResetPwm = false;

            logInfoP("Dimm back to last known values");

            lights->dimmToLastAfterI2cIsBack();

        }
        _timerCheckConnection = millis();
    }

    lights->dimmLoop();


    // // counter for processed channels in one loop
    // uint8_t channelProcessed = 0;


    // // skip when the free time has been used up or all channels have already been processed once.
    // while (channelProcessed < LED_ChannelCount && openknx.freeLoopTime())
    // {
    //     if (_channelIterator >= LED_ChannelCount) _channelIterator = 0;

    //     _channels[_channelIterator]->loop();
    //     _channelIterator++;
    //     channelProcessed++;
    // }
}

#ifdef OPENKNX_DUALCORE

void LedModule::setup1(bool configured)
{
    delay(1000);
    logInfoP("Setup1");
}

void LedModule::loop1(bool configured)
{
    if (delayCheck(_timer2, 7200))
    {
        logInfoP("Loop1");
        _timer2 = millis();
    }
}
#endif

void LedModule::processInputKo(GroupObject &ko)
{
    logDebugP("processInputKo GA%04X", ko.asap());
    logHexDebugP(ko.valueRef(), ko.valueSize());

    uint16_t asap = ko.asap();

    // if (asap == LED_KoScene) {
    //     logInfoP("Scene Info received");
    //     uint8_t sceneId = ko.value(DPT_SceneNumber);
    //     //activateScene(sceneId);
    //     return;
    // }

    
    int16_t channelnumber = (asap - LED_KoOffset ) / LED_KoBlockSize;
    if(channelnumber < LEDMODULE_MAX_LIGHT_CHANNELS)
        if(_channels[channelnumber] != nullptr) {
            logInfoP("KO for lightchannel received");
            _channels[channelnumber]->ProcessInputKo(ko);
        }

}

void LedModule::showHelp()
{
    openknx.console.printHelpLine("ledModule", "Print a ledModule text");
}

bool LedModule::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd.substr(0, 9) == "ledModule")
    {
        logInfoP("======================== Information ===========================================");
        logInfoP("LED MODULE INFORMATION");
        logInfoP("PWM driver:              %s", LEDMODULE_PWMDRIVER);
        if(LEDMODULE_PWMDRIVER == "PCA9685PW") {
            logInfoP("1Wire SDA:              %s", LEDMODULE_WIRE1_SDA);
            logInfoP("1Wire SCL:              %s", LEDMODULE_WIRE1_SCL);
        }
        logIndentUp();
        logInfoP("Info 1");
        logInfoP("Info 2");
        logIndentUp();
        logInfoP("Info 2a");
        logInfoP("Info 2b");
        logIndentDown();
        logInfoP("Info 3");
        logIndentDown();
        logInfoP("--------------------------------------------------------------------------------");
        return true;
    }

    return false;
}

//If KNX power goes off, activate emergency light
void LedModule::savePower() {
    for (uint8_t i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
    {
        //TODO_KPA: Move to channel
        // uint8_t value = knx.paramByte(LED_SceneEmergency_LEDCH0 + i );
        // pwm.setPWM(i, 0, value);
    }
}


// void LedModule::activateScene(uint8_t sceneId) {

//     if ((sceneId < 0) && (sceneId > 9)) {
//         logErrorP("Scene number not between 1 and 10");
//         return;
//     }

//     logInfoP("Activating KNX scene: %i", sceneId+1);

//     bool stateOnOff = false;
//     uint32_t scene[MAX_LIGHT_CHANNELS];


//     for (uint8_t i = 0; i < MAX_LIGHT_CHANNELS; i++)
//     {
//         uint8_t sceneOffset = sceneId * MAX_LIGHT_CHANNELS;
//         uint8_t value = knx.paramByte(LED_Scene1_LEDCH0 + i + sceneOffset);

//         scene[i] = value; //Convert 8bit to 12bit value
//         //logDebugP("CH%i value: %i", i, value);
//     }

//     for (int i = 0; i < MAX_LIGHT_CHANNELS; i++) {

//         uint32_t value = scene[i];
//         //If at least one LED channel is on, OnOff status KO should be "on"
//         if (value > 0) {
//             stateOnOff = true;
//         }
//     }
    

//     KoLED_StateOnOff.value(stateOnOff, DPT_State);
    
//     //dimmLightTo(0, scene);
// }






//checkConnection return 1 if there is no error
int LedModule::checkConnection() {

    byte error;

    Wire1.beginTransmission(0x40);
    error = Wire1.endTransmission();

    if (error == 0) {
        return 1;
    } else {
        logErrorP("PCA9685 PWM not available via I2C %s", error);
        doResetPwm = true;
    }
    return 0;
}



LedModule openknxLedModule;