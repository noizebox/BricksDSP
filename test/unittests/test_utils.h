#include "gtest/gtest.h"

#include "dsp_brick.h"

#ifndef BRICKS_DSP_TEST_UTILS_H
#define BRICKS_DSP_TEST_UTILS_H

void fill_buffer(bricks::AudioBuffer& buffer, float value);

void assert_buffer(const bricks::AudioBuffer& buffer, float value);

#endif //BRICKS_DSP_TEST_UTILS_H
