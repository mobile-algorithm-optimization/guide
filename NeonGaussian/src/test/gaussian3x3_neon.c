#include "gaussian3x3.h"
#ifdef ANDROID
#include <arm_neon.h>
#endif

static inline int32_t Gaussian3x3RowCalcu(const uint8_t *src0, const uint8_t *src1,
                          const uint8_t *src2, uint8_t *dst, int32_t width)
{
    if ((NULL == src0) || (NULL == src1) || (NULL == src2) || (NULL == dst))
    {
        printf("input param invalid!\n");
        return -1;
    }

    int32_t col = 0;
    uint16x8_t vqn0, vqn1, vs_1, vs, vs1;
    uint8x8_t v_lnp;

    int32_t width_t = (width - 9) & (-8);

    uint8x8_t v_ld00 = vld1_u8(src0);
    uint8x8_t v_ld01 = vld1_u8(src0 + 8);
    uint8x8_t v_ld10 = vld1_u8(src1);
    uint8x8_t v_ld11 = vld1_u8(src1 + 8);
    uint8x8_t v_ld20 = vld1_u8(src2);
    uint8x8_t v_ld21 = vld1_u8(src2 + 8);
    uint8x8_t v_zero = vdup_n_u8(0);

    vqn0 = vaddl_u8(v_ld00, v_ld20);
    vqn0 = vaddq_u16(vqn0, vshll_n_u8(v_ld10, 1));
    vqn1 = vaddl_u8(v_ld01, v_ld21);
    vqn1 = vaddq_u16(vqn1, vshll_n_u8(v_ld11, 1));

    vs_1 = vextq_u16(vextq_u16(vqn0, vqn0, 2), vqn0, 7);
    vs1  = vextq_u16(vqn0, vqn1, 1);
    vs   = vaddq_u16(vaddq_u16(vqn0, vqn0), vaddq_u16(vs_1, vs1));

    v_lnp = vqrshrn_n_u16(vs, 4);
    vst1_u8(dst, v_lnp);
    vs_1 = vextq_u16(vqn0, vqn1, 7);

    for (col = 8; col < width_t; col += 8)
    {
        uint8x8_t v_ld0 = vld1_u8(src0 + col + 8);
        uint8x8_t v_ld1 = vld1_u8(src1 + col + 8);
        uint8x8_t v_ld2 = vld1_u8(src2 + col + 8);

        uint16x8_t vqn2 = vaddl_u8(v_ld0, v_ld2);
        vqn2 = vaddq_u16(vqn2, vshll_n_u8(v_ld1, 1));

        vs1 = vextq_u16(vqn1, vqn2, 1);
        uint16x8_t vtmp = vshlq_n_u16(vqn1, 1);

        uint16x8_t v_sum = vaddq_u16(vtmp, vaddq_u16(vs1, vs_1));
        uint8x8_t v_rst = vqrshrn_n_u16(v_sum, 4);
        vst1_u8(dst + col, v_rst);

        vs_1 = vextq_u16(vqn1, vqn2, 7);
        vqn1 = vqn2;
    }
    
    {
        uint8x8_t v_ld0 = v_zero, v_ld1 = v_zero, v_ld2 = v_zero;

        v_ld0 = vld1_lane_u8(src0 + col + 8, v_ld0, 0);
        v_ld1 = vld1_lane_u8(src1 + col + 8, v_ld1, 0);
        v_ld2 = vld1_lane_u8(src2 + col + 8, v_ld2, 0);

        uint16x8_t vqn2 = vaddl_u8(v_ld0, v_ld2);
        vqn2 = vaddq_u16(vqn2, vshll_n_u8(v_ld1, 1));

        vs1 = vextq_u16(vqn1, vqn2, 1);
        uint16x8_t vtmp = vshlq_n_u16(vqn1, 1);

        uint16x8_t v_sum = vaddq_u16(vtmp, vaddq_u16(vs1, vs_1));
        uint8x8_t v_rst = vqrshrn_n_u16(v_sum, 4);
        vst1_u8(dst + col, v_rst);
        col += 8;
    }

    for (; col < width; col++)
    {
        int32_t idx_l = (col == width - 1) ? width - 2 : col - 1;
        int32_t idx_r = (col == width - 1) ? width - 2 : col + 1;

        int32_t acc = 0;
        acc += (src0[idx_l] + src0[idx_r]);
        acc += (src0[col] << 1);

        acc += (src1[idx_l] + src1[idx_r]) << 1;
        acc += (src1[col] << 2);

        acc += (src2[idx_l] + src2[idx_r]);
        acc += (src2[col] << 1);

        uint16_t res = ((acc + (1 << 3)) >> 4) & 0xFFFF;
        dst[col] = CAST_U8(res);
    }

    return 0;
}

int32_t Gaussian3x3Sigma0NeonU8C1(const uint8_t *src, uint8_t *dst, int32_t height, int32_t width, int32_t istride,
                                 int32_t ostride)
{
    if ((NULL == src) || (NULL == dst))
    {
        printf("input param invalid!\n");
        return -1;
    }

    //reflect101
    const uint8_t *p_src0 = src + istride;
    const uint8_t *p_src1 = src;
    const uint8_t *p_src2 = src + istride;

    uint8_t *p_dst = dst;

    //process first row
    {
        Gaussian3x3RowCalcu(p_src0, p_src1, p_src2, p_dst, width);
    }
    
    //process mid 
    for (int32_t row = 1; row < height - 1; row++)
    {
        p_src0 = src + (row - 1) * istride;
        p_src1 = src + (row - 0) * istride;
        p_src2 = src + (row + 1) * istride;
        p_dst  = dst + row * ostride;

        Gaussian3x3RowCalcu(p_src0, p_src1, p_src2, p_dst, width);
    }
    
    //process last row
    {
        p_src0 = src + (height - 2) * istride;
        p_src1 = src + (height - 1) * istride;
        p_src2 = src + (height - 2) * istride;
        p_dst  = dst + (height - 1) * ostride;

        Gaussian3x3RowCalcu(p_src0, p_src1, p_src2, p_dst, width);
    }

    return 0;
}
