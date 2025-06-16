#include "utils.h"
#include <stdint.h>

uint16_t u16_sat_add(uint16_t a, uint16_t b){
  return (a > UINT16_MAX -b) ? UINT16_MAX : a + b;
}

uint16_t u16_sat_sub(uint16_t a, uint16_t b){
  return (a < b) ? 0: a - b;
}

