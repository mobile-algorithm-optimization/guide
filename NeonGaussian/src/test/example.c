#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "data_type.h"
#include "gaussian3x3.h"

static void GetTime(double *time)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    (*time) = (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

static int32_t CompareResult(uint8_t *pt0, uint8_t *pt1, uint32_t stride, uint32_t width, uint32_t height)
{
    int32_t ret = 0;
	
    for (uint32_t i = 0; i < height; i++)
    {
        uint8_t *pt_tmp0 = pt0 + i * stride;
        uint8_t *pt_tmp1 = pt1 + i * stride;
        for (uint32_t j = 0; j < width; j++)
        {
            if (pt_tmp0[j] != pt_tmp1[j])
            {
                ret = -1;
                return ret;
            }
        }
    }
    return ret;
}

int32_t Gaussian3x3Sigma0NoneU8C1(const uint8_t *src, int32_t height, int32_t width, int32_t istride,
                                 int32_t ostride, uint8_t *dst)
{
    if ((NULL == src) || (NULL == dst))
    {
        printf("input param invalid!\n");
        return -1;
    }

    //reflect101
    const uint8_t * __restrict p_src0 = src + istride;
    const uint8_t * __restrict p_src1 = src;
    const uint8_t * __restrict p_src2 = src + istride;

    uint8_t *__restrict p_dst = dst;

    uint32_t acc = 0;
    uint16_t res = 0;

    //process first row
    for (int32_t col = 0; col < width; col++)
    {
        int32_t idx_l = (col == 0) ? 1 : ((col == width - 1) ? width - 2 : col - 1);
        int32_t idx_r = (col == 0) ? 1 : ((col == width - 1) ? width - 2 : col + 1);

        acc = 0;
        acc += (p_src0[idx_l] + p_src0[idx_r]);
        acc += (p_src0[col] * 2);

        acc += (p_src1[idx_l] + p_src1[idx_r]) * 2;
        acc += (p_src1[col] * 4);

        acc += (p_src2[idx_l] + p_src2[idx_r]);
        acc += (p_src2[col] * 2);

        res = ((acc + (1 << 3)) >> 4) & 0xFFFF;
        p_dst[col] = CAST_U8(res);
    }

    p_src0 = src + (height - 2) * istride;
    p_src1 = src + (height - 1) * istride;
    p_src2 = src + (height - 2) * istride;
    p_dst  = dst + (height - 1) * ostride;
    //process last row
    for (int32_t col = 0; col < width; col++)
    {
        int32_t idx_l = (col == 0) ? 1 : ((col == width - 1) ? width - 2 : col - 1);
        int32_t idx_r = (col == 0) ? 1 : ((col == width - 1) ? width - 2 : col + 1);

        acc = 0;
        acc += (p_src0[idx_l] + p_src0[idx_r]);
        acc += (p_src0[col] * 2);

        acc += (p_src1[idx_l] + p_src1[idx_r]) * 2;
        acc += (p_src1[col] * 4);

        acc += (p_src2[idx_l] + p_src2[idx_r]);
        acc += (p_src2[col] * 2);

        res = ((acc + (1 << 3)) >> 4) & 0xFFFF;
        p_dst[col] = CAST_U8(res);
    }

    //process body
    for (int32_t row = 1; row < height - 1; row++)
    {
        p_src0 = src + (row - 1) * istride;
        p_src1 = src + (row - 0) * istride;
        p_src2 = src + (row + 1) * istride;

        p_dst  = dst + row * ostride;

        for (int32_t col = 1; col < width - 1; col++)
        {
            acc = 0;
            acc += (p_src0[col - 1] + p_src0[col + 1]);
            acc += (p_src0[col - 0] * 2);

            acc += (p_src1[col - 1] + p_src1[col + 1])* 2;
            acc += (p_src1[col - 0] * 4);

            acc += (p_src2[col - 1] + p_src2[col + 1]);
            acc += (p_src2[col - 0] * 2);

            res = ((acc + (1 << 3)) >> 4) & 0xFFFF;
            p_dst[col] = CAST_U8(res);
        }

        //process first col reflect101
        {
            acc = 0;
            acc += (p_src0[1] + p_src0[1]);
            acc += (p_src0[0] * 2);

            acc += (p_src1[1] + p_src1[1]) * 2;
            acc += (p_src1[0] * 4);

            acc += (p_src2[1] + p_src2[1]);
            acc += (p_src2[0] * 2);

            res = ((acc + (1 << 3)) >> 4) & 0xFFFF;
            p_dst[0] = CAST_U8(res);
        }

        //process last col reflect101
        {
            int32_t idx = width - 1;
            acc = 0;
            acc += (p_src0[idx - 1] + p_src0[idx - 1]);
            acc += (p_src0[idx + 0] * 2);

            acc += (p_src1[idx - 1] + p_src1[idx - 1]) * 2;
            acc += (p_src1[idx + 0] * 4);

            acc += (p_src2[idx - 1] + p_src2[idx - 1]);
            acc += (p_src2[idx + 0] * 2);

            res = ((acc + (1 << 3)) >> 4) & 0xFFFF;
            p_dst[idx] = CAST_U8(res);
        }

    }
    
    return 0;
}

int32_t main()
{
    int32_t width  = 4095;
    int32_t height = 2161;
    int32_t ret = 0;
    FILE *fp = NULL;
    double c_time, t1, t2;
    int32_t run_cnt = 100;

    uint8_t *p_src      = (uint8_t *)malloc(width * height * sizeof(uint8_t));
    uint8_t *p_dst_ref  = (uint8_t *)malloc(width * height * sizeof(uint8_t));
    uint8_t *p_dst_neon = (uint8_t *)malloc(width * height * sizeof(uint8_t));

    if ((NULL == p_src) || (NULL == p_dst_ref) || (NULL == p_dst_neon))
    {
        printf("malloc failed\n");
        if (p_src)
        {
            free(p_src);
            p_src = NULL;
        }
        if (p_dst_ref)
        {
            free(p_dst_ref);
            p_dst_ref = NULL;
        }
        if (p_dst_neon)
        {
            free(p_dst_neon);
            p_dst_neon = NULL;
        }
    }

    fp = fopen("./data/gray_4095x2161.raw", "rb");
    if (NULL == fp)
    {
        printf("fopen gray_4095x2161.raw failed\n");
        goto EXIT;
    }
    fread(p_src, sizeof(uint8_t), height * width, fp);
    fclose(fp);

    //none c test
    c_time = 0.0f;
    for (int32_t i = 0; i < run_cnt; i++)
    {
        GetTime(&t1);
        ret |= Gaussian3x3Sigma0NoneU8C1(p_src, height, width, width, width, p_dst_ref);
        
        if (ret != 0)
        {
            printf("Gaussian3x3Sigma0NoneU8C1 failed\n");
            goto EXIT;
        }
        GetTime(&t2);
        c_time += (t2 - t1);
    }
    printf("Gaussian3x3 None average time = %f \n", c_time / run_cnt);

    c_time = 0.0f;
    for (int32_t i = 0; i < run_cnt; i++)
    {
        GetTime(&t1);
        ret |= Gaussian3x3Sigma0NeonU8C1(p_src, p_dst_neon, height, width, width, width);
        
        if (ret != 0)
        {
            printf("Gaussian3x3Sigma0NeonU8C1 failed\n");
            goto EXIT;
        }
        GetTime(&t2);
        c_time += (t2 - t1);
    }
    printf("Gaussian3x3 Neon average time = %f \n", c_time / run_cnt);

    if (!CompareResult(p_dst_ref, p_dst_neon, width, width, height))
    {
        printf("Gaussian3x3None and Gaussian3x3Neon result bit matched\n");
    }
    else
    {
        printf("Gaussian3x3None and Gaussian3x3Neon result not matched\n");
    }
EXIT:
    free(p_src);
    free(p_dst_ref);
    free(p_dst_neon);
    p_src = NULL;
    p_dst_ref = NULL;
    p_dst_neon = NULL;
    printf("gausaian_testcase done! \n");
    return 0;
}
