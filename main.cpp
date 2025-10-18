#include "mbed.h"

/*
Formula SAE - Electronic Throttle Control (ETC) Project

This program simulates an ETC system for a Formula SAE car.
It reads pedal sensors, brake sensor, a cockpit RTD switch,
and wheel speed sensors, then applies safety checks before
outputting a final throttle value.

Features:
- Reads and converts pedal voltages into percentages
- Checks pedal implausibility between APPS0 and APPS1
- Ready To Drive (RTD) logic with buzzer alert
- Brake System Error (BSE) check: cuts throttle if brake + pedal pressed
- Launch Control: reduces torque if wheel slip detected
- Prints system state (pedal, brake, RTD, implausibility, slip, throttle)

Note: Pins, voltage ranges, and scaling must be updated for your board.
Look for:  // Update with board Information
*/

// Update with board Information
static constexpr float ADC_REFERENCE_VOLTAGE = 3.3f;
// Update with board Information
static constexpr float APPS0_MIN_VOLTAGE = 0.25f;
// Update with board Information
static constexpr float APPS0_MAX_VOLTAGE = 2.25f;
// Update with board Information
static constexpr float APPS1_MIN_VOLTAGE = 0.30f;
// Update with board Information
static constexpr float APPS1_MAX_VOLTAGE = 2.70f;
// Update with board Information
static constexpr float PEDAL_IMPLAUSIBILITY_THR = 10.0f;
static constexpr auto LOOP_DELAY = 50ms;

// Update with board Information
static constexpr float BRAKE_RTD_THRESHOLD_PERCENT = 80.0f;
static constexpr bool COCKPIT_SWITCH_ACTIVE_HIGH = true;
// Update with board Information
static constexpr auto RTD_BUZZER_DURATION = 1s;

// Update with board Information
static constexpr float BRAKE_IMPLAUS_THRESHOLD = 25.0f;
// Update with board Information
static constexpr float PEDAL_IMPLAUS_THRESHOLD = 25.0f;

// Update with board Information
static constexpr float SLIP_THRESHOLD = 5.0f;
// Update with board Information
static constexpr float TORQUE_REDUCTION_FACTOR = 0.7f;

// Update with board Information
AnalogIn pedalSensor0(A0);
// Update with board Information
AnalogIn pedalSensor1(A1);
// Update with board Information
AnalogIn brakeSensor(A2);
// Update with board Information
DigitalIn cockpitSwitch(PA_5);
// Update with board Information
DigitalOut rtdBuzzer(PB_5);

// Wheel speed sensors (for Launch Control)
// Update with board Information
AnalogIn frontWheelSpeedSensor(A3);
// Update with board Information
AnalogIn rearWheelSpeedSensor(A4);

static float clampValue(float inputValue, float minValue, float maxValue)
{
    float clampedValue;
    if (inputValue < minValue)
    {
        clampedValue = minValue;
    }
    else if (inputValue > maxValue)
    {
        clampedValue = maxValue;
    }
    else
    {
        clampedValue = inputValue;
    }
    return clampedValue;
}

static float readVoltageFromSensor(AnalogIn &sensor)
{
    float fractionOfScale = sensor.read();
    float voltage = fractionOfScale * ADC_REFERENCE_VOLTAGE;
    return voltage;
}

static float convertVoltageToPedalPercent(float voltage, float minVoltage, float maxVoltage)
{
    float range = maxVoltage - minVoltage;
    float percent;
    if (range == 0.0f)
    {
        percent = 0.0f;
    }
    else
    {
        percent = (voltage - minVoltage) * 100.0f / range;
    }
    percent = clampValue(percent, 0.0f, 100.0f);
    return percent;
}

static bool isImplausible(float pedalPercent0, float pedalPercent1)
{
    float difference = pedalPercent0 - pedalPercent1;
    if (difference < 0.0f)
    {
        difference = -difference;
    }

    if (difference > PEDAL_IMPLAUSIBILITY_THR)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void beepBuzzerForDuration(chrono::milliseconds duration)
{
    rtdBuzzer = 1;
    ThisThread::sleep_for(duration);
    rtdBuzzer = 0;
}

static bool isBSEImplausible(float brakePercent, float pedalPercent)
{
    if (brakePercent > BRAKE_IMPLAUS_THRESHOLD && pedalPercent > PEDAL_IMPLAUS_THRESHOLD)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool isWheelSlip(float frontSpeed, float rearSpeed)
{
    float difference = rearSpeed - frontSpeed;
    if (difference < 0.0f)
    {
        difference = -difference;
    }

    float slipPercent;
    if (frontSpeed > 0.1f)
    {
        slipPercent = (difference / frontSpeed) * 100.0f;
    }
    else
    {
        slipPercent = 0.0f;
    }

    if (slipPercent > SLIP_THRESHOLD)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int main()
{
    printf("====================================================\n");
    printf("   Formula SAE - Electronic Throttle Control (ETC)  \n");
    printf("   Student Project: Pedal Sensors, Safety, and RTD  \n");
    printf("====================================================\n\n");

    printf("System setup values:\n");
    printf("  ADC Reference Voltage: %.2f V\n", ADC_REFERENCE_VOLTAGE);
    printf("  APPS0 Range: %.2f V to %.2f V\n", APPS0_MIN_VOLTAGE, APPS0_MAX_VOLTAGE);
    printf("  APPS1 Range: %.2f V to %.2f V\n", APPS1_MIN_VOLTAGE, APPS1_MAX_VOLTAGE);
    printf("  Pedal Implausibility Threshold: %.1f %% difference\n", PEDAL_IMPLAUSIBILITY_THR);
    printf("  Brake Threshold for RTD: %.1f %%\n", BRAKE_RTD_THRESHOLD_PERCENT);
    printf("  Brake vs Pedal Implausibility: Brake > %.1f %% AND Pedal > %.1f %%\n",
           BRAKE_IMPLAUS_THRESHOLD, PEDAL_IMPLAUS_THRESHOLD);
    printf("  Wheel Slip Threshold: %.1f %% difference (launch control)\n", SLIP_THRESHOLD);
    printf("  Torque Reduction Factor when slipping: %.2f\n", TORQUE_REDUCTION_FACTOR);
    printf("----------------------------------------------------\n\n");

    Timer elapsedTimer;
    bool timerHasStarted = false;
    bool readyToDriveActive = false;

    while (true)
    {
        if (timerHasStarted == false)
        {
            elapsedTimer.start();
            timerHasStarted = true;
        }

        float sensor0Voltage = readVoltageFromSensor(pedalSensor0);
        float sensor1Voltage = readVoltageFromSensor(pedalSensor1);

        float pedalPercent0 = convertVoltageToPedalPercent(sensor0Voltage, APPS0_MIN_VOLTAGE, APPS0_MAX_VOLTAGE);
        float pedalPercent1 = convertVoltageToPedalPercent(sensor1Voltage, APPS1_MIN_VOLTAGE, APPS1_MAX_VOLTAGE);

        bool pedalImplausibilityDetected = isImplausible(pedalPercent0, pedalPercent1);

        float brakeVoltage = readVoltageFromSensor(brakeSensor);
        float brakePercent = convertVoltageToPedalPercent(brakeVoltage, APPS0_MIN_VOLTAGE, APPS0_MAX_VOLTAGE);

        int cockpitRawValue = cockpitSwitch.read();
        bool cockpitSwitchOn;
        if (COCKPIT_SWITCH_ACTIVE_HIGH == true)
        {
            if (cockpitRawValue == 1)
            {
                cockpitSwitchOn = true;
            }
            else
            {
                cockpitSwitchOn = false;
            }
        }
        else
        {
            if (cockpitRawValue == 0)
            {
                cockpitSwitchOn = true;
            }
            else
            {
                cockpitSwitchOn = false;
            }
        }

        bool brakePressedEnough;
        if (brakePercent >= BRAKE_RTD_THRESHOLD_PERCENT)
        {
            brakePressedEnough = true;
        }
        else
        {
            brakePressedEnough = false;
        }

        bool readyToDriveConditionsMet;
        if (brakePressedEnough == true && cockpitSwitchOn == true && pedalImplausibilityDetected == false)
        {
            readyToDriveConditionsMet = true;
        }
        else
        {
            readyToDriveConditionsMet = false;
        }

        if (readyToDriveActive == false && readyToDriveConditionsMet == true)
        {
            readyToDriveActive = true;
            beepBuzzerForDuration(chrono::duration_cast<chrono::milliseconds>(RTD_BUZZER_DURATION));
        }

        float finalPedalPercent;
        if (pedalImplausibilityDetected == true)
        {
            finalPedalPercent = 0.0f;
        }
        else
        {
            finalPedalPercent = (pedalPercent0 + pedalPercent1) / 2.0f;
        }

        bool bseImplausibilityDetected = isBSEImplausible(brakePercent, finalPedalPercent);
        if (bseImplausibilityDetected == true)
        {
            finalPedalPercent = 0.0f;
        }

        float frontWheelVoltage = readVoltageFromSensor(frontWheelSpeedSensor);
        float rearWheelVoltage = readVoltageFromSensor(rearWheelSpeedSensor);

        // Update with board Information
        float frontWheelSpeed = frontWheelVoltage;
        // Update with board Information
        float rearWheelSpeed = rearWheelVoltage;

        bool wheelSlipDetected = isWheelSlip(frontWheelSpeed, rearWheelSpeed);
        if (wheelSlipDetected == true)
        {
            finalPedalPercent = finalPedalPercent * TORQUE_REDUCTION_FACTOR;
        }

        if (readyToDriveActive == false)
        {
            finalPedalPercent = 0.0f;
        }

        int64_t elapsedMicroseconds = elapsedTimer.elapsed_time().count();
        int64_t elapsedMilliseconds = elapsedMicroseconds / 1000;

        printf("[Time: %8lld ms]  ", static_cast<long long>(elapsedMilliseconds));
        printf("APPS0: %.3f V (%.2f %%) | ", sensor0Voltage, pedalPercent0);
        printf("APPS1: %.3f V (%.2f %%) | ", sensor1Voltage, pedalPercent1);
        printf("Brake: %.2f %% | ", brakePercent);
        printf("Cockpit Switch: %s | ", (cockpitSwitchOn ? "ON" : "OFF"));
        printf("Ready To Drive: %s | ", (readyToDriveActive ? "YES" : "NO"));
        printf("Pedal Implausibility: %s | ", (pedalImplausibilityDetected ? "YES" : "NO"));
        printf("BSE Implausibility: %s | ", (bseImplausibilityDetected ? "YES" : "NO"));
        printf("Wheel Slip Detected: %s | ", (wheelSlipDetected ? "YES" : "NO"));
        printf("Final Pedal Output: %.2f %%\n", finalPedalPercent);

        ThisThread::sleep_for(LOOP_DELAY);
    }

    return 0;
}
