#include "ppfg.h"
using namespace PipelineProfilingGraph;

int main() {
	Queue q0;
	q0.push_back(Pass("G-Buffer", 0, 0, {}));
	q0.push_back(Pass("Shadows", 0, 1, {}));
	q0.push_back(Pass("lighting", 0, 2, { {1, 0} }));
	Queue q1;
	q1.push_back(Pass("SSAO", 1, 0, { {0, 1} }));
	std::vector<Resource> res;
	Resource depth = Resource("Depth", { 0, 0 }, { 0, 2 }, { {0, 2}, {1, 0} }, { {0, 0} });
	depth.barriers.push_back(Barrier({ 0, 0 }, "ba1", Barrier::TRANSITION_BARRIER | Barrier::IMMEDIACY));
	Resource shadowMap = Resource("ShadowMap", { 0, 1 }, { 0, 2 }, { {0, 2} }, { {0, 1} });
	shadowMap.barriers.push_back(Barrier({ 0, 1 }, "ba2", Barrier::ALIASING_BARRIER | Barrier::BEGIN));
	shadowMap.barriers.push_back(Barrier({ 0, 2 }, "ba3", Barrier::ALIASING_BARRIER | Barrier::END));
	Resource ssao = Resource("SSAO", { 1, 0 }, { 0, 2 }, { {0, 2} }, { {1, 0} });
	ssao.barriers.push_back(Barrier({ 1, 0 }, "ba4", Barrier::UAV_BARRIER | Barrier::IMMEDIACY));
	res.push_back(depth);
	res.push_back(shadowMap);
	res.push_back(ssao);
	PipelineGraph sg({ q0, q1 }, res);
	sg.Setup();
	sg.Raster();
	return 0;
}