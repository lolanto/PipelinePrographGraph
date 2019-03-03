#include "ppfgEle.h"
#include <string>
#include <vector>
#include <map>

namespace PipelineProfilingGraph {

	using QueueIdx = uint32_t;
	using PassIdx = uint32_t;
	using ResourceIdx = uint32_t;
	const uint32_t INVALID_INDEX = UINT32_MAX; /**< 任何索引设置为该值都意味着无效 */


	/** 描述一个pass的位置 */
	struct PassLocate {
		QueueIdx queueIndex; /**< 该pass所在的queue的索引 */
		PassIdx inqueueIndex; /**< 该pass在queue中的索引 */
		bool operator==(const PassLocate& rhs) {
			return (queueIndex == rhs.queueIndex && inqueueIndex == rhs.inqueueIndex);
		}
		bool operator!=(const PassLocate& rhs) {
			return !(this->operator==(rhs));
		}
	};
	/** 无效的pass位置 */
	const PassLocate INVALID_PASS_LOCATE = { INVALID_INDEX, INVALID_INDEX };

	using FenceSignalPasses = std::vector<PassLocate>;

	struct Pass {
		Pass(const char* n, QueueIdx queIdx, 
			PassIdx inqueueIdx, const FenceSignalPasses& dep)
			: name(n), locate({ queIdx, inqueueIdx }),
			depPasses(dep), processed(false) {}
		Pass(const char* n, QueueIdx queIdx,
			PassIdx inqueueIdx, FenceSignalPasses&& dep)
			: name(n), locate({ queIdx, inqueueIdx }),
			processed(false) {
			depPasses.swap(dep);
		}

		std::string name; /**< 该pass的名称 */
		PassLocate locate; /**< 该pass的位置 */
		FenceSignalPasses depPasses; /**< 该pass强依赖的(fence)的pass的位置 */
		bool processed; /**< 该pass是否已经被处理过 */
	};

	struct Barrier {
		enum Flag : uint8_t {
			TRANSITION_BARRIER = 0x01U,
			ALIASING_BARRIER = 0x02U,
			UAV_BARRIER = 0x04U,
			IMMEDIACY = 0x08U,
			BEGIN = 0x10U,
			END = 0x20U
		};
		Barrier() = default;
		Barrier(PassLocate pass, const char* desc, uint8_t flags)
			: submitPass(pass), description(desc), flags(flags) {}
		PassLocate submitPass; /**< 提交该指令的pass */
		std::string description; /**< 状态切换的描述 */
		uint8_t flags; /**< 当前barrier的状态，由Flag通过or操作设置 */
	};

	struct Resource {
		Resource() {}
		Resource(const char* name, PassLocate firstCreatePass, PassLocate lastDestroyPass,
			const std::vector<PassLocate>& readPasses,
			const std::vector<PassLocate>& writePasses)
			:name(name), firstCreate(firstCreatePass), lastDestroy(lastDestroyPass),
			readPasses(readPasses), writedPasses(writedPasses) {}
		Resource(const char* name, PassLocate firstCreatePass, PassLocate lastDestroyPass,
			std::vector<PassLocate>&& rp,
			std::vector<PassLocate>&& wp)
			:name(name), firstCreate(firstCreatePass), lastDestroy(lastDestroyPass) {
			readPasses.swap(rp);
			writedPasses.swap(wp);
		}
		std::string name; /**< 资源的名称 */
		PassLocate firstCreate; /**< 第一个创建该资源的pass，若为INVALID_PASS_LOCATE，表示该资源一直被创建 */
		PassLocate lastDestroy; /**< 第一个删除该资源的pass，若为INVALID_PASS_LOCATE, 表示该资源一直未被删除*/
		std::vector<PassLocate> readPasses; /**< 读取该资源的pass */
		std::vector<PassLocate> writedPasses; /**< 写入该资源的pass */
		std::vector<Barrier> barriers; /**< 资源使用到的barrier，需要额外手动设置 */
	};

	using Queue = std::vector<Pass>;

	class PipelineGraph {
	public:
		/** Pipeline分析图的构造函数
		 * @param passMap 记录渲染图中所有pass以及pass的依赖关系
		 * @param resMap 记录渲染图中用到的所有的资源以及其读写关系
		 * @remark 传入的passMap以及resMap都会在函数调用后被替换成空的容器 */
		PipelineGraph(std::vector<Queue>& passMap,
			std::vector<Resource>& resMap) {
			m_passMap.swap(passMap);
			m_resourceMap.swap(resMap);
		}
		/** Pipeline分析图的构造函数
		 * @param passMap 记录渲染图中所有pass以及pass的依赖关系
		 * @param resMap 记录渲染图中用到的所有的资源以及其读写关系
		 * @remark 传入的passMap以及resMap都会在函数调用后被替换成空的容器 */
		PipelineGraph(std::vector<Queue>&& passMap,
			std::vector<Resource>& resMap) {
			m_passMap.swap(passMap);
			m_resourceMap.swap(resMap);
		}
		/** 该函数将分析好的图输出到文件中
		 * @param name 输出的图的名称 
		 * @remark 调用该函数前，必须保证setup被调用*/
		void Raster(const char* name = nullptr);
		/** 该函数根据输入的pass和资源情况，设置图元素 */
		void Setup();
	private:
		/** 计算某个pass的矩形形状，并返回该pass的Rectangle
		 * @param pass 当前需要处理的pass
		 * @return 该pass初步处理后的Rectangle
		 * @remark 初步处理是指只有长宽，以及x坐标，y坐标无效 */
		Rectangle processPassHelper(Pass& pass);
		/** 计算某个queue的矩形形状
		 * @param queIdx 需要处理的queue的索引
		 * @return 返回当前queue的宽度
		 * @remark 调用前必须确保该queue所有的pass已经初步处理完毕 */
		float processQueueHelper(QueueIdx queIdx);
		/** 计算某个Fence的箭头指向 
		 * @param singal 负责更新fence的pass的位置
		 * @param receiver 负责接收fence的pass的位置
		 * @remark 调用前必须确保pass和queue被更新完*/
		void processFenceHelper(const PassLocate& signal,
			const PassLocate& receiver);
		/** 计算某个resource的矩形形状
		 * @param resIdx 需要处理的resource在m_resourceMap中的索引
		 * @remark 调用前必须保证所有的queue被处理完成*/
		void processResourceHelper(ResourceIdx resIdx);
	private:
		std::vector<Queue> m_passMap; /**< 存储渲染图中所有的pass */
		std::vector<Resource> m_resourceMap; /**< 存储渲染图中用到的所有元素 */
		std::vector<Rectangle> m_queues; /**< 存储图的queue图形元素设置 */
		std::vector< std::vector<Rectangle> > m_queuePasses; /**< 存储渲染图中所有pass的图形元素设置 */
		std::vector<Rectangle> m_resources; /**< 存储图中所有资源的图形元素设置 */
		std::vector<Transition> m_transts; /**< 存储图中所有barrier的图形元素设置 */
		std::vector<Arrow> m_arrows; /**< 存储途中所有箭头(详看箭头类型设置)的图形元素设置 */
	};


}
