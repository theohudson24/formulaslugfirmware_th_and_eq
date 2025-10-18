# FormulaSlug ETC Prototype (Mbed OS)

Language Used: C++

**Authors**
* Theo Hudson
* Ernesto Quintanilla

---

## What this project does

The firmware reads two accelerator pedal position sensors (APPS0 and APPS1), a brake sensor, a cockpit switch, and two wheel speed sensors. It computes a safe pedal command with these protections:

* Pedal sensor range and clamping
* APPS cross-check for implausibility
* Brake System Encoder style check: if brake and pedal are both high, cut output
* Ready To Drive sequence that requires brake plus cockpit switch
* Simple launch control that reduces torque when slip is detected
* Periodic console logging for debugging

All thresholds and ranges are defined at the top of `main.cpp` and currently use placeholder values marked with the comment **Update with board Information**. We will replace those with the real values when we have access to the board and sensors.

---

## Hardware and default pin mapping

These defaults match the current `main.cpp`. Still need to tune them to the given board:

| Function | Default pin |
|---|---|
| APPS0 analog input | A0 |
| APPS1 analog input | A1 |
| Brake analog input | A2 |
| Cockpit switch digital input | PA_5 |
| RTD buzzer digital output | PB_5 |
| Front wheel speed analog input | A3 |
| Rear wheel speed analog input | A4 |

**Thresholds and constants** (placeholders to be tuned on hardware)

* `ADC_REFERENCE_VOLTAGE` 3.3 V
* `APPS0_MIN_VOLTAGE` 0.25 V
* `APPS0_MAX_VOLTAGE` 2.25 V
* `APPS1_MIN_VOLTAGE` 0.30 V
* `APPS1_MAX_VOLTAGE` 2.70 V
* `PEDAL_IMPLAUSIBILITY_THR` 10 percent absolute difference
* `BRAKE_RTD_THRESHOLD_PERCENT` 80 percent
* `COCKPIT_SWITCH_ACTIVE_HIGH` true
* `RTD_BUZZER_DURATION` 1 s
* `BRAKE_IMPLAUS_THRESHOLD` 25 percent
* `PEDAL_IMPLAUS_THRESHOLD` 25 percent
* `SLIP_THRESHOLD` 5 percent
* `TORQUE_REDUCTION_FACTOR` 0.7

---

