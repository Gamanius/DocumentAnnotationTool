#pragma once

#include "GenericUIObject.h"

namespace DocantoWin {
	class UIContainer {
		std::vector<std::shared_ptr<GenericUIObject>> m_all_uiobjects;
		std::pair<std::shared_ptr<GenericUIObject>, int> m_hit_uiobject;

		std::weak_ptr<Context> ctx;

	public:
		UIContainer(std::weak_ptr<Context> c);

		void draw();
		void pointer_down(Docanto::Geometry::Point<float> where);
		void pointer_update(Docanto::Geometry::Point<float> where);
		void pointer_up(Docanto::Geometry::Point<float> where);

		void resize(Docanto::Geometry::Dimension<long> new_dim);

		std::vector<std::shared_ptr<GenericUIObject>>& get_all_uiobjects_ref();

		void add(std::shared_ptr<GenericUIObject> obj);
	};
}