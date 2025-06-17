#include "utils.h"
#include <stdint.h>

uint16_t u16_sat_add(uint16_t a, uint16_t b){
  return (a > UINT16_MAX -b) ? UINT16_MAX : a + b;
}

uint16_t u16_sat_sub(uint16_t a, uint16_t b){
  return (a < b) ? 0: a - b;
}

uint32_t sat_add(uint32_t a, uint32_t b){
  return (a > UINT32_MAX -b) ? UINT32_MAX : a + b;
}

uint32_t sat_sub(uint32_t a, uint32_t b){
  return (a < b) ? 0: a - b;
}

