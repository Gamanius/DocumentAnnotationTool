#include "include.h"

Renderer::CubicBezierGeometry Renderer::create_bezier_geometry(const std::vector<Point<float>>& points) {
	std::vector<Renderer::Point<float>> a(points.size()), b(points.size());
	calcBezierPoints(points, a, b);

	std::vector<Renderer::Point<float>> points_copy = points; 

	return Renderer::CubicBezierGeometry({ std::move(points_copy), {std::move(a), std::move(b)} });
}