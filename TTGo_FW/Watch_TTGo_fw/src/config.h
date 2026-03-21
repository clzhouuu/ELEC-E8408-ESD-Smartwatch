#pragma once
#include <Arduino.h>

// => Hardware select
#define LILYGO_WATCH_2020_V2             //To use T-Watch2020 V2, please uncomment this line
//#define LILYGO_WATCH_2020_V3           //To use T-Watch2020 V2, please uncomment this line
#define LILYGO_WATCH_LVGL
#include <LilyGoWatch.h>

#include "BluetoothSerial.h"
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include "utils.h"

// Icons
#define MY_SYMBOL_STEPS  "\xEF\x95\x8B"
#define MY_SYMBOL_FIRE   "\xEF\x81\xAD"
#define MY_SYMBOL_PERSON "\xEF\x95\x94"

// Fonts
#define FONT_SMALL &lv_font_montserrat_14
#define FONT_MEDIUM &lv_font_montserrat_16
#define FONT_HUGE &lv_font_montserrat_32

LV_FONT_DECLARE(fire);
LV_FONT_DECLARE(steps);
LV_FONT_DECLARE(person);