#include <assert.h>
#include <stdio.h>
#include "b12_validation.h"
static b12_list list(uint32_t a,uint32_t b,size_t n) {
 b12_list x={0};x.size=x.returned_size=n*4;x.values[0]=a;x.values[1]=b;return x;
}
int main(void) {
 uint32_t allowed[]={1,0x2055,0x2058};size_t tests=0;
 b12_list empty=list(0,0,0),binary=list(1,0,1),fd=list(0x2055,0,1),two=list(0x2055,0x2058,2);
 assert(b12_valid(&empty,allowed,3));tests++;
 assert(b12_valid(&binary,allowed,3));tests++;
 assert(b12_valid(&two,allowed,3));tests++;
 b12_list x=binary;x.size_status=-30;assert(!b12_valid(&x,allowed,3));tests++;
 x=binary;x.value_status=-30;assert(!b12_valid(&x,allowed,3));tests++;
 x=binary;x.size=3;assert(!b12_valid(&x,allowed,3));tests++;
 x=binary;x.returned_size=0;assert(!b12_valid(&x,allowed,3));tests++;
 x=binary;x.size=x.returned_size=B12_MAX_BYTES+4;assert(!b12_valid(&x,allowed,3));tests++;
 x=two;x.values[1]=x.values[0];assert(!b12_valid(&x,allowed,3));tests++;
 x=binary;x.values[0]=0xdeadbeef;assert(!b12_valid(&x,allowed,3));tests++;
 assert(!b12_valid(&fd,allowed,1));tests++; /* missing advertised handle subtype */
 assert(b12_intersection(&fd,&fd,1));tests++;
 assert(!b12_intersection(&empty,&fd,1));tests++; /* prior missing platform-list validation */
 b12_list devices[]={two,fd};assert(b12_intersection(&fd,devices,2));tests++;
 assert(!b12_intersection(&two,devices,2));tests++; /* union is not intersection */
 devices[1]=empty;assert(b12_intersection(&empty,devices,2));tests++;
 assert(!b12_intersection(&fd,devices,2));tests++;
 assert(!b12_intersection(&empty,devices,0));tests++;
 assert(b12_device_directions(&fd,&empty));tests++;
 assert(b12_device_directions(&empty,&fd));tests++;
 assert(!b12_device_directions(&empty,&empty));tests++;
 x=two;x.values[0]=0x2058;x.values[1]=0x2055;devices[0]=two;devices[1]=x;
 assert(b12_intersection(&x,devices,2));tests++; /* no assumed list order */
 printf("PASS: %zu pure-CPU B12 validation assertions; no OpenCL linkage.\n",tests);
}
