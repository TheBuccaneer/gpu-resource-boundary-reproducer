__kernel void dynamic_local(__local int *scratch,
                            __global int *out)
{
    if (get_local_id(0) == 0)
        scratch[0] = 0x12345678;

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_global_id(0) == 0)
        out[0] = scratch[0];
}
