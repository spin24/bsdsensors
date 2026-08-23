enable_testing()

add_library(test_util SHARED fake_port_io.cc test_util.cc)
target_link_libraries(test_util bsdsensors)
add_executable(nuvoton_test nuvoton_test.cc test_main.cc)
target_link_libraries(nuvoton_test bsdsensors GTest::gtest test_util)

add_executable(disk_temp_test disk_temp_test.cc)
target_link_libraries(disk_temp_test bsdsensors GTest::gtest)

add_test(NAME disk_temp_test COMMAND disk_temp_test)

set(CHIPS w83627hgaw nct6796d w83667hg_p5q w83667hg_p6t_se nct6799d)
foreach(CHIP IN LISTS CHIPS)
    add_test(NAME ${CHIP}_test COMMAND nuvoton_test
        --testdata_path ${CMAKE_SOURCE_DIR}/testdata
        --testin_filename ${CHIP}_in.pb.txt
        --testout_sensors_filename ${CHIP}_sensors.pb.txt
    )
endforeach()
