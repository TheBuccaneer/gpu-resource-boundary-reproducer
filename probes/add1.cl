__kernel void add1(__global const int *in,
                   __global int *out)
{
    size_t i = get_global_id(0);
    out[i] = in[i] + 1;
}
