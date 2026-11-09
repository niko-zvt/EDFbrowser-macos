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

#include <limits.h>
#include <stddef.h>


int edf_record_bytes(const int *samples_per_record, int nsignals, int bytes_per_sample, int limit, int *out_bytes)
{
  long long sum = 0;
  long long bytes;
  int i;

  if((samples_per_record == NULL) || (out_bytes == NULL) || (nsignals < 1) || (bytes_per_sample < 1) || (limit < 1))
  {
    return EDF_CHECK_RANGE;
  }

  for(i = 0; i < nsignals; i++)
  {
    if(samples_per_record[i] < 1)
    {
      return EDF_CHECK_RANGE;
    }

    if(sum > ((long long)INT_MAX - samples_per_record[i]))
    {
      return EDF_CHECK_OVERFLOW;
    }

    sum += samples_per_record[i];
  }

  if(sum > ((long long)INT_MAX / bytes_per_sample))
  {
    return EDF_CHECK_OVERFLOW;
  }

  bytes = sum * (long long)bytes_per_sample;

  if(bytes > limit)
  {
    return EDF_CHECK_LIMIT;
  }

  *out_bytes = (int)bytes;

  return EDF_CHECK_OK;
}


int edf_physical_scale(double phys_min, double phys_max, int dig_min, int dig_max, double *sense, double *offset)
{
  double span;

  if((sense == NULL) || (offset == NULL))
  {
    return EDF_CHECK_RANGE;
  }

  if(dig_max <= dig_min)
  {
    return EDF_CHECK_RANGE;
  }

  if(phys_max == phys_min)
  {
    return EDF_CHECK_RANGE;
  }

  span = (double)dig_max - (double)dig_min;
  *sense = (phys_max - phys_min) / span;

  if(*sense == 0.0)
  {
    return EDF_CHECK_RANGE;
  }

  *offset = (phys_max / *sense) - (double)dig_max;

  return EDF_CHECK_OK;
}


int edf_annotation_channel_fits(int stored_count, int capacity)
{
  if((stored_count < 0) || (capacity < 1))
  {
    return EDF_CHECK_RANGE;
  }

  if(stored_count >= capacity)
  {
    return EDF_CHECK_LIMIT;
  }

  return EDF_CHECK_OK;
}
