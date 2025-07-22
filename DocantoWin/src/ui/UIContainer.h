#pragma once

#include "GenericUIObject.h"

namespace DocantoWin {
	class UIContainer {
		std::vector<std::shared_ptr<GenericUIObject>> m_all_uiobjects;
		std::pair<std::shared_ptr<GenericUIObject>, int> m_hit_uiobject;

		std::shared_ptr<Direct2DRender> m_render;

	public:
		UIContainer(std::shared_ptr<Direct2DRender> render);

		void draw();
		void pointer_down(Docanto::Geometry::Point<float> where);
		void pointer_update(Docanto::Geometry::Point<float> where);
		void pointer_up(Docanto::Geometry::Point<float> where);

		void add(std::shared_ptr<GenericUIObject> obj);
	};
}