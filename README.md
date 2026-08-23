# bsdsensors

Hardware sensor monitoring for FreeBSD: temperatures, voltages, fan speeds
and fan control, read directly from Super I/O chips.

[![build](https://github.com/spin24/bsdsensors/actions/workflows/build.yml/badge.svg)](https://github.com/spin24/bsdsensors/actions/workflows/build.yml)

## Features

* Read temperature sensors
* Set temperature sensor source
* Read fan speed and duty cycle
* View and configure fan control methods (SmartFan IV, Thermal Cruise,
  Speed Cruise, Manual)
* Read voltage sensors
* Read disk temperatures via `smartctl(8)` (SATA, SAS and NVMe)

## Quick start

CI builds a fully static FreeBSD binary on every push to `master`:

```sh
fetch https://github.com/spin24/bsdsensors/releases/download/ci-latest/bsdsensors
chmod +x bsdsensors
sudo ./bsdsensors
```

Root privileges are required to access `/dev/io`.

## Usage

```
bsdsensors [options]

  --sensors=LIST   comma-separated sensors to print, e.g.
                   temp:CPUTIN,volt:Vcore,fan:SYSFAN (with --value prints raw value only)
  --chip=NAME      restrict output to one chip model
  --debug          verbose logging
  --quiet          suppress log output
  --proto          print raw protobuf dump
  --json           print sensors as JSON
  --nodisks        skip disk temperatures (smartctl)
  --request=JSON   apply configuration changes (fan control etc.)
  --dump           dump every register of the detected chip (can be dangerous)
```

Default output:

```
Temperatures:
SENSOR             VALUE  SOURCE
----------------  ------  -----------
SMIOVT1            111 C  from SYSTIN
SMIOVT2           33.5 C  from CPUTIN
CPUTIN              33 C
...

Voltages:
SENSOR    VALUE
------  -------
Vcore    0.88 V
...

Fans:
FAN       RPM  DUTY  METHOD       TEMP SOURCE
-------  ----  ----  -----------  ---------------
SYSFAN    891   34%  SmartFan IV  CPUTIN (33.5 C)
CPUFAN   1269   34%  SmartFan IV  PECI0 (33 C)
...
```

Readings of ~100-111 C on unconnected inputs (`AUXTIN1..3`, unused `TSI*`,
`DIM*`) are normal; those channels float when nothing is attached.

## Supported chips

### Nuvoton

| Chip                    | Status                        |
| ----------------------- | ----------------------------- |
| NCT5532D                | untested                      |
| NCT5577D                | untested (ID shared w/ NCT6776F) |
| NCT5562D                | untested                      |
| NCT6102D / NCT6106D     | untested                      |
| NCT6627UD / W83627UHG   | untested                      |
| NCT6776F / NCT6776D     | untested                      |
| NCT6779D                | untested                      |
| NCT6791D (incl. 0xc803) | tested on ASUS H97-Plus       |
| NCT6793D                | tested on ASUS Z270-A PRIME   |
| NCT6796D                | tested on ASUS PRIME Z790-A WIFI |
| NCT6799D-R              | tested on ASUS TUF GAMING B650-PLUS WIFI |
| W83627HG-AW             | untested                      |
| W83627DHG               | tested on Supermicro X7SPA-HF |
| W83627DHG-P / -PT       | untested                      |
| W83627EHF/EHG, EF/EG    | untested                      |
| W83667HG                | tested on ASUS P5Q & P6T-SE   |
| W83667HG-A              | untested                      |
| W83697HF                | untested                      |

### ITE

* IT8772E (work in progress)

### Fintek

* F71869A

### Microchip

Not started.

## Building from source

On FreeBSD:

```sh
pkg install -y cmake protobuf gflags glog
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To produce a single self-contained executable (no shared libraries needed at
runtime), configure with `-DBSDSENSORS_STATIC_BIN=ON`. This expects static
archives (`libprotobuf.a`, `libglog.a`, `libgflags.a`) to be available; see
`.github/workflows/build.yml` for how CI builds them from source.

Dependencies: CMake >= 3.16, protobuf, gflags, glog (GTest optional for tests).

## License

BSD 3-Clause. Copyright (c) 2018 Henry Hu, portions copyright (c) 2026
spin24. See [LICENSE](LICENSE).
