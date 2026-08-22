/*
 * value_util.cc
 * Copyright (C) 2018 Henry Hu
 *
 * Distributed under terms of the 3-clause BSD license.
 */

#include "value_util.h"
#include "nuvoton_fan_control.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>

namespace bsdsensors {

using namespace std;

void PrintTempValue(const TemperatureProto& temp, std::ostream& out) {
    out << "Temperature " << temp.name() << ": " << temp.value() << " C";
    if (!temp.source().empty()) {
        out << " from " << temp.source();
    }
    out << std::endl;
}

void PrintFanSpeedValue(const FanProto& fan, std::ostream& out) {
    out << "  Speed: " << fan.speed().value() << " RPM" << std::endl;
}

void PrintFanStatus(const FanProto& fan, std::ostream& out) {
    out << "Fan " << fan.name() << std::endl;
    if (fan.has_speed()) {
        PrintFanSpeedValue(fan, out);
    }
    if (fan.has_control()) {
        out << fan.control();
    }
}

void PrintVoltValue(const VoltageProto& volt, std::ostream& out) {
    out << "Voltage " << volt.name() << ": " << volt.value() << " V";
    out << std::endl;
}

void PrintSensorValues(const SensorsProto& sensors, std::ostream& out) {
    size_t label_width = 8;
    for (const auto& temp : sensors.temperatures()) {
        label_width = max(label_width, temp.name().size() + 2);
    }
    for (const auto& volt : sensors.voltages()) {
        label_width = max(label_width, volt.name().size() + 2);
    }
    for (const auto& fan : sensors.fans()) {
        label_width = max(label_width, fan.name().size() + 2);
    }

    const int kValueWidth = 10;
    auto print_row = [&](const string& name, const string& value,
                         const string& suffix) {
        out << name << ":"
            << string(label_width - min(label_width, name.size() + 1), ' ');
        out << string(kValueWidth - min<size_t>(kValueWidth, value.size()),
                     ' ')
            << value;
        if (!suffix.empty()) {
            out << "  " << suffix;
        }
        out << "\n";
    };

    for (const auto& volt : sensors.voltages()) {
        const double v = volt.value();
        ostringstream ss;
        ss << fixed << setprecision(2);
        if (fabs(v) < 1.0) {
            ss << v * 1000.0 << " mV";
        } else {
            ss << v << " V";
        }
        print_row(volt.name(), ss.str(), "");
    }

    for (const auto& temp : sensors.temperatures()) {
        ostringstream ss;
        ss << fixed << setprecision(1) << showpos << temp.value() << noshowpos
           << "°C";
        string suffix;
        if (!temp.source().empty()) {
            suffix = "(from " + temp.source() + ")";
        }
        print_row(temp.name(), ss.str(), suffix);
    }

    for (const auto& fan : sensors.fans()) {
        if (!fan.has_speed()) {
            continue;
        }
        ostringstream ss;
        ss << fan.speed().value() << " RPM";
        string suffix;
        if (fan.has_control()) {
            std::vector<std::string> parts;
            const auto& control = fan.control();
            if (control.has_current_percent()) {
                parts.push_back(
                    std::to_string(
                        static_cast<int>(control.current_percent() * 100)) +
                    "%");
            }
            if (control.has_current_method()) {
                parts.push_back(control.current_method());
            }
            if (control.has_temp_source()) {
                std::string source = control.temp_source();
                if (control.has_temp_value()) {
                    ostringstream ts;
                    ts << fixed << setprecision(1) << control.temp_value()
                       << " C";
                    source += " at " + ts.str();
                }
                parts.push_back(source);
            }
            for (size_t i = 0; i < parts.size(); ++i) {
                if (i > 0) suffix += ", ";
                suffix += parts[i];
            }
            if (!suffix.empty()) {
                suffix = "(" + suffix + ")";
            }
        }
        print_row(fan.name(), ss.str(), suffix);
    }
}

namespace {

std::string NumToString(double value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

void PrintAlignedTable(std::ostream& out,
                       const std::vector<std::string>& headers,
                       std::vector<std::vector<std::string>> rows,
                       const std::vector<bool>& right_align) {
    std::vector<size_t> widths(headers.size(), 0);
    for (size_t i = 0; i < headers.size(); ++i) {
        widths[i] = headers[i].size();
    }
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size() && i < widths.size(); ++i) {
            widths[i] = max(widths[i], row[i].size());
        }
    }

    auto print_line = [&](const std::vector<std::string>& cells) {
        for (size_t i = 0; i < cells.size(); ++i) {
            const bool right =
                i < right_align.size() && right_align[i] && cells[i].size();
            if (right) {
                out << std::string(widths[i] - cells[i].size(), ' ');
            }
            out << cells[i];
            if (!right && i + 1 < cells.size()) {
                out << std::string(widths[i] - min(widths[i], cells[i].size()),
                                   ' ');
            }
            if (i + 1 < cells.size()) {
                out << "  ";
            } else {
                out << "\n";
            }
        }
    };

    print_line(headers);
    for (size_t i = 0; i < widths.size(); ++i) {
        out << std::string(widths[i], '-');
        if (i + 1 < widths.size()) {
            out << "  ";
        }
    }
    out << "\n";
    for (const auto& row : rows) {
        print_line(row);
    }
}

}  // namespace

void PrintSensorValuesTable(const SensorsProto& sensors, std::ostream& out) {
    std::vector<std::vector<std::string>> temp_rows;
    size_t max_name_width = 4;
    for (const auto& temp : sensors.temperatures()) {
        max_name_width = max(max_name_width, temp.name().size());
    }
    for (const auto& volt : sensors.voltages()) {
        max_name_width = max(max_name_width, volt.name().size());
    }

    out << "Temperatures:\n";
    temp_rows.clear();
    for (const auto& temp : sensors.temperatures()) {
        std::vector<std::string> row{temp.name(),
                                     NumToString(temp.value()) + " C"};
        if (!temp.source().empty()) {
            row.push_back("from " + temp.source());
        }
        temp_rows.push_back(std::move(row));
    }
    PrintAlignedTable(out, {"SENSOR", "VALUE", "SOURCE"}, temp_rows,
                      {false, true, false});

    out << "\nVoltages:\n";
    std::vector<std::vector<std::string>> volt_rows;
    for (const auto& volt : sensors.voltages()) {
        volt_rows.push_back(
            {volt.name(), NumToString(volt.value()) + " V"});
    }
    PrintAlignedTable(out, {"SENSOR", "VALUE"}, volt_rows, {false, true});

    out << "\nFans:\n";
    std::vector<std::vector<std::string>> fan_rows;
    for (const auto& fan : sensors.fans()) {
        std::vector<std::string> row{fan.name()};
        if (fan.has_speed()) {
            row.push_back(NumToString(fan.speed().value()));
        } else {
            row.push_back("-");
        }
        if (fan.has_control() &&
            fan.control().has_current_percent()) {
            row.push_back(std::to_string(
                static_cast<int>(fan.control().current_percent() * 100)) +
                          "%");
        } else {
            row.push_back("-");
        }
        if (fan.has_control() &&
            fan.control().has_current_method()) {
            row.push_back(fan.control().current_method());
        } else {
            row.push_back("-");
        }
        if (fan.has_control() && fan.control().has_temp_source()) {
            std::string source = fan.control().temp_source();
            if (fan.control().has_temp_value()) {
                source += " (" +
                          NumToString(fan.control().temp_value()) + " C)";
            }
            row.push_back(source);
        } else {
            row.push_back("-");
        }
        fan_rows.push_back(std::move(row));
    }
    PrintAlignedTable(out,
                      {"FAN", "RPM", "DUTY", "METHOD", "TEMP SOURCE"},
                      fan_rows, {false, true, true, false, false});
}

std::ostream& operator<<(std::ostream& out,
                         const FanControlProto& fan_control) {
    if (fan_control.has_current_percent()) {
        out << "  Current: " << (int)(fan_control.current_percent() * 100) << "%" << std::endl;
    }
    if (fan_control.has_current_method()) {
        out << "  Control method: " << fan_control.current_method() << std::endl;
    }
    if (fan_control.has_temp_source()) {
        out << "  Temp source: " << fan_control.temp_source();
        if (fan_control.has_temp_value()) {
            out << " at " << fan_control.temp_value() << " C";
        }
        out << std::endl;
    }
    for (const auto& method : fan_control.methods()) {
        out << "  Method " << method.name() << ":" << std::endl;
        switch (method.method_case()) {
            case FanControlMethodProto::kGenericMethod: {
                break;
            }
            case FanControlMethodProto::kNuvotonMethod: {
                out << method.nuvoton_method();
                break;
            }
            case FanControlMethodProto::METHOD_NOT_SET: {
                break;
            }
        }
    }
    return out;
}

void PrintSelectedSensors(const SensorsProto& sensors, const string& selected,
                          bool value_only, std::ostream& out) {
    for (const auto& sensor : StrSplit(selected, ',')) {
        const auto& parts = StrSplit(sensor, ':');
        if (parts.size() != 2) {
            LOG(ERROR) << "malformed sensor: " << sensor;
            continue;
        }

        bool found = false;
        if (parts[0] == "fan") {
            for (const auto& fan : sensors.fans()) {
                if (fan.name() == parts[1]) {
                    out << fan.speed().value() << endl;
                    found = true;
                }
            }
        } else if (parts[0] == "temp") {
            for (const auto& temp : sensors.temperatures()) {
                if (temp.name() == parts[1]) {
                    out << temp.value() << endl;
                    found = true;
                }
            }
        } else if (parts[0] == "volt") {
            for (const auto& volt : sensors.voltages()) {
                if (volt.name() == parts[1]) {
                    out << volt.value() << endl;
                    found = true;
                }
            }
        } else {
            LOG(ERROR) << "unknown sensor type: " << parts[0];
            continue;
        }
        if (!found) {
            LOG(ERROR) << "sensor not found: " << parts[1];
        }
    }
}

}  // namespace bsdsensors
