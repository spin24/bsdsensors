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

}  // namespace

// Extracts the temperature from smartctl text output.
// Handles ATA attribute tables (SATA), NVMe and SAS formats.
bool ParseSmartTemperature(const std::string& text, double* temp) {
    double ata194 = -1, ata190 = -1;
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
        // SATA ATA attribute line:
        // "194 Temperature_Celsius 0x0002 044 044 000 Old_age Always - 37 (Min/Max 21/59)"
        std::istringstream ls(line);
        long id = 0;
        if (!(ls >> id) || (id != 194 && id != 190)) {
            continue;
        }
        if (line.find("Temperature_Celsius") == std::string::npos &&
            line.find("Airflow_Temperature_Cel") == std::string::npos) {
            continue;
        }
        std::vector<std::string> tokens;
        std::string token;
        while (ls >> token) {
            tokens.push_back(token);
        }
        // RAW_VALUE follows the UPDATED + WHEN_FAILED columns
        // (WHEN_FAILED is "-"), e.g. "... Always - 37 (Min/Max 21/59)".
        for (size_t i = 1; i + 1 < tokens.size(); ++i) {
            if (tokens[i] != "-" ||
                (tokens[i - 1] != "Always" && tokens[i - 1] != "Offline")) {
                continue;
            }
            const char* raw = tokens[i + 1].c_str();
            if (!std::isdigit(static_cast<unsigned char>(raw[0]))) {
                break;
            }
            const double value = atof(raw);
            if (value > 0 && value < 150) {
                if (id == 194) {
                    ata194 = value;
                } else {
                    ata190 = value;
                }
            }
            break;
        }
    }
    if (ata194 > 0) {
        *temp = ata194;
        return true;
    }
    if (ata190 > 0) {
        *temp = ata190;
        return true;
    }
    return false;
}

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
