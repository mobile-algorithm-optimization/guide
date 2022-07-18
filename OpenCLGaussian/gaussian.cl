
__kernel void Gauss3x3u8c1Buffer(__global uchar *src, int row, int col,
                                 int src_pitch, int dst_pitch,
                                 __global uchar *dst)
{
    int x = get_global_id(0) << 2;
    int y = get_global_id(1);

    if ( x >= col || y >= row)
    {
        return;
    }

    int r1_index = mad24(y, src_pitch, x);
    int r0_index = select(mad24(y - 1, src_pitch, x), mad24(y + 1, src_pitch, x), ((y - 1) < 0));
    int r2_index = select(r1_index - src_pitch, r1_index + src_pitch, ((y + 1) < row));

    int8 r0 = convert_int8(vload8(0, src + r0_index));
    int8 r1 = convert_int8(vload8(0, src + r1_index));
    int8 r2 = convert_int8(vload8(0, src + r2_index));

    int8 vert_sum = (r0 + r2) + (r1  << (int8)(1));
    int4 v_hori_s0 = vert_sum.lo;
    int4 v_hori_s1 = (int4)(vert_sum.s1234);
    int4 v_hori_s2 = (int4)(vert_sum.s2345);
    int4 v_res = (v_hori_s0 + v_hori_s2 + (v_hori_s1 << (int4)(1)) + (int4)(1 << 3)) >> (int4)(4);
    uchar4 v_dst = convert_uchar4_sat(v_res);

    int dst_index = mad24(y, dst_pitch, x + 1);
    vstore4(v_dst, 0, dst + dst_index);
}
