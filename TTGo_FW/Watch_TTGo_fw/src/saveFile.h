#pragma once
#include "config.h"

void saveIdToFile(uint16_t id);
void saveStepsToFile(uint32_t step_count);
void saveDistanceToFile(float distance);
void saveDateTimeToFile(const String &dur, const String &startDate, const String &startTime);
void deleteSession();