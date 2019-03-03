#ifndef SVG_ELEMENT_H
#define SVG_ELEMENT_H

#include <vector>
/** 该文件存储途中所有图形元素的描述结构 */
namespace PipelineProfilingGraph {

	const float LEFT_MARGIN = 5.0f;
	const float TOP_MARGIN = 5.0f;

	const float QUEUE_HEIGHT = 20.0f;
	const float QUEUE_PADDING = 5.0f;

	const float PASS_HEIGHT = 15.0f;
	const float PASS_WIDTH = PASS_HEIGHT / 0.618f;
	const float PASS_PADDING = PASS_HEIGHT * 0.618f;

	const float PASS_QUEUE_PADDING_OFFSET = QUEUE_PADDING + (QUEUE_HEIGHT - PASS_HEIGHT);

	const float RESOURCE_HEIGHT = PASS_HEIGHT / 2.0f;
	const float RESOURCE_PADDING = QUEUE_PADDING;

	const float BARRIER_WIDTH = PASS_WIDTH / 4.0f;
	const float BARRIER_HEIGHT = BARRIER_WIDTH / 0.618f;

	const float ARROW_PADDING = PASS_HEIGHT / 40.0f;
	const float ARROW_LINE_WIDTH = BARRIER_WIDTH / 4;
	const float ARROW_LINE_END_RADIUS = ARROW_LINE_WIDTH / 0.618f;

	struct Point {
		float x, y;
	};

	struct Rectangle {
		enum Type {
			UNDEFINED,
			QUEUE,
			PASS,
			RESOURCE,
		};
		Rectangle() : leftUpPoint({ .0f, .0f }), width(.0f), height(.0f), type(Type::UNDEFINED) {}
		Rectangle(Point lup, float width, float height, Type type)
			: leftUpPoint(lup), width(width), height(height), type(type) {}
		Rectangle(Point lup, Type type)
			: leftUpPoint(lup), type(type) {
			switch (type) {
			case QUEUE:
				width = 0.0f;
				height = QUEUE_HEIGHT;
				break;
			case PASS:
				width = PASS_WIDTH;
				height = PASS_HEIGHT;
				break;
			case RESOURCE:
				width = .0f;
				height = RESOURCE_HEIGHT;
				break;
			default:
				width = height = 0.0f;
				break;
			}
		}
		Point leftUpPoint; /**< 左上角的位置 */
		float width; /**< 矩形的宽度 */
		float height; /**< 矩形的高度 */
		Type type; /**< 矩形的类型，类型不同外观不同 */
		std::string desc; /**< 矩形的描述信息 */
	};

	/** “传递”形状的图形元素 */
	struct Transition {
		Point center; /**< 图形的中心点 */
		uint8_t flag; /**< 图形的特殊设置，设置不同外观不同，详看Barrier::Flag枚举值 */
		std::string desc; /**< 图形的描述信息 */
	};
	/** 箭头图形元素 */
	struct Arrow {
		enum Type {
			READ,
			WRITE,
			FENCE
		};
		std::vector<Point> inflexionPoint; /**< 箭头的各个拐角位置，begin和end的点表示箭头的两个端点 */
		Type type; /**< 箭头类型，类型不同外观不同 */
	};
}

#endif // SVG_ELEMENT_H
