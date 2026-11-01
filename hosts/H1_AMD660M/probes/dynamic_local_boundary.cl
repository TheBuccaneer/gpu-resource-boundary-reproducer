__kernel void dynamic_local_boundary(__local int *scratch,
                                     uint last_index,
                                     __global int *out)
{
    if (get_global_id(0) == 0) {
        scratch[0] = 0x11111111;
        scratch[last_index] = 0x22222222;
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_global_id(0) == 0) {
        out[0] = scratch[0];
        out[1] = scratch[last_index];
    }
}
