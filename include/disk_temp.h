/*
 * disk_temp.h
 * Copyright (C) 2026 spin24
 *
 * Distributed under terms of the 3-clause BSD license.
 */

#ifndef __BSDSENSORS_DISK_TEMP_H__
#define __BSDSENSORS_DISK_TEMP_H__

#include "sensors.pb.h"

namespace bsdsensors {

// Appends disk temperatures reported by smartctl(8) to the sensor list.
// Best effort: silently skips missing smartctl or unparsable output.
void AddDiskTemperatures(SensorsProto* sensors);

}  // namespace bsdsensors

#endif  // __BSDSENSORS_DISK_TEMP_H__
