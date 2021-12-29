__kernel void TransposeKernel(__global uchar *src, __global uchar *dst, int width, int height)
{
    uint g_idx = get_global_id(0);
    uint g_idy = get_global_id(1);
    if ((g_idx >= width) || (g_idy >= height))
        return;
    dst[g_idx * height + g_idy] = src[g_idy * width + g_idx];
}
