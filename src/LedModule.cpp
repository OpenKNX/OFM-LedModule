#include "LedModule.h"
#include "LedModuleHW.h"
#include "LightChannel.h"

#include "OpenKNX.h"
#include "LedModuleConfig.h"

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
#if defined(LEDMODULE_DIMMER_PCA9685)
    dimmer = new HWDimmerPCA(HWDimmerPCA::PCAType::PCA9685);
#else
#if defined(LEDMODULE_DIMMMER_RP2040)
    dimmer = new HWDimmerRP2040(dimPins, LEDMODULE_MAX_LIGHT_CHANNELS, LEDMODULE_PWM_FREQ);
#else // create dummy driver to have dimmer initialized
    dimmer = new HWDimmer(1);
    logErrorP("Unknown PWM driver %s ('RP2040' and 'PCA9685PW' are supported)", LEDMODULE_PWMDRIVER);
#endif
#endif
}

void LedModule::setup(bool configured)
{
    delay(1000);
    logInfoP("Setup0");
    logIndentUp();

    setupCustomFlash();

    lights = new LedModuleHW(dimmer);

    setupChannels();

    logIndentDown();

    logInfoP("Switch off all light on boot");
    for (int i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
    {
        dimmer->setLevel(0, i);
    }
    delay(1000);
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
    // TODO: Reactivate flash storage when values are know and functionality is needed
    // _ledStorage.init("ledModule", LEDMODULE_FLASH_OFFSET, LEDMODULE_FLASH_SIZE);
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

    if (delayCheck(_timerCheckConnection, 500))
    {
        // If PWM side of the Adum1251 has no power and power returns, the PWM lib is not initialized
        if (!dimmer->checkConnection())
        {
            if (doResetPwm)
            {
                dimmer->reconnect();
                lights->dimmToLastAfterI2cIsBack();
            }
        }
        else
        {
            doResetPwm = true;
        }
        _timerCheckConnection = millis();
    }
    lights->dimmLoop();
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
    // logDebugP("processInputKo GA%04X", ko.asap());
    // logHexDebugP(ko.valueRef(), ko.valueSize());

    uint16_t asap = ko.asap();
    uint16_t channelnumber = (asap - LED_KoOffset) / LED_KoBlockSize;
    if (channelnumber < LEDMODULE_MAX_LIGHT_CHANNELS)
    {
        if (_channels[channelnumber] != nullptr)
        {
            // logInfoP("KO for lightchannel received");
            _channels[channelnumber]->ProcessInputKo(ko);
        }
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
#ifdef LEDMODULE_DIMMER_PCA9685
        logInfoP("1Wire SDA:              %s", LEDMODULE_WIRE1_SDA);
        logInfoP("1Wire SCL:              %s", LEDMODULE_WIRE1_SCL);
#endif
#ifdef LEDMODULE_DIMMMER_RP2040
        std::string tmp = "Dimmer Pins: ";
        for (int i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
        {
            tmp.append(std::to_string(dimPins[i]));
            tmp.append(", ");
        }
        logInfoP("%s", tmp.c_str());
#endif
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

// If KNX power goes off, activate emergency light
void LedModule::savePower()
{
    for (uint8_t i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
    {
        // TODO_KPA: Move to channel
        //  uint8_t value = knx.paramByte(LED_SceneEmergency_LEDCH0 + i );
        //  pwm.setPWM(i, 0, value);
    }
}

LedModule openknxLedModule;