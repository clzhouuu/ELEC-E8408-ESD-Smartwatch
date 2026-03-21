#pragma once
#include "config.h"

void sendDataBT(fs::FS &fs, const char *path);
void sendSessionBT();
void BTsync();
void receiveBTConfig();