#include "Direct2D.h"

Renderer::CubicBezierGeometry Renderer::create_bezier_geometry(const std::vector<Math::Point<float>>& points) {
	std::vector<Math::Point<float>> a(points.size()), b(points.size());
	calcBezierPoints(points, a, b);

	std::vector<Math::Point<float>> points_copy = points;

	return Renderer::CubicBezierGeometry({ std::move(points_copy), {std::move(a), std::move(b)} });
}