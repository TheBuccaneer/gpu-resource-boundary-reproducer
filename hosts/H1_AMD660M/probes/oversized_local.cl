__kernel void oversized_local(__global int *out)
{
    __local int scratch[20000];

    if (get_local_id(0) == 0)
        scratch[0] = 123;

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_global_id(0) == 0)
        out[0] = scratch[0];
}
