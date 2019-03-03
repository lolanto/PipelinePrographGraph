#include "ppfg.h"
#include "svgProcess.h"

const float SVGBase::STROKE_WIDTH = 0.8f;

namespace PipelineProfilingGraph {

	inline float centerOffset(float outside, float inside) {
		return (outside - inside) / 2.0f;
	}

	Rectangle PipelineGraph::processPassHelper(Pass& pass) {
		if (pass.processed) {
			/** 假如pass已经被处理过，直接返回其Rectangle */
			return m_queuePasses[pass.locate.queueIndex][pass.locate.inqueueIndex];
		}
		/** 获得同一queue的上一个pass的大小以及
		 * Fence依赖的各个pass的大小 */
		std::vector<Rectangle> depRect;
		/** 同一个queue上的前一个pass的rect */
		if (pass.locate.inqueueIndex == 0) {
			/** 该pass是queue的第一个pass */
			depRect.push_back(Rectangle({ -PASS_WIDTH - PASS_PADDING, 0 }, Rectangle::PASS));
		}
		else {
			depRect.push_back(
				processPassHelper(
					m_passMap[pass.locate.queueIndex][pass.locate.inqueueIndex - 1]));
		}
		/** 依赖的pass的rect */
		for (const auto& depLocate : pass.depPasses) {
			depRect.push_back(
				processPassHelper(
					m_passMap[depLocate.queueIndex][depLocate.inqueueIndex]
				)
			);
		}
		/** 筛选出depRect中最靠右的rect */
		const Rectangle* mostRightRect = depRect.data();
		for (const auto& rect : depRect) {
			if (rect.leftUpPoint.x > mostRightRect->leftUpPoint.x) {
				mostRightRect = &rect;
			}
		}
		/** 计算出该pass的rect */
		Rectangle rectForThisPass({mostRightRect->leftUpPoint.x + PASS_WIDTH + PASS_PADDING, 0},
			Rectangle::PASS);
		rectForThisPass.desc = pass.name;
		m_queuePasses[pass.locate.queueIndex][pass.locate.inqueueIndex] = rectForThisPass;
		pass.processed = true;

		return rectForThisPass;
	}

	float PipelineGraph::processQueueHelper(QueueIdx queIdx) {
		Rectangle queRect = Rectangle({ LEFT_MARGIN, TOP_MARGIN }, Rectangle::QUEUE);
		queRect.leftUpPoint.y += queIdx * (QUEUE_HEIGHT + QUEUE_PADDING);
		/** 计算该queue应有的width */
		for (auto& passRect : m_queuePasses[queIdx]) {
			passRect.leftUpPoint.x += queRect.leftUpPoint.x + PASS_PADDING;
			passRect.leftUpPoint.y += queRect.leftUpPoint.y + centerOffset(QUEUE_HEIGHT, PASS_HEIGHT);
		}
		queRect.width = m_queuePasses[queIdx].back().leftUpPoint.x + PASS_WIDTH + PASS_PADDING - LEFT_MARGIN;

		m_queues[queIdx] = queRect;
		return queRect.width;
	}

	void PipelineGraph::processFenceHelper(const PassLocate & signal, const PassLocate & receiver)
	{
		/** 算出fence */
		const Rectangle& signalRect = m_queuePasses[signal.queueIndex][signal.inqueueIndex];
		const Rectangle& receRect = m_queuePasses[receiver.queueIndex][receiver.inqueueIndex];
		Arrow fence;
		fence.type = Arrow::FENCE;
		if (signalRect.leftUpPoint.y > receRect.leftUpPoint.y) {
			/** 依赖项在下 */
			fence.inflexionPoint.push_back({ signalRect.leftUpPoint.x + signalRect.width / 2, signalRect.leftUpPoint.y - ARROW_PADDING });
			fence.inflexionPoint.push_back({ signalRect.leftUpPoint.x + signalRect.width / 2, signalRect.leftUpPoint.y - PASS_QUEUE_PADDING_OFFSET / 2 });
			fence.inflexionPoint.push_back({ receRect.leftUpPoint.x, signalRect.leftUpPoint.y - PASS_QUEUE_PADDING_OFFSET / 2 });
			fence.inflexionPoint.push_back({ receRect.leftUpPoint.x, receRect.leftUpPoint.y + PASS_HEIGHT + ARROW_PADDING });
		}
		else {
			/** 依赖项在上 */
			fence.inflexionPoint.push_back({ signalRect.leftUpPoint.x + signalRect.width / 2, signalRect.leftUpPoint.y + PASS_HEIGHT + ARROW_PADDING });
			fence.inflexionPoint.push_back({ signalRect.leftUpPoint.x + signalRect.width / 2, signalRect.leftUpPoint.y + PASS_HEIGHT + PASS_QUEUE_PADDING_OFFSET / 2 });
			fence.inflexionPoint.push_back({ receRect.leftUpPoint.x, signalRect.leftUpPoint.y + PASS_HEIGHT + PASS_QUEUE_PADDING_OFFSET / 2 });
			fence.inflexionPoint.push_back({ receRect.leftUpPoint.x, receRect.leftUpPoint.y - ARROW_PADDING });
		}
		m_arrows.push_back(fence);
	}

	void PipelineGraph::processResourceHelper(ResourceIdx resIdx)
	{
		/** 调用该函数时，m_queues，m_queuePasses已被设置完成 */
		auto& resource = m_resourceMap[resIdx];
		Rectangle resRect({.0f, .0f}, Rectangle::RESOURCE);
		resRect.leftUpPoint.y = m_queues.back().leftUpPoint.y +
			m_queues.back().height + RESOURCE_PADDING +
			resIdx * (RESOURCE_HEIGHT+ RESOURCE_PADDING);

		if (resource.firstCreate != INVALID_PASS_LOCATE) {
			/** 该资源存在初始创建的pass */
			const auto& passRect = m_queuePasses[resource.firstCreate.queueIndex]
				[resource.firstCreate.inqueueIndex];
			resRect.leftUpPoint.x = passRect.leftUpPoint.x;
		}
		else {
			resRect.leftUpPoint.x = LEFT_MARGIN;
		}

		if (resource.lastDestroy != INVALID_PASS_LOCATE) {
			/** 该资源存在最终删除的pass */
			const auto& passRect = m_queuePasses[resource.lastDestroy.queueIndex]
				[resource.lastDestroy.inqueueIndex];
			resRect.width = passRect.leftUpPoint.x + passRect.width - resRect.leftUpPoint.x;
		}
		else {
			resRect.width = m_queues[0].width;
		}
		resRect.desc = resource.name;
		m_resources.push_back(resRect);

		/** 处理资源的读写情况 */
		for (const auto& read : resource.readPasses) {
			Arrow arrow;
			arrow.type = Arrow::READ;
			const auto& pass = m_queuePasses[read.queueIndex]
				[read.inqueueIndex];
			arrow.inflexionPoint.push_back({ pass.leftUpPoint.x + ARROW_LINE_WIDTH / 2, pass.leftUpPoint.y + PASS_HEIGHT + ARROW_PADDING });
			arrow.inflexionPoint.push_back({ pass.leftUpPoint.x + ARROW_LINE_WIDTH / 2, resRect.leftUpPoint.y - ARROW_PADDING });
			m_arrows.push_back(arrow);
		}

		for(const auto& write : resource.writedPasses) {
			Arrow arrow;
			arrow.type = Arrow::WRITE;
			const auto& pass = m_queuePasses[write.queueIndex]
				[write.inqueueIndex];
			arrow.inflexionPoint.push_back({ pass.leftUpPoint.x + ARROW_LINE_WIDTH / 2, pass.leftUpPoint.y + PASS_HEIGHT + ARROW_PADDING });
			arrow.inflexionPoint.push_back({ pass.leftUpPoint.x + ARROW_LINE_WIDTH / 2, resRect.leftUpPoint.y - ARROW_PADDING });
			m_arrows.push_back(arrow);
		}

		/** 处理资源的barrier */
		for (const auto& barrier : resource.barriers) {
			const auto& pass = m_queuePasses[barrier.submitPass.queueIndex]
				[barrier.submitPass.inqueueIndex];
			Transition transt;
			transt.center.x = pass.leftUpPoint.x;
			transt.center.y = resRect.leftUpPoint.y + RESOURCE_HEIGHT / 2.0f;
			transt.flag = barrier.flags;
			transt.desc = barrier.description;
			m_transts.push_back(transt);
		}
}

	void PipelineGraph::Raster(const char* name)
	{
		SVGBase svg(name ? name : "test");
		/** 处理所有queue */
		for (const auto& queue : m_queues)
			svg.AddRect(queue);
		/** 处理所有的pass */
		for (const auto& queue : m_queuePasses)
			for (const auto& pass : queue)
				svg.AddRect(pass);
		/** 处理所有的resource */
		for (const auto& resource : m_resources)
			svg.AddRect(resource);
		/** 处理所有的arrow */
		for (auto& arrow : m_arrows)
			svg.AddArrow(arrow);
		/** 处理所有的barrer */
		for (const auto& transt : m_transts)
			svg.AddTransition(transt);

		svg.Save();
	}

	void PipelineGraph::Setup()
	{
		/** 初始化各个vector */
		m_queues = std::vector<Rectangle>(m_passMap.size());
		for (const auto& queue : m_passMap) {
			m_queuePasses.push_back(std::vector<Rectangle>(queue.size()));
		}
		/** 初步处理所有的pass */
		for (auto& queue : m_passMap) {
			for (auto& pass : queue) {
				processPassHelper(pass);
			}
		}
		/** 处理所有的queue */
		float maxQueueWidth = 0.0f;
		for (QueueIdx queIdx = 0; queIdx < m_queues.size(); ++queIdx) {
			float curQueueWidth = processQueueHelper(queIdx);
			if (maxQueueWidth < curQueueWidth)
				maxQueueWidth = curQueueWidth;
		}
		/** 处理所有的fence */
		for (const auto& queue : m_passMap) {
			for (const auto& pass : queue) {
				for (const auto& signal : pass.depPasses) {
					processFenceHelper(signal, pass.locate);
				}
			}
		}
		for (auto& rect : m_queues) {
			rect.width = maxQueueWidth;
		}
		/** 处理所有的resource */
		for (ResourceIdx resIdx = 0; resIdx < m_resourceMap.size(); ++resIdx) {
			processResourceHelper(resIdx);
		}
	}

}
