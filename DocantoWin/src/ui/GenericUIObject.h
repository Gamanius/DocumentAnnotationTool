#pragma once

#ifndef _DOCANTOWIN_GENERICUIOBJECT_
#define _DOCANTOWIN_GENERICUIOBJECT_

#include "win/Direct2D.h"


namespace DocantoWin {
	class GenericUIObject {
	private:
		Docanto::Geometry::Point<float> m_position;
		Docanto::Geometry::Point<float> m_local_mouse;

		Docanto::Geometry::Point<float> m_caption_delta_mouse;
	protected:
		std::shared_ptr<Window> m_window;
		std::shared_ptr<Direct2DRender> m_localrender;

		std::shared_ptr<Window> m_parent_window = nullptr;
		bool m_is_floating = false;

		int resize_hittest(Docanto::Geometry::Point<long> p);
	public:
		GenericUIObject(const std::wstring& UIName);

		void set_parent_window(std::shared_ptr<Window> main_window);

		virtual Docanto::Geometry::Dimension<float> get_bounds() = 0;
		virtual Docanto::Geometry::Dimension<long> get_min_dims() = 0;
		virtual int hit_test(Docanto::Geometry::Point<long> where) = 0;
		virtual bool is_resizable() const;

		virtual void pointer_press(Docanto::Geometry::Point<float> where, int hit) = 0;
		virtual void pointer_update(Docanto::Geometry::Point<float> where, int hit) = 0;
		virtual void pointer_release(Docanto::Geometry::Point<float> where, int hit) = 0;

		bool sys_pointer_down(Docanto::Geometry::Point<float> where, int hit);
		bool sys_pointer_update(Docanto::Geometry::Point<float> where, int hit);
		bool sys_pointer_release(Docanto::Geometry::Point<float> where, int hit);

		virtual void draw(std::shared_ptr<Direct2DRender> render = nullptr) = 0;

		bool is_inbounds(Docanto::Geometry::Dimension<float> r);
		Docanto::Geometry::Rectangle<float> get_rec();

		virtual void set_pos(Docanto::Geometry::Point<float> where);
		Docanto::Geometry::Point<float> get_pos();

		Docanto::Geometry::Point<float> get_mouse_pos();

		void make_float(bool f);
		bool is_floating() const;
	};
}

#endif // !_DOCANTOWIN_GENERICUIOBJECT_
