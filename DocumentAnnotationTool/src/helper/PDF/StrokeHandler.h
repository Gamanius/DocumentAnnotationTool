#pragma once

#include "../General/General.h" 
#include "../Windows/Direct2D.h"
#include "MuPDF.h"

#include <algorithm>
#include <list>

#ifndef _STROKE_HANDLER_H_
#define _STROKE_HANDLER_H_

class StrokeHandler {
	struct Stroke {
		std::shared_ptr<ContextWrapper> ctx;
		std::vector<Math::Point<float>> points;
		pdf_annot* annot = nullptr;
		size_t page = 0;
		Renderer::CubicBezierGeometry geometry;
		Math::Rectangle<float> bounding_box;
		Direct2DRenderer::PathObject path;
		float thickness = 1.0f;
		Renderer::Color color = { 0, 0, 0 };
		bool to_be_erased = false;

		Stroke() = default;
		Stroke(const Stroke& s) = delete;
		Stroke(Stroke&& s) noexcept;
		Stroke& operator=(Stroke s) noexcept;
		~Stroke();

		friend void swap(Stroke& first, Stroke& second) {
			using std::swap;

			swap(first.points, second.points);
			swap(first.thickness, second.thickness);
			swap(first.color, second.color);
			swap(first.path, second.path);
			swap(first.geometry, second.geometry);
			swap(first.page, second.page);
			swap(first.annot, second.annot);
			swap(first.ctx, second.ctx);
			swap(first.bounding_box, second.bounding_box);
			swap(first.to_be_erased, second.to_be_erased);
		}
	};

	std::list<Stroke> m_strokes; 
	std::map<UINT, Stroke> m_active_strokes;
	// earising strokes
	std::vector<Math::Point<float>> m_earising_points;
	// not owned by this class
	MuPDFHandler::PDF* m_pdf = nullptr;
	// not owned by this class!
	Direct2DRenderer* m_renderer = nullptr;
	// not owned by this class!
	WindowHandler* m_window = nullptr;

	void apply_stroke_to_pdf(Stroke& s);
	void parse_all_strokes();
	std::optional<size_t> get_page_from_point(Math::Point<float> p);

public:
	StrokeHandler() = default;

	StrokeHandler(MuPDFHandler::PDF* const pdf, Direct2DRenderer* const renderer, WindowHandler* const window);
	StrokeHandler(const StrokeHandler& t) = delete;

	// move constructor
	StrokeHandler(StrokeHandler&& t) noexcept;
	// move assignment
	StrokeHandler& operator=(StrokeHandler t) noexcept;

	~StrokeHandler();

	void start_stroke(const WindowHandler::PointerInfo& p);
	void update_stroke(const WindowHandler::PointerInfo& p);
	void end_stroke(const WindowHandler::PointerInfo& p);

	void earsing_stroke(const WindowHandler::PointerInfo& p);
	void end_earsing_stroke(const WindowHandler::PointerInfo& p);

	void render_strokes();


	friend void swap(StrokeHandler& first, StrokeHandler& second);

};
#endif // _STROKE_HANDLER_H_
