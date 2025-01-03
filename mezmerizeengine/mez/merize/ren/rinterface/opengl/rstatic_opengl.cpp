#include "rstatic_opengl.h"
#include "gl_def.h"
#include "../rinterface.h"

extern "C"
{
#include "gldebughelp.h"
}

const float half = 0.501960784f;



void RStatic_OpenGL::InitializeWindow(RINTERFACE_WINDOW_CLASS window)
{
	auto truwindow = window;
	truwindow->setActive(true);
	GLenum glewError = glewInit();
	assert(!glewError);

	gldebughelpSETUPNOW();

	glEnable(GL_DEPTH_TEST); //enable depth buffer
	glDepthFunc(GL_LESS); //should always be GL_LESS
	glEnable(GL_CULL_FACE); //enable backface culling
	glEnable(GL_MULTISAMPLE); //
	glPointSize(8);
	

	glClearColor(0, half, half, 1.0f);


	BeforeRender(window);
	AfterRender(window);

	
}

void RStatic_OpenGL::BeforeRender(RINTERFACE_WINDOW_CLASS window)
{
	auto truwindow = window;
	//make sure the window context is active
	truwindow->setActive(true);

#if 1
	if (engine->rendersys[0]->available())
	{
		glClearColor(0.f, half, half, 0.0f);
	}
	else
	{
		glClearColor(half, 0.0f, 0.f, 0.0f);

	}
	glClear(GL_COLOR_BUFFER_BIT);
#endif
	glClear(GL_DEPTH_BUFFER_BIT);
}

void RStatic_OpenGL::AfterRender(RINTERFACE_WINDOW_CLASS window)
{
	//swap buffers?
	auto truwindow = window;
	truwindow->display();
	//need to respect sfml's drawing tools if we are using the sfml window class thing
}

void RStatic_OpenGL::Window_Resized(int new_width, int new_height)
{
	glViewport(0, 0, new_width, new_height);
}

RStatic_OpenGL* RStatic_OpenGL::Get()
{
	return dynamic_cast<RStatic_OpenGL*>(engine->rendersys.m_rstatic);
}
