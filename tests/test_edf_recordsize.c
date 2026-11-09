/*
***************************************************************************
*
* Copyright (C) 2007 - 2026 Teunis van Beelen
* Copyright (C) 2026 Nikolai Zhivotenko
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, version 3 of the License.
*
***************************************************************************
*/


#include "edf_recordsize.h"

#include <assert.h>
#include <stdio.h>


static void test_two_channel_edf(void)
{
  int samples[2] = {256, 256};
  int bytes = 0;

  assert(edf_record_bytes(samples, 2, 2, EDF_MAX_RECORD_BYTES, &bytes) == EDF_CHECK_OK);
  assert(bytes == 1024);
}


static void test_sample_sum_does_not_fit_in_int(void)
{
  int samples[22];
  int bytes = -1;
  int i;

  for(i = 0; i < 22; i++)
  {
    samples[i] = 100000000;
  }

  assert(edf_record_bytes(samples, 22, 2, EDF_MAX_RECORD_BYTES, &bytes) == EDF_CHECK_OVERFLOW);
  assert(edf_record_bytes(samples, 22, 3, BDF_MAX_RECORD_BYTES, &bytes) == EDF_CHECK_OVERFLOW);
  assert(bytes == -1);
}


static void test_record_size_limits(void)
{
  int exact = 5242880;
  int over = 5242881;
  int bytes = 0;

  assert(edf_record_bytes(&exact, 1, 2, EDF_MAX_RECORD_BYTES, &bytes) == EDF_CHECK_OK);
  assert(bytes == EDF_MAX_RECORD_BYTES);
  assert(edf_record_bytes(&over, 1, 2, EDF_MAX_RECORD_BYTES, &bytes) == EDF_CHECK_LIMIT);

  bytes = 0;
  assert(edf_record_bytes(&exact, 1, 3, BDF_MAX_RECORD_BYTES, &bytes) == EDF_CHECK_OK);
  assert(bytes == BDF_MAX_RECORD_BYTES);
  assert(edf_record_bytes(&over, 1, 3, BDF_MAX_RECORD_BYTES, &bytes) == EDF_CHECK_LIMIT);
}


static void test_zero_samples_and_equal_digital_limits(void)
{
  int zero = 0;
  int bytes = 0;
  double sense = 1.0;
  double offset = 1.0;

  assert(edf_record_bytes(&zero, 1, 2, EDF_MAX_RECORD_BYTES, &bytes) == EDF_CHECK_RANGE);
  assert(bytes == 0);
  assert(edf_physical_scale(-100.0, 100.0, 0, 0, &sense, &offset) == EDF_CHECK_RANGE);
  assert(sense == 1.0);
  assert(offset == 1.0);
  assert(edf_physical_scale(-100.0, 100.0, -32768, 32767, &sense, &offset) == EDF_CHECK_OK);
}


static void test_annotation_channel_capacity(void)
{
  assert(edf_annotation_channel_fits(255, EDF_ANNOTATION_CHANNEL_CAPACITY) == EDF_CHECK_OK);
  assert(edf_annotation_channel_fits(256, EDF_ANNOTATION_CHANNEL_CAPACITY) == EDF_CHECK_LIMIT);
}


int main(void)
{
  test_two_channel_edf();
  test_sample_sum_does_not_fit_in_int();
  test_record_size_limits();
  test_zero_samples_and_equal_digital_limits();
  test_annotation_channel_capacity();
  printf("edf_recordsize tests passed\n");
  return 0;
}
