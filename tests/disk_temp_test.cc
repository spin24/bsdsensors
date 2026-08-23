/*
 * disk_temp_test.cc
 * Copyright (C) 2026 spin24
 *
 * Distributed under terms of the 3-clause BSD license.
 */

#include "disk_temp.h"

#include <gtest/gtest.h>

namespace bsdsensors {

TEST(DiskTempTest, ParsesRawValueNotNormalized) {    // WD Ultrastar He12: normalized VALUE is 44 but RAW_VALUE is 37.
    const std::string text =
        "ID# ATTRIBUTE_NAME          FLAG     VALUE WORST THRESH TYPE      "
        "UPDATED  WHEN_FAILED RAW_VALUE\n"
        "194 Temperature_Celsius     0x0002   044   044   000    Old_age   "
        "Always       -       37 (Min/Max 21/59)\n";
    double temp = 0;
    ASSERT_TRUE(ParseSmartTemperature(text, &temp));
    EXPECT_DOUBLE_EQ(temp, 37.0);
}

TEST(DiskTempTest, PrefersAttr194Over190) {
    const std::string text =
        "190 Airflow_Temperature_Cel 0x0022   100   100   000    Old_age   "
        "Always       -       31\n"
        "194 Temperature_Celsius     0x0002   043   043   000    Old_age   "
        "Always       -       37 (Min/Max 21/59)\n";
    double temp = 0;
    ASSERT_TRUE(ParseSmartTemperature(text, &temp));
    EXPECT_DOUBLE_EQ(temp, 37.0);
}

TEST(DiskTempTest, FallsBackToAirflowAttr) {
    const std::string text =
        "190 Airflow_Temperature_Cel 0x0022   100   100   000    Old_age   "
        "Always       -       31\n";
    double temp = 0;
    ASSERT_TRUE(ParseSmartTemperature(text, &temp));
    EXPECT_DOUBLE_EQ(temp, 31.0);
}

TEST(DiskTempTest, ParsesNvme) {
    const std::string text = "Temperature: 38 Celsius\n";
    double temp = 0;
    ASSERT_TRUE(ParseSmartTemperature(text, &temp));
    EXPECT_DOUBLE_EQ(temp, 38.0);
}

TEST(DiskTempTest, RejectsGarbage) {
    double temp = 0;
    EXPECT_FALSE(ParseSmartTemperature("hello world", &temp));
}

}  // namespace bsdsensors

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
