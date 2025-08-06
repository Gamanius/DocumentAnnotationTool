#pragma once

#include "Window.h"

namespace DocantoWin {
	class TSFHandler {
	public:
		TSFHandler(std::shared_ptr<Window> w);
		~TSFHandler();

	private:
		struct impl;
		std::unique_ptr<impl> pimpl;
	};

}