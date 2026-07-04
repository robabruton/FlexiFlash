# FlexiFlash desktop firmware - ESP-IDF application project.
#
# Shares the component library under firmware/components/ with the handheld app;
# only this project's board profile, partition table, and variant-specific main
# wiring differ. Build or flash by running idf.py from this directory.
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../components")
set(SDKCONFIG_DEFAULTS
    "${CMAKE_CURRENT_LIST_DIR}/../../sdkconfig.defaults.common"
    "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(flexiflash_desktop)
