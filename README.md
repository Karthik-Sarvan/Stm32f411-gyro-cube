# STM32F411 gyro cube

Bare-metal orientation demo for the **32F411E-DISCO MB1115D**. The board
streams sensor angles over USB and the laptop displays them on an OpenGL cube.

## Hardware

Connect both USB cables:

- **ST-LINK** for programming
- **USB_USER** for serial data (`/dev/ttyACM0`)

The firmware uses the onboard gyro on SPI1 and the accelerometer/magnetometer
on I2C1. No external wiring is required.

## Build and flash

Requirements: `make`, `arm-none-eabi-gcc`, and `st-flash`.

```sh
make
make flash
```

Keep the board still for about half a second after reset for gyro calibration.

## Run

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install -r host/requirements.txt
python host/cube.py --port /dev/ttyACM0
```

Use `--rotation-gain` to change sensitivity. Press **Escape** to exit.

```sh
python host/cube.py --port /dev/ttyACM0 --rotation-gain 3
```

The serial format is:

```text
GYRO,roll_deg,pitch_deg,yaw_deg
```

See [`docs/README.md`](docs/README.md) for the local hardware documents.
