#ifndef _NMEA_RANDOM_H
#define _NMEA_RANDOM_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define NMEA_RANDOM_MAX INT32_MAX

static __inline long int nmea_random(const double min, const double max) {
  int32_t value;
  int randomFile;
  double range = fabs(max - min);

#ifdef _WIN32
  value = random();
#else

//TODO: random generator

#endif /* _WIN32 */

  return min + ((abs(value) * range) / NMEA_RANDOM_MAX);
}

static __inline void nmea_init_random(void) {
  srandom(time(NULL));
}

#endif /* _NMEA_RANDOM_H */
