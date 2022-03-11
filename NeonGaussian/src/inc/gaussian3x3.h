#ifndef GAUSSIAN3x3_H
#define GAUSSIAN3x3_H

#include <math.h>
#include <stdio.h>
#include "data_type.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t Gaussian3x3Sigma0NeonU8C1(const uint8_t *p_src, uint8_t *p_dst, int32_t heigh, int32_t width, int32_t istride, 
                                  int32_t ostride);

#ifdef __cplusplus
}
#endif

#endif //GAUSSIAN3x3_H
