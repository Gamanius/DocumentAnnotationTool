#pragma once
#include "json.hpp"
#include "Math.h"
#include "d2d1.h"

typedef unsigned char byte;

namespace Renderer {
	struct Color {
		byte r, g, b = 0;
		Color() = default;
		Color(byte r, byte g, byte b) : r(r), g(g), b(b) {}

		operator D2D1_COLOR_F() const {
			return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f);
		}
	};

	struct AlphaColor : public Color {
		byte alpha = 0;
		AlphaColor() = default;
		AlphaColor(byte r, byte g, byte b, byte alpha) : Color(r, g, b), alpha(alpha) {}

		operator D2D1_COLOR_F() const {
			return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, alpha / 255.0f);
		}
	};

	inline void to_json(nlohmann::json& j, const Color& c) {
		j = nlohmann::json{ {"r", c.r}, {"g", c.g}, {"b", c.b} };
	}

	inline void from_json(const nlohmann::json& j, Color& c) {
		j.at("r").get_to(c.r);
		j.at("g").get_to(c.g);
		j.at("b").get_to(c.b);
	}

	inline void to_json(nlohmann::json& j, const AlphaColor& c) {
		j = nlohmann::json{ {"r", c.r}, {"g", c.g}, {"b", c.b}, {"alpha", c.alpha} };
	}

	inline void from_json(const nlohmann::json& j, AlphaColor& c) {
		j.at("r").get_to(c.r);
		j.at("g").get_to(c.g);
		j.at("b").get_to(c.b);
		j.at("alpha").get_to(c.alpha);
	}

	// https://www.quantstart.com/articles/Tridiagonal-Matrix-Algorithm-Thomas-Algorithm-in-C/
	inline void solveTridiagonal(std::vector<float>& a,
		std::vector<float>& b,
		std::vector<float>& c,
		std::vector<Math::Point<float>>& d,
		std::vector<Math::Point<float>>& x) {
		for (size_t i = 1; i < d.size(); i++) {
			auto w = a[i - 1] / b[i - 1];
			b[i] = b[i] - w * c[i - 1];
			d[i] = d[i] - w * d[i - 1];
		}

		x[d.size() - 1] = d[d.size() - 1] / b[d.size() - 1];
		for (long i = static_cast<long>(d.size() - 2); i >= 0; i--) {
			x[i] = (d[i] - c[i] * x[i + 1]) / b[i];
		}
	}

	//https://omaraflak.medium.com/b%C3%A9zier-interpolation-8033e9a262c2
	inline void calcBezierPoints(const std::vector<Math::Point<float>>& points, std::vector<Math::Point<float>>& a, std::vector<Math::Point<float>>& b) {
		//create the vectos
		auto n = points.size() - 1;
		std::vector<Math::Point<float>> p(n);

		p[0] = points[0] + 2 * points[1];
		for (size_t i = 1; i < n - 1; i++) {
			p[i] = 2 * (2 * points[i] + points[i + 1]);
		}
		p[n - 1] = 8 * points[n - 1] + points[n];

		// matrix
		std::vector<float> m_a(n - 1, 1);
		std::vector<float> m_b(n, 4);
		std::vector<float> m_c(n - 1, 1);
		m_a[n - 2] = 2;
		m_b[0] = 2;
		m_b[n - 1] = 7;

		solveTridiagonal(m_a, m_b, m_c, p, a);

		b[n - 1] = (a[n - 1] + points[n]) / 2.0;
		for (size_t i = 0; i < n - 1; i++) {
			b[i] = 2 * points[i + 1] - a[i + 1];
		}
	}

	template <typename T, size_t N>
	struct BezierGeometry {
		std::vector<Math::Point<T>> points;
		std::array<std::vector<Math::Point<T>>, N> control_points;
	};

	using CubicBezierGeometry = BezierGeometry<float, 2>;

	CubicBezierGeometry create_bezier_geometry(const std::vector<Math::Point<float>>& points);
}
