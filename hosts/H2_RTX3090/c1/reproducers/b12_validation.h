#ifndef B12_VALIDATION_H
#define B12_VALIDATION_H
/* Pure host-side validation. No OpenCL calls or dependency on the ICD loader. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define B12_MAX_BYTES 4096u
#define B12_MAX_VALUES (B12_MAX_BYTES / sizeof(uint32_t))
typedef struct {
 int size_status, value_status;
 size_t size, returned_size;
 uint32_t values[B12_MAX_VALUES];
} b12_list;
static inline int b12_valid(const b12_list *x,const uint32_t *allowed,size_t nallowed) {
 if(x->size_status!=0||x->value_status!=0||x->size>B12_MAX_BYTES||x->size%sizeof(uint32_t)||x->returned_size!=x->size)return 0;
 size_t n=x->size/sizeof(uint32_t);
 for(size_t i=0;i<n;i++) {
  int found=0;for(size_t j=0;j<nallowed;j++)if(x->values[i]==allowed[j])found=1;
  if(!found)return 0;
  for(size_t j=0;j<i;j++)if(x->values[j]==x->values[i])return 0;
 }
 return 1;
}
static inline int b12_has(const b12_list *x,uint32_t value) {
 for(size_t i=0;i<x->size/sizeof(uint32_t);i++)if(x->values[i]==value)return 1;
 return 0;
}
/* Inputs must first pass b12_valid. Set order is deliberately irrelevant. */
static inline int b12_intersection(const b12_list *platform,const b12_list *devices,size_t n) {
 if(!n)return 0;
 size_t expected=0;
 for(size_t i=0;i<devices[0].size/sizeof(uint32_t);i++) {
  uint32_t value=devices[0].values[i];int common=1;
  for(size_t d=1;d<n;d++)if(!b12_has(&devices[d],value))common=0;
  if(common){expected++;if(!b12_has(platform,value))return 0;}
 }
 return platform->size/sizeof(uint32_t)==expected;
}
static inline int b12_device_directions(const b12_list *imports,const b12_list *exports) {
 return imports->size>0||exports->size>0;
}
#endif
