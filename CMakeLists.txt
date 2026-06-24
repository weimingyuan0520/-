# ============================================================
#  项目顶层 CMakeLists.txt — 智能光敏监控系统 (ESP32-S3)
# ============================================================
cmake_minimum_required(VERSION 3.16)

# 引入 ESP-IDF 构建系统
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# 声明工程名称
project(light_monitor)
