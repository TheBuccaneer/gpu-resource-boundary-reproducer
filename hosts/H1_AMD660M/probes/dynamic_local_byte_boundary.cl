__kernel void dynamic_local_boundary(__local uchar *scratch,
                                     uint last_index,
                                     __global int *out)
{
    if (get_global_id(0) == 0) {
        scratch[0] = (uchar)0x11;
        scratch[last_index] = (uchar)0x22;
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_global_id(0) == 0) {
        out[0] = scratch[0];
        out[1] = scratch[last_index];
    }
}
