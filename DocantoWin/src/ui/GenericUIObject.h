#pragma once

#ifndef _DOCANTOWIN_GENERICUIOBJECT_
#define _DOCANTOWIN_GENERICUIOBJECT_

#include "helper/Context.h"
#include "win/Window.h"

namespace DocantoWin {
	class GenericUIObject {
	private:
		Docanto::Geometry::Point<float> m_position;
		Docanto::Geometry::Dimension<float> m_dimension;

		Docanto::Geometry::Point<float> m_caption_delta_mouse;
		Docanto::Geometry::Point<float> m_last_mouse_pos;

		bool resizable = false;
		const float resize_square_size   = 4.0f;
		const byte  resize_sqaure_amount = 5;
	protected:
		std::shared_ptr<Window> m_window;
		std::shared_ptr<Direct2DRender> m_localrender;

		std::weak_ptr<Context> ctx;
		bool m_is_floating = false;

		int resize_hittest(Docanto::Geometry::Point<long> p);
	public:
		GenericUIObject(const std::wstring& UIName, std::weak_ptr<Context> c, bool resize = true);

		Docanto::Geometry::Dimension<float> get_bounds();
		void set_bounds(Docanto::Geometry::Dimension<float> bounds);
		
		virtual Docanto::Geometry::Dimension<long> get_min_dims() = 0;

		int sys_hit_test(Docanto::Geometry::Point<long> where);
		virtual int hit_test(Docanto::Geometry::Point<long> where) = 0;

		bool is_resizable() const;
		void set_resizable(bool resize);
		
		virtual bool pointer_press(const Window::PointerInfo& p, int hit) = 0;
		virtual bool pointer_update(const Window::PointerInfo& p, int hit) = 0;
		virtual bool pointer_release(const Window::PointerInfo& p, int hit) = 0;

		bool sys_pointer_down(const Window::PointerInfo& p, int hit);
		bool sys_pointer_update(const Window::PointerInfo& p, int hit);
		bool sys_pointer_release(const Window::PointerInfo& p, int hit);

		virtual Window::CURSOR_TYPE get_mouse(Docanto::Geometry::Point<float> where) = 0;
		Window::CURSOR_TYPE do_get_mouse(Docanto::Geometry::Point<float> where);

		void draw_border();
		void sys_draw();
		virtual void draw(std::shared_ptr<Direct2DRender> render) = 0;

		bool is_inbounds(Docanto::Geometry::Dimension<float> r);
		bool is_inbounds();
		Docanto::Geometry::Rectangle<float> get_rec();

		virtual void set_pos(Docanto::Geometry::Point<float> where);
		Docanto::Geometry::Point<float> get_pos();

		Docanto::Geometry::Point<float> get_mouse_pos();

		void make_float(bool f);
		bool is_floating() const;
	};
}

#endif // !_DOCANTOWIN_GENERICUIOBJECT_
