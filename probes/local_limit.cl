#ifndef LOCAL_INTS
#define LOCAL_INTS 1
#endif

__kernel void local_limit(__global int *out)
{
    __local int scratch[LOCAL_INTS];

    size_t idx = get_global_id(0) % LOCAL_INTS;

    scratch[idx] = (int)idx;

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_global_id(0) == 0)
        out[0] = scratch[idx];
}
