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


#ifndef EDF_RECORDSIZE_H
#define EDF_RECORDSIZE_H


#define EDF_MAX_RECORD_BYTES              (10 * 1024 * 1024)
#define BDF_MAX_RECORD_BYTES              (15 * 1024 * 1024)
#define EDF_ANNOTATION_CHANNEL_CAPACITY   (256)

#define EDF_CHECK_OK                      (0)
#define EDF_CHECK_RANGE                   (1)
#define EDF_CHECK_OVERFLOW                (2)
#define EDF_CHECK_LIMIT                   (3)


#ifdef __cplusplus
extern "C" {
#endif

/* Byte size of one data record. Rejects a sample count below 1, a sample
   sum that does not fit in a signed 32-bit int, and a byte size above limit. */
int edf_record_bytes(const int *samples_per_record, int nsignals, int bytes_per_sample, int limit, int *out_bytes);

/* Physical scale. Rejects equal or inverted digital limits before dividing. */
int edf_physical_scale(double phys_min, double phys_max, int dig_min, int dig_max, double *sense, double *offset);

/* stored_count is how many annotation channels are already stored. */
int edf_annotation_channel_fits(int stored_count, int capacity);

#ifdef __cplusplus
}
#endif


#endif
