/*
* Copyright (C) 2016 Advanced Micro Devices, Inc.
*
* Author: Peter Lohrmann <Peter.Lohrmann@amd.com>
*/
#pragma once
#include <unordered_map>
#include <list>
#include "vktrace_trace_packet_identifiers.h"
#include "vktrace_lib_trim_generate.h"
#include "vulkan.h"

// Trim support
// Indicates whether trim support will be utilized during this instance of vktrace.
// Only set once based on the VKTRACE_TRIM_FRAMES env var.
extern bool g_trimEnabled;
extern bool g_trimIsPreTrim;
extern bool g_trimIsInTrim;
extern bool g_trimIsPostTrim;
extern uint64_t g_trimFrameCounter;
extern uint64_t g_trimStartFrame;
extern uint64_t g_trimEndFrame;

namespace trim
{
    // Create / Destroy all image resources in the order performed by the application. 
    // Enabling this as a pre-processor macro so that we can compare performance and file size costs.
    // TRUE: Needed on AMD hardware
    // FALSE: Use normal object tracking to create only the necessary resources
#define TRIM_USE_ORDERED_IMAGE_CREATION TRUE

    void initialize();

    // Use this to snapshot the global state tracker at the start of the trim frames.
    void snapshot_state_tracker();

    // Outputs object-related trace packets to the trace file.
    void write_all_referenced_object_calls();
    void add_recorded_packet(vktrace_trace_packet_header* pHeader);
    void write_recorded_packets();
    void write_destroy_packets();
    void delete_all_packets();

    void add_CommandBuffer_call(VkCommandBuffer commandBuffer, vktrace_trace_packet_header* pHeader);
    void remove_CommandBuffer_calls(VkCommandBuffer commandBuffer);

    void reset_DescriptorPool(VkDescriptorPool descriptorPool);

    VkMemoryPropertyFlags LookUpMemoryProperties(VkDevice device, uint32_t memoryTypeIndex);

    // check if a memory type on the physical device is only DEVICE_LOCAL and not HOST_VISIBLE
    bool IsMemoryDeviceOnly(VkDevice device, VkDeviceMemory memory);

    VkImageAspectFlags getImageAspectFromFormat(VkFormat format);

#if TRIM_USE_ORDERED_IMAGE_CREATION
    void add_Image_call(vktrace_trace_packet_header* pHeader);
#endif //TRIM_USE_ORDERED_IMAGE_CREATION

    // Typically an application will have one VkAllocationCallbacks struct and 
    // will pass in that same address as needed, so we'll keep a map to correlate
    // the supplied address to the AllocationCallbacks object
    void add_Allocator(const VkAllocationCallbacks* pAllocator);
    VkAllocationCallbacks* get_Allocator(const VkAllocationCallbacks* pAllocator);

    // some of the items in this struct are based on what is tracked in the 'VkLayer_object_tracker' (struct _OBJTRACK_NODE).
    typedef struct _Trim_ObjectInfo
    {
        uint64_t vkObject;                               // object handle
        bool bReferencedInTrim;                          // True if the object was referenced during the trim frames
        VkInstance belongsToInstance;                    // owning Instance
        VkPhysicalDevice belongsToPhysicalDevice;        // owning PhysicalDevice
        VkDevice belongsToDevice;                        // owning Device
        union _ObjectInfo {                              // additional object-specific information
            struct _Instance {              // VkInstance
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
                vktrace_trace_packet_header* pEnumeratePhysicalDevicesCountPacket;
                vktrace_trace_packet_header* pEnumeratePhysicalDevicesPacket;
            } Instance;
            struct _PhysicalDevice {        // VkPhysicalDevice
                vktrace_trace_packet_header* pGetPhysicalDeviceSurfaceCapabilitiesKHRPacket;
                vktrace_trace_packet_header* pGetPhysicalDeviceSurfaceSupportKHRPacket;
                vktrace_trace_packet_header* pGetPhysicalDeviceMemoryPropertiesPacket;
                vktrace_trace_packet_header* pGetPhysicalDeviceQueueFamilyPropertiesCountPacket;
                vktrace_trace_packet_header* pGetPhysicalDeviceQueueFamilyPropertiesPacket;
                VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
            } PhysicalDevice;
            struct _SurfaceKHR {            // VkSurfaceKHR
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
            } SurfaceKHR;
            struct _Device {                // VkDevice
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
            } Device;
            struct _Queue {                 // VkQueue
                vktrace_trace_packet_header* pCreatePacket;
            } Queue;
            struct _CommandPool {           // VkCommandPool
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
                uint32_t numCommandBuffersAllocated[VK_COMMAND_BUFFER_LEVEL_RANGE_SIZE];
            } CommandPool;
            struct _SwapchainKHR {           // VkSwapchainKHR
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
                vktrace_trace_packet_header* pGetSwapchainImageCountPacket;
                vktrace_trace_packet_header* pGetSwapchainImagesPacket;
            } SwapchainKHR;
            struct _CommandBuffer {         // VkCommandBuffer
                VkCommandPool commandPool;
                VkCommandBufferLevel level;
            } CommandBuffer;
            struct _DeviceMemory {          // VkDeviceMemory
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
                VkDeviceSize size;
                uint32_t memoryTypeIndex;
                VkMemoryPropertyFlags propertyFlags;
                void* mappedAddress;
                VkDeviceSize mappedOffset;
                VkDeviceSize mappedSize;
                vktrace_trace_packet_header* pMapMemoryPacket;
                vktrace_trace_packet_header* pUnmapMemoryPacket;
                vktrace_trace_packet_header* pPersistentlyMapMemoryPacket;
            } DeviceMemory;
            struct _Image {                 // VkImage
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
                bool bIsSwapchainImage;
#if !TRIM_USE_ORDERED_IMAGE_CREATION
                vktrace_trace_packet_header* pGetImageMemoryRequirementsPacket;
                vktrace_trace_packet_header* pBindImageMemoryPacket;
#endif //!TRIM_USE_ORDERED_IMAGE_CREATION
                vktrace_trace_packet_header* pMapMemoryPacket;
                vktrace_trace_packet_header* pUnmapMemoryPacket;
                VkDeviceMemory memory;
                VkDeviceSize memoryOffset;
                VkDeviceSize memorySize;
                VkFormat format;
                VkExtent3D extent;
                uint32_t mipLevels;
                uint32_t arrayLayers;
                VkSharingMode sharingMode;
                uint32_t queueFamilyIndex;
                VkAccessFlags accessFlags;
                VkImageAspectFlags aspectMask;
                VkImageLayout initialLayout;
                VkImageLayout mostRecentLayout;
                bool needsStagingBuffer;
            } Image;
            struct _ImageView {             // VkImageView
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
            } ImageView;
            struct _Buffer {                // VkBuffer
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
                vktrace_trace_packet_header* pBindBufferMemoryPacket;
                vktrace_trace_packet_header* pMapMemoryPacket;
                vktrace_trace_packet_header* pUnmapMemoryPacket;
                uint32_t queueFamilyIndex;
                VkAccessFlags accessFlags;
                VkDeviceMemory memory;
                VkDeviceSize memoryOffset;
                VkDeviceSize size;
                bool needsStagingBuffer;
            } Buffer;
            struct _BufferView {            // VkBufferView
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
            } BufferView;
            struct _Sampler {               // VkSampler
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
            } Sampler;
            struct _DescriptorSetLayout {   // VkDescriptorSetLayout
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
                uint32_t numImages;
                uint32_t numBuffers;
                uint32_t numTexelBufferViews;
                uint32_t bindingCount;
                VkDescriptorSetLayoutBinding* pBindings;
            } DescriptorSetLayout;
            struct _PipelineLayout {        // VkPipelineLayout
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
            } PipelineLayout;
            struct _RenderPass {            // VkRenderPass
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
            } RenderPass;
            struct _ShaderModule {          // VkShaderModule
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
            } ShaderModule;
            struct _PipelineCache {         // VkPipelineCache
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
            } PipelineCache;
            struct _Pipeline {              // VkPipeline
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
                VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
                VkComputePipelineCreateInfo computePipelineCreateInfo;
                // TODO: Need to build out this structure based on VkGraphicsPipelineCreateInfo
            } Pipeline;
            struct _DescriptorPool {        // VkDescriptorPool
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
                VkDescriptorPoolCreateFlags createFlags;
                uint32_t maxSets;
                uint32_t numSets;
            } DescriptorPool;
            struct _DescriptorSet {         // VkDescriptorSet
                VkDescriptorPool descriptorPool;
                VkDescriptorSetLayout layout;
                uint32_t numBindings; // this is the number of elements allocated in each of the two arrays below.
                uint32_t writeDescriptorCount; // this is the number of descriptor sets that will need a write update.
                VkWriteDescriptorSet* pWriteDescriptorSets;
                uint32_t copyDescriptorCount;  // this is the number of descriptor sets that will need a copy update.
                VkCopyDescriptorSet* pCopyDescriptorSets;
            } DescriptorSet;
            struct _Framebuffer {           // VkFramebuffer
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
            } Framebuffer;
            struct _Semaphore {           // VkSemaphore
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
                VkQueue signaledOnQueue;
            } Semaphore;
            struct _Fence {           // VkFence
                const VkAllocationCallbacks* pAllocator;
                bool signaled;
            } Fence;
            struct _Event {           // VkEvent
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
            } Event;
            struct _QueryPool {           // VkQueryPool
                vktrace_trace_packet_header* pCreatePacket;
                const VkAllocationCallbacks* pAllocator;
                VkCommandBuffer commandBuffer;
                uint32_t size;
                bool* pResultsAvailable;
                VkQueryType queryType;
            } QueryPool;
        } ObjectInfo;
    } ObjectInfo;

#define TRIM_DECLARE_OBJECT_TRACKER_FUNCS(type) \
    ObjectInfo* add_##type##_object(Vk##type var); \
    void remove_##type##_object(Vk##type var); \
    ObjectInfo* get_##type##_objectInfo(Vk##type var);

    void mark_CommandBuffer_reference(VkCommandBuffer commandbuffer);

    typedef std::unordered_map<void*, ObjectInfo> TrimObjectInfoMap;

    typedef struct _Trim_StateTracker
    {
        TrimObjectInfoMap createdInstances;
        TrimObjectInfoMap createdPhysicalDevices;
        TrimObjectInfoMap createdDevices;
        TrimObjectInfoMap createdSurfaceKHRs;
        TrimObjectInfoMap createdCommandPools;
        TrimObjectInfoMap createdCommandBuffers;
        TrimObjectInfoMap createdDescriptorPools;
        TrimObjectInfoMap createdRenderPasss;
        TrimObjectInfoMap createdPipelineCaches;
        TrimObjectInfoMap createdPipelines;
        TrimObjectInfoMap createdQueues;
        TrimObjectInfoMap createdSemaphores;
        TrimObjectInfoMap createdDeviceMemorys;
        TrimObjectInfoMap createdFences;
        TrimObjectInfoMap createdSwapchainKHRs;
        TrimObjectInfoMap createdImages;
        TrimObjectInfoMap createdImageViews;
        TrimObjectInfoMap createdBuffers;
        TrimObjectInfoMap createdBufferViews;
        TrimObjectInfoMap createdFramebuffers;
        TrimObjectInfoMap createdEvents;
        TrimObjectInfoMap createdQueryPools;
        TrimObjectInfoMap createdShaderModules;
        TrimObjectInfoMap createdPipelineLayouts;
        TrimObjectInfoMap createdSamplers;
        TrimObjectInfoMap createdDescriptorSetLayouts;
        TrimObjectInfoMap createdDescriptorSets;
    } StateTracker;

    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(Instance);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(PhysicalDevice);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(Device);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(SurfaceKHR);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(CommandPool);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(CommandBuffer);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(DescriptorPool);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(RenderPass);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(PipelineCache);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(Pipeline);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(Queue);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(Semaphore);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(DeviceMemory);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(Fence);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(SwapchainKHR);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(Image);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(ImageView);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(Buffer);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(BufferView);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(Framebuffer);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(Event);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(QueryPool);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(ShaderModule);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(PipelineLayout);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(Sampler);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(DescriptorSetLayout);
    TRIM_DECLARE_OBJECT_TRACKER_FUNCS(DescriptorSet);

    void remove_Instance_object(VkInstance var);
    void remove_PhysicalDevice_object(VkPhysicalDevice var);
    void remove_Device_object(VkDevice var);
    void remove_SurfaceKHR_object(VkSurfaceKHR var);
    void remove_CommandPool_object(VkCommandPool var);
    void remove_CommandBuffer_object(VkCommandBuffer var);
    void remove_DescriptorPool_object(VkDescriptorPool var);
    void remove_RenderPass_object(VkRenderPass var);
    void remove_PipelineCache_object(VkPipelineCache var);
    void remove_Pipeline_object(VkPipeline var);
    void remove_Queue_object(VkQueue var);
    void remove_Semaphore_object(VkSemaphore var);
    void remove_DeviceMemory_object(VkDeviceMemory var);
    void remove_Fence_object(VkFence var);
    void remove_SwapchainKHR_object(VkSwapchainKHR var);
    void remove_Image_object(VkImage var);
    void remove_ImageView_object(VkImageView var);
    void remove_Buffer_object(VkBuffer var);
    void remove_BufferView_object(VkBufferView var);
    void remove_Framebuffer_object(VkFramebuffer var);
    void remove_Event_object(VkEvent var);
    void remove_QueryPool_object(VkQueryPool var);
    void remove_ShaderModule_object(VkShaderModule var);
    void remove_PipelineLayout_object(VkPipelineLayout var);
    void remove_Sampler_object(VkSampler var);
    void remove_DescriptorSetLayout_object(VkDescriptorSetLayout var);
    void remove_DescriptorSet_object(VkDescriptorSet var);
} // namespace trim
