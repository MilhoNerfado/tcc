#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

set(BOARD heltec_wifi_lora32_v2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(controller LANGUAGES C)

target_sources(app PRIVATE src/main.c)
