/* C0: instance and physical-device queries only. No VkDevice, queue or submission. */
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#define B(field) printf("\"" #field "\":%u,",f12.field)
int main(void) {
 uint32_t loader=VK_API_VERSION_1_0;
 if(vkEnumerateInstanceVersion) vkEnumerateInstanceVersion(&loader);
 VkApplicationInfo ai={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.pApplicationName="H2-C0-query-only",.apiVersion=VK_API_VERSION_1_2};
 VkInstanceCreateInfo ci={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&ai};
 VkInstance instance; VkResult rc=vkCreateInstance(&ci,NULL,&instance);
 printf("{\"loader_api\":%u,\"create_instance_status\":%d,\"devices\":[",loader,rc);
 if(rc!=VK_SUCCESS){puts("]}");return 1;}
 uint32_t n=0; vkEnumeratePhysicalDevices(instance,&n,NULL);
 VkPhysicalDevice *dev=calloc(n,sizeof(*dev)); vkEnumeratePhysicalDevices(instance,&n,dev);
 for(uint32_t i=0;i<n;i++) {
  VkPhysicalDeviceProperties p;vkGetPhysicalDeviceProperties(dev[i],&p);
  VkPhysicalDeviceVulkan12Features f12={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
  VkPhysicalDeviceVariablePointersFeatures vp={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES,.pNext=&f12};
  VkPhysicalDeviceFeatures2 f={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,.pNext=&vp};
  vkGetPhysicalDeviceFeatures2(dev[i],&f);
  if(i)printf(",");
  printf("{\"name\":\"%s\",\"vendor_id\":%u,\"device_id\":%u,\"device_type\":%u,\"api_version\":%u,\"driver_version_raw\":%u,",p.deviceName,p.vendorID,p.deviceID,p.deviceType,p.apiVersion,p.driverVersion);
  printf("\"maxComputeSharedMemorySize\":%u,\"maxComputeWorkGroupInvocations\":%u,\"maxImageDimension3D\":%u,\"maxPerStageDescriptorSampledImages\":%u,",p.limits.maxComputeSharedMemorySize,p.limits.maxComputeWorkGroupInvocations,p.limits.maxImageDimension3D,p.limits.maxPerStageDescriptorSampledImages);
  printf("\"features\":{");B(shaderInt8);B(bufferDeviceAddress);B(scalarBlockLayout);B(timelineSemaphore);B(vulkanMemoryModel);B(storageBuffer8BitAccess);
  printf("\"variablePointers\":%u,\"variablePointersStorageBuffer\":%u,\"shaderInt64\":%u,\"shaderFloat64\":%u},",vp.variablePointers,vp.variablePointersStorageBuffer,f.features.shaderInt64,f.features.shaderFloat64);
  uint32_t ne=0;vkEnumerateDeviceExtensionProperties(dev[i],NULL,&ne,NULL);VkExtensionProperties *ep=calloc(ne,sizeof(*ep));vkEnumerateDeviceExtensionProperties(dev[i],NULL,&ne,ep);
  printf("\"extensions\":{");for(uint32_t j=0;j<ne;j++)printf("%s\"%s\":%u",j?",":"",ep[j].extensionName,ep[j].specVersion);printf("}}");free(ep);
 }
 puts("]}");free(dev);vkDestroyInstance(instance,NULL);return 0;
}
