#pragma once
#include "../rstatic.h"

class RStatic_OpenGL : public RStatic
{ public:
	virtual void InitializeWindow(RINTERFACE_WINDOW_CLASS window);
	virtual void BeforeRender(RINTERFACE_WINDOW_CLASS window);
	virtual void AfterRender(RINTERFACE_WINDOW_CLASS window);
};