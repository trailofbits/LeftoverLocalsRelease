#include <array>
#include <cstdint>
#include <fstream>
#include <set>
#include <stdarg.h>
#include <vector>

#include <vulkan/vulkan.h>
#ifdef __ANDROID__
	#include <android/log.h>
#endif

namespace easyvk {

	const uint32_t push_constant_size_bytes = 20;

	class Device;
	class Buffer;

	class Instance {
		public:
			Instance(bool = false);
			std::vector<VkPhysicalDevice> physicalDevices();
			void teardown();
		private:
			bool enableValidationLayers;
			VkInstance instance;
			VkDebugReportCallbackEXT debugReportCallback;
	};

	class Device {
		public:
			Device(Instance &_instance, VkPhysicalDevice _physicalDevice);
			VkDevice device;
			VkPhysicalDeviceProperties properties;
			uint32_t selectMemory(VkBuffer buffer, VkMemoryPropertyFlags flags);
			VkQueue computeQueue();
			uint32_t computeFamilyId = uint32_t(-1);
			void teardown();
		private:
			Instance &instance;
			VkPhysicalDevice physicalDevice;
	};

	class Buffer {
		public:
			Buffer(Device &device, uint32_t size);
			VkBuffer buffer;

			void store(size_t i, uint32_t value) {
				*(data + i) = value;
			}

			uint32_t load(size_t i) {
				return *(data + i);
			}
			void clear() {
				for (uint32_t i = 0; i < size; i++)
					store(i, 0);
			}

			void teardown();
		private:
			easyvk::Device &device;
			VkDeviceMemory memory;
			uint32_t size;
            uint32_t* data;
	};

	class Program {
		public:
			Program(Device &_device, const char* filepath, std::vector<easyvk::Buffer> &buffers);
			Program(Device &_device, std::vector<uint32_t> spvCode, std::vector<easyvk::Buffer> &buffers);
			void initialize(const char* entry_point);
			void run();
			float runWithDispatchTiming();
			void setWorkgroups(uint32_t _numWorkgroups);
			void setWorkgroupSize(uint32_t _workgroupSize);
			void teardown();
		private:
			std::vector<easyvk::Buffer> &buffers;
			VkShaderModule shaderModule;
			easyvk::Device &device;
			VkDescriptorSetLayout descriptorSetLayout;
			VkDescriptorPool descriptorPool;
			VkDescriptorSet descriptorSet;
			std::vector<VkWriteDescriptorSet> writeDescriptorSets;
			std::vector<VkDescriptorBufferInfo> bufferInfos;
			VkPipelineLayout pipelineLayout;
			VkPipeline pipeline;
			uint32_t numWorkgroups;
			uint32_t workgroupSize;
			VkFence fence;
			VkCommandBuffer commandBuffer;
			VkCommandPool commandPool;
			VkQueryPool timestampQueryPool;
	};

	const char* vkDeviceType(VkPhysicalDeviceType type);
}
