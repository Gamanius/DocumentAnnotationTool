#pragma once

#ifndef _DOCANTOWIN_GENERICUIOBJECT_
#define _DOCANTOWIN_GENERICUIOBJECT_

#include "win/Direct2D.h"


namespace DocantoWin {
	class GenericUIObject {
	protected:
		std::shared_ptr<Window> m_window;
		std::shared_ptr<Direct2DRender> m_localrender;

		bool m_is_floating = false;
	public:
		GenericUIObject(const std::wstring& UIName);

		virtual Docanto::Geometry::Rectangle<float> get_bounds() = 0;
		virtual Docanto::Geometry::Dimension<long> get_min_dims() = 0;
		virtual int hit_test(Docanto::Geometry::Point<long> where) = 0;

		virtual void pointer_press(Docanto::Geometry::Point<float> where, int hit) = 0;
		virtual void pointer_update(Docanto::Geometry::Point<float> where, int hit) = 0;
		virtual void pointer_release(Docanto::Geometry::Point<float> where, int hit) = 0;

		virtual void draw(std::shared_ptr<Direct2DRender> render = nullptr) = 0;

		bool is_inbounds(Docanto::Geometry::Dimension<float> r);
		void make_float(bool f);
		bool is_floating() const;
	};
}

#endif // !_DOCANTOWIN_GENERICUIOBJECT_
