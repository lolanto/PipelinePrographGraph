#ifndef SVG_PROCESS_H
#define SVG_PROCESS_H

#include <tinyxml2.h>
#include <cstdio>
#include "ppfgEle.h"

class SVGBase {
private:
	static const float STROKE_WIDTH;
public:
	SVGBase(const char* name) : m_svgName(name),
		m_canvasWidth(300), m_canvasHeight(300) {
		m_canvas = m_doc.NewElement("svg");
		m_canvas->SetAttribute("version", 1.1f);
		m_canvas->SetAttribute("baseProfile", "full");
		m_canvas->SetAttribute("width", m_canvasWidth);
		m_canvas->SetAttribute("height", m_canvasHeight);
		m_canvas->SetAttribute("xmlns", "http://www.w3.org/2000/svg");
		m_canvas->SetAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
		m_doc.InsertFirstChild(m_canvas);
		/** 预定义基本体 */
		tinyxml2::XMLElement* def = m_doc.NewElement("defs");
		m_canvas->InsertEndChild(def);
		/** 创建Barrier的基本体 */
		tinyxml2::XMLElement* barrierRight = m_doc.NewElement("polygon");
		std::vector<char> points(100, 0);
		std::sprintf(points.data(), "%.3f,%.3f %.3f,%.3f %.3f,%.3f %.3f,%.3f %.3f,%.3f %.3f,%.3f",
			0.0f, 0.0f, -PipelineProfilingGraph::BARRIER_WIDTH / 2, -PipelineProfilingGraph::BARRIER_HEIGHT / 2,
			PipelineProfilingGraph::BARRIER_WIDTH / 2, -PipelineProfilingGraph::BARRIER_HEIGHT / 2,
			PipelineProfilingGraph::BARRIER_WIDTH, 0.0f,
			PipelineProfilingGraph::BARRIER_WIDTH / 2, PipelineProfilingGraph::BARRIER_HEIGHT / 2,
			-PipelineProfilingGraph::BARRIER_WIDTH / 2, PipelineProfilingGraph::BARRIER_HEIGHT / 2);
		barrierRight->SetAttribute("points", points.data());
		barrierRight->SetAttribute("id", "RightBarrier");
		def->InsertEndChild(barrierRight);
		tinyxml2::XMLElement* barrierLeft = m_doc.NewElement("polygon");
		memset(points.data(), 0, points.size() * sizeof(char));
		std::sprintf(points.data(), "%.3f,%.3f %.3f,%.3f %.3f,%.3f %.3f,%.3f %.3f,%.3f %.3f,%.3f",
			0.0f, 0.0f, PipelineProfilingGraph::BARRIER_WIDTH / 2, -PipelineProfilingGraph::BARRIER_HEIGHT / 2,
			-PipelineProfilingGraph::BARRIER_WIDTH / 2, -PipelineProfilingGraph::BARRIER_HEIGHT / 2,
			-PipelineProfilingGraph::BARRIER_WIDTH, 0.0f,
			-PipelineProfilingGraph::BARRIER_WIDTH / 2, PipelineProfilingGraph::BARRIER_HEIGHT / 2,
			PipelineProfilingGraph::BARRIER_WIDTH / 2, PipelineProfilingGraph::BARRIER_HEIGHT / 2);
		barrierLeft->SetAttribute("points", points.data());
		barrierLeft->SetAttribute("id", "LeftBarrier");
		def->InsertEndChild(barrierLeft);
		/** 创建菱形 */
		tinyxml2::XMLElement* diamond = m_doc.NewElement("polygon");
		memset(points.data(), 0, points.size() * sizeof(char));
		std::sprintf(points.data(), "%.3f,%.3f %.3f,%.3f %.3f,%.3f %.3f,%.3f",
			-PipelineProfilingGraph::ARROW_LINE_END_RADIUS, 0.0f,
			0.0f, -PipelineProfilingGraph::ARROW_LINE_END_RADIUS,
			PipelineProfilingGraph::ARROW_LINE_END_RADIUS, 0.0f,
			0.0f, PipelineProfilingGraph::ARROW_LINE_END_RADIUS);
		diamond->SetAttribute("points", points.data());
		diamond->SetAttribute("id", "Diamond");
		def->InsertEndChild(diamond);
	}
	void SetCanvasWidth(float width) {
		m_canvas->SetAttribute("width", width);
		m_canvasWidth = width;
	}
	void SetCanvasHeight(float height) {
		m_canvas->SetAttribute("height", height);
		m_canvasHeight = height;
	}
	void Save() {
		m_doc.SaveFile((m_svgName + ".xml").c_str());
	}
	tinyxml2::XMLElement* AddRect(const PipelineProfilingGraph::Rectangle& rect) {
		if (rect.leftUpPoint.x + rect.width > m_canvasWidth) {
			SetCanvasWidth(rect.leftUpPoint.x + rect.width + PipelineProfilingGraph::LEFT_MARGIN);
		}
		if (rect.leftUpPoint.y + rect.height > m_canvasHeight) {
			SetCanvasHeight(rect.leftUpPoint.y + rect.height + PipelineProfilingGraph::TOP_MARGIN);
		}

		tinyxml2::XMLElement* rectEle = m_doc.NewElement("rect");
		rectEle->SetAttribute("x", rect.leftUpPoint.x);
		rectEle->SetAttribute("y", rect.leftUpPoint.y);
		rectEle->SetAttribute("width", rect.width);
		rectEle->SetAttribute("height", rect.height);
		rectangleStyleHelper(rect.type, rectEle);
		titleHelper(rectEle, rect.desc.c_str());
		m_canvas->InsertEndChild(rectEle);
		return rectEle;
	}
	tinyxml2::XMLElement* AddTransition(const PipelineProfilingGraph::Transition& transt) {
		/** 这里假设Transition是不会撑开画布的(Transition一定在画布内) */
		bool towardLeft = transt.flag & PipelineProfilingGraph::Barrier::END;
		tinyxml2::XMLElement* barrier = m_doc.NewElement("use");
		barrier->SetAttribute("x", transt.center.x);
		barrier->SetAttribute("y", transt.center.y);
		if (towardLeft)
			barrier->SetAttribute("xlink:href", "#LeftBarrier");
		else
			barrier->SetAttribute("xlink:href", "#RightBarrier");
		lineTranstStyleHelper(transt.flag, barrier);
		titleHelper(barrier, transt.desc.c_str());
		m_canvas->InsertEndChild(barrier);
		return barrier;
	}

	tinyxml2::XMLElement* AddArrow(PipelineProfilingGraph::Arrow& arrow) {
		tinyxml2::XMLElement* arrowPath = m_doc.NewElement("path");
		tinyxml2::XMLElement* arrowHead = nullptr;
		if (arrow.type == PipelineProfilingGraph::Arrow::FENCE) {
			pathHelper(arrow.inflexionPoint, arrowPath);
			arrowPath->SetAttribute("stroke", "#c0c090");
			arrowHead = m_doc.NewElement("use");
			arrowHead->SetAttribute("xlink:href", "#Diamond");
			arrowHead->SetAttribute("fill", "#c0c090");
			arrowHead->SetAttribute("x", arrow.inflexionPoint.back().x);
			arrowHead->SetAttribute("y", arrow.inflexionPoint.back().y);
		}
		else {
			auto& xoc = m_xOccupy.find(static_cast<uint32_t>(arrow.inflexionPoint[0].x));
			if (xoc == m_xOccupy.end()) {
				m_xOccupy.insert(std::make_pair(static_cast<uint32_t>(arrow.inflexionPoint[0].x), arrow.inflexionPoint[0].x));
			}
			else {
				xoc->second += PipelineProfilingGraph::ARROW_LINE_END_RADIUS;
				arrow.inflexionPoint[0].x = xoc->second;
				arrow.inflexionPoint[1].x = xoc->second;
			}
			pathHelper(arrow.inflexionPoint, arrowPath);
			if (arrow.type == PipelineProfilingGraph::Arrow::READ) {
				arrowPath->SetAttribute("stroke", "#13ff13");
				arrowHead = m_doc.NewElement("circle");
				arrowHead->SetAttribute("r", PipelineProfilingGraph::ARROW_LINE_END_RADIUS);
				arrowHead->SetAttribute("cx", arrow.inflexionPoint.back().x);
				arrowHead->SetAttribute("cy", arrow.inflexionPoint.back().y);
				arrowHead->SetAttribute("fill", "#13ff13");
			}
			else {
				arrowPath->SetAttribute("stroke", "#ff1917");
				arrowHead = m_doc.NewElement("rect");
				arrowHead->SetAttribute("x", arrow.inflexionPoint.back().x - PipelineProfilingGraph::ARROW_LINE_END_RADIUS);
				arrowHead->SetAttribute("y", arrow.inflexionPoint.back().y - PipelineProfilingGraph::ARROW_LINE_END_RADIUS);
				arrowHead->SetAttribute("fill", "#ff1917");
				arrowHead->SetAttribute("width", PipelineProfilingGraph::ARROW_LINE_END_RADIUS * 2);
				arrowHead->SetAttribute("height", PipelineProfilingGraph::ARROW_LINE_END_RADIUS * 2);
			}
		}
		m_canvas->InsertEndChild(arrowPath);
		m_canvas->InsertEndChild(arrowHead);
		return arrowPath;
	}
private:
	void rectangleStyleHelper(PipelineProfilingGraph::Rectangle::Type type,
		tinyxml2::XMLElement* ele) {
		switch (type)
		{
		case PipelineProfilingGraph::Rectangle::UNDEFINED:
			break;
		case PipelineProfilingGraph::Rectangle::QUEUE:
			ele->SetAttribute("fill", "transparent");
			ele->SetAttribute("stroke", "black");
			ele->SetAttribute("stroke-width", STROKE_WIDTH);
			ele->SetAttribute("rx", 3);
			ele->SetAttribute("ry", 3);
			break;
		case PipelineProfilingGraph::Rectangle::PASS:
			ele->SetAttribute("fill", "#43a6e2");
			ele->SetAttribute("stroke", "black");
			ele->SetAttribute("stroke-width", STROKE_WIDTH);
			ele->SetAttribute("rx", 3);
			ele->SetAttribute("ry", 3);
			break;
		case PipelineProfilingGraph::Rectangle::RESOURCE:
			ele->SetAttribute("fill", "#ffa129");
			ele->SetAttribute("stroke", "black");
			ele->SetAttribute("stroke-width", STROKE_WIDTH);
			ele->SetAttribute("rx", 3);
			ele->SetAttribute("ry", 3);
			break;
		default:
			break;
		}
	}
	void lineTranstStyleHelper(uint8_t flag,
		tinyxml2::XMLElement* ele) {
		uint8_t typeFlag1 = flag & 0x07U;
		uint8_t typeFlag2 = flag & 0x38U;
		switch (typeFlag1) {
		case PipelineProfilingGraph::Barrier::TRANSITION_BARRIER:
			ele->SetAttribute("stroke", "#da3b01");
			break;
		case PipelineProfilingGraph::Barrier::ALIASING_BARRIER:
			ele->SetAttribute("stroke", "#128712");
			break;
		case PipelineProfilingGraph::Barrier::UAV_BARRIER:
			ele->SetAttribute("stroke", "#0065b3");
			break;
		case PipelineProfilingGraph::Barrier::TRANSITION_BARRIER | PipelineProfilingGraph::Barrier::ALIASING_BARRIER:
			ele->SetAttribute("stroke", "#ffbb00");
			break;
		case PipelineProfilingGraph::Barrier::TRANSITION_BARRIER | PipelineProfilingGraph::Barrier::UAV_BARRIER:
			ele->SetAttribute("stroke", "#8763c5");
			break;
		case PipelineProfilingGraph::Barrier::TRANSITION_BARRIER | PipelineProfilingGraph::Barrier::ALIASING_BARRIER | PipelineProfilingGraph::Barrier::UAV_BARRIER:
			ele->SetAttribute("stroke", "white");
			break;
		case PipelineProfilingGraph::Barrier::ALIASING_BARRIER | PipelineProfilingGraph::Barrier::UAV_BARRIER:
			ele->SetAttribute("stroke", "#24ff24");
			break;
		default:
			break;
		}
		switch (typeFlag2) {
		case PipelineProfilingGraph::Barrier::IMMEDIACY:
			ele->SetAttribute("stroke-linejoin", "miter");
			break;
		case PipelineProfilingGraph::Barrier::BEGIN:
		case PipelineProfilingGraph::Barrier::END:
			ele->SetAttribute("stroke-linejoin", "round");
			break;
		case PipelineProfilingGraph::Barrier::IMMEDIACY | PipelineProfilingGraph::Barrier::BEGIN:
		case PipelineProfilingGraph::Barrier::IMMEDIACY | PipelineProfilingGraph::Barrier::END:
			ele->SetAttribute("stroke-linejoin", "bevel");
			break;
		}
	}
	void pathHelper(const std::vector<PipelineProfilingGraph::Point>& points,
		tinyxml2::XMLElement* ele) {
		std::string d;
		std::vector<char> cmd(30, 0);
		std::sprintf(cmd.data(), "M%.3f %.3f ", points[0].x, points[0].y);
		d += cmd.data();
		memset(cmd.data(), 0, cmd.size() * sizeof(char));
		for (uint32_t index = 1; index < points.size(); ++index) {
			std::sprintf(cmd.data(), "L%.3f %.3f ", points[index].x, points[index].y);
			d += cmd.data();
			memset(cmd.data(), 0, cmd.size() * sizeof(char));
		}
		ele->SetAttribute("d", d.c_str());
		ele->SetAttribute("stroke-width", PipelineProfilingGraph::ARROW_LINE_WIDTH);
		ele->SetAttribute("fill", "transparent");
	}
	void titleHelper(tinyxml2::XMLElement* ele, const char* titleContent) {
		tinyxml2::XMLElement* title = m_doc.NewElement("title");
		title->SetText(titleContent);
		ele->InsertEndChild(title);
	}
private:
	std::map<uint32_t, float> m_xOccupy;
	std::string m_svgName;
	tinyxml2::XMLDocument m_doc;
	tinyxml2::XMLElement* m_canvas;
	float m_canvasWidth;
	float m_canvasHeight;
};

#endif // SVG_PROCESS_H
