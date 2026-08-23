/*
 * disk_temp.cc
 * Copyright (C) 2026 spin24
 *
 * Distributed under terms of the 3-clause BSD license.
 */

#include "disk_temp.h"

#include <dirent.h>
#include <stdio.h>

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace bsdsensors {

namespace {

bool IsDiskDevice(const std::string& name,
                  const std::vector<std::string>& prefixes) {
    for (const auto& prefix : prefixes) {
        if (name.compare(0, prefix.size(), prefix) != 0) {
            continue;
        }
        const std::string rest = name.substr(prefix.size());
        if (rest.empty()) {
            continue;
        }
        bool all_digits = true;
        for (const char c : rest) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                all_digits = false;
                break;
            }
        }
        if (all_digits) {
            return true;
        }
    }
    return false;
}

bool RunSmartctl(const std::string& dev, std::string* output) {
    const std::string cmd = "smartctl -a " + dev + " 2>/dev/null";
    FILE* fp = popen(cmd.c_str(), "r");
    if (fp == nullptr) {
        return false;
    }
    std::ostringstream ss;
    char buf[512];
    while (fgets(buf, sizeof(buf), fp) != nullptr) {
        ss << buf;
    }
    pclose(fp);
    *output = ss.str();
    return true;
}

// Extracts the temperature from smartctl text output.
// Handles ATA attribute tables (SATA), NVMe and SAS formats.
bool ParseSmartTemperature(const std::string& text, double* temp) {
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        // NVMe: "Temperature: 38 Celsius"
        if (line.compare(0, 12, "Temperature:") == 0) {
            const size_t celsius = line.find("Celsius");
            if (celsius != std::string::npos) {
                const double value = atof(line.substr(12).c_str());
                if (value > 0 && value < 150) {
                    *temp = value;
                    return true;
                }
            }
            continue;
        }
        // SAS: "Current Drive Temperature:     34 C"
        if (line.find("Current Drive Temperature:") != std::string::npos) {
            const size_t pos = line.find(':');
            const double value = atof(line.c_str() + pos + 1);
            if (value > 0 && value < 150) {
                *temp = value;
                return true;
            }
            continue;
        }
        // SATA ATA attribute: "194 Temperature_Celsius ... 33"
        std::istringstream ls(line);
        long id = 0;
        if (!(ls >> id)) {
            continue;
        }
        if (id != 194 && id != 190) {
            continue;
        }
        if (line.find("Temperature_Celsius") == std::string::npos &&
            line.find("Airflow_Temperature_Cel") == std::string::npos) {
            continue;
        }
        std::string token;
        std::string value_col;
        int column = 0;
        while (ls >> token) {
            if (column == 3) {  // normalized VALUE column
                value_col = token;
                break;
            }
            ++column;
        }
        if (!value_col.empty()) {
            const double value = atof(value_col.c_str());
            if (value > 0 && value < 150) {
                *temp = value;
                return true;
            }
        }
    }
    return false;
}

}  // namespace

void AddDiskTemperatures(SensorsProto* sensors) {
    DIR* dir = opendir("/dev");
    if (dir == nullptr) {
        return;
    }

    const std::vector<std::string> prefixes = {"ada", "da", "nvme"};
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        const std::string name(entry->d_name);
        if (!IsDiskDevice(name, prefixes)) {
            continue;
        }
        const std::string dev = "/dev/" + name;
        std::string output;
        if (!RunSmartctl(dev, &output)) {
            continue;
        }
        double temp = 0;
        if (!ParseSmartTemperature(output, &temp)) {
            continue;
        }
        TemperatureProto* proto = sensors->add_temperatures();
        proto->set_name(name);
        proto->set_value(temp);
        proto->set_source("smartctl");
    }
    closedir(dir);
}

}  // namespace bsdsensors
