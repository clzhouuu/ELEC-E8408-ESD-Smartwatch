#pragma once
#include "config.h"

// metrics save to file
void saveIdToFile(uint16_t id);
void saveStepsToFile(uint32_t step_count);
void saveKcalToFile(uint32_t caloriesBurned);
void saveDistanceToFile(float distance);
void saveDateTimeToFile(const String &dur, const String &startTime, const String &startDate);
void deleteSession();