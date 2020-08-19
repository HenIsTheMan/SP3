#include "App.h"

extern float dt;
extern bool endLoop;
extern int optimalWinXPos;
extern int optimalWinYPos;
extern int optimalWinWidth;
extern int optimalWinHeight;
extern int winWidth;
extern int winHeight;

const GLFWvidmode* App::mode = nullptr;
GLFWwindow* App::win = nullptr;

//#define RENDER_DEPTH_MAP

App::App():
	fullscreen(false),
	elapsedTime(0.f),
	lastFrameTime(0.f),
	scene(),
	FBORefIDs(),
	texRefIDs(),
	RBORefIDs()
{
	if(!InitAPI(win)){
		(void)puts("Failed to init API\n");
		endLoop = true;
	}
	(void)Init();
}

App::~App(){
	glDeleteTextures(sizeof(texRefIDs) / sizeof(texRefIDs[0]), texRefIDs);
	glDeleteRenderbuffers(sizeof(RBORefIDs) / sizeof(RBORefIDs[0]), RBORefIDs);
	glDeleteFramebuffers(sizeof(FBORefIDs) / sizeof(FBORefIDs[0]), FBORefIDs);
	glfwTerminate(); //Clean/Del all GLFW's resources that were allocated
}

bool App::Init(){
	mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

	glGenFramebuffers(sizeof(FBORefIDs) / sizeof(FBORefIDs[0]), FBORefIDs);
	glGenTextures(sizeof(texRefIDs) / sizeof(texRefIDs[0]), texRefIDs);
	glGenRenderbuffers(sizeof(RBORefIDs) / sizeof(RBORefIDs[0]), RBORefIDs);

	glBindFramebuffer(GL_FRAMEBUFFER, FBORefIDs[(int)FBO::GeoPass]);
		for(Tex i = Tex::Pos; i <= Tex::Reflection; ++i){
			int currTexRefID;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &currTexRefID);
			glBindTexture(GL_TEXTURE_2D, texRefIDs[(int)i]);
				glTexImage2D(GL_TEXTURE_2D, 0, i == Tex::Reflection ? GL_RGBA : GL_RGBA16F, 2048, 2048, 0, GL_RGBA, i == Tex::Reflection ? GL_UNSIGNED_BYTE : GL_FLOAT, NULL); //??
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (int)i, GL_TEXTURE_2D, texRefIDs[(int)i], 0);
			glBindTexture(GL_TEXTURE_2D, currTexRefID);
		}

		glBindRenderbuffer(GL_RENDERBUFFER, RBORefIDs[0]);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 2048, 2048);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBORefIDs[0]);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
			(void)printf(STR(FBO::GeoPass));
			(void)puts(" is incomplete!\n");
			return false;
		}
	glBindFramebuffer(GL_FRAMEBUFFER, FBORefIDs[(int)FBO::LightingPass]);
		for(Tex i = Tex::Lit; i <= Tex::Bright; ++i){
			int currTexRefID;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &currTexRefID);
			glBindTexture(GL_TEXTURE_2D, texRefIDs[(int)i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 2048, 2048, 0, GL_RGBA, GL_FLOAT, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + int(i) - int(Tex::Lit), GL_TEXTURE_2D, texRefIDs[(int)i], 0);
			glBindTexture(GL_TEXTURE_2D, currTexRefID);
		}

		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
			(void)printf(STR(FBO::LightingPass));
			(void)puts(" is incomplete!\n");
			return false;
		}
	for(FBO i = FBO::PingPong0; i <= FBO::PingPong1; ++i){
		glBindFramebuffer(GL_FRAMEBUFFER, FBORefIDs[(int)i]);
			int currTexRefID;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &currTexRefID);
			glBindTexture(GL_TEXTURE_2D, texRefIDs[int(Tex::PingPong0) + int(FBO::PingPong1) - int(i)]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 2048, 2048, 0, GL_RGBA, GL_FLOAT, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texRefIDs[int(Tex::PingPong0) + int(FBO::PingPong1) - int(i)], 0);
			glBindTexture(GL_TEXTURE_2D, currTexRefID);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, FBORefIDs[(int)FBO::DepthD]);
		glBindTexture(GL_TEXTURE_2D, texRefIDs[(int)Tex::DepthD]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texRefIDs[(int)Tex::DepthD], 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, FBORefIDs[(int)FBO::DepthS]);
		glBindTexture(GL_TEXTURE_2D, texRefIDs[(int)Tex::DepthS]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texRefIDs[(int)Tex::DepthS], 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	(void)InitOptions();
	(void)scene.Init();
	glPointSize(10.f);
	glLineWidth(5.f);

	return true;
}

bool App::InitOptions() const{
	//Stencil buffer usually contains 8 bits per stencil value that amts to 256 diff stencil values per pixel
	//Use stencil buffer operations to write to the stencil buffer when rendering frags (read stencil values in the same or following frame(s) to pass or discard frags based on their stencil value)
	glEnable(GL_STENCIL_TEST); //Discard frags based on frags of other drawn objs in the scene

	glEnable(GL_DEPTH_TEST); //Done in screen space after the frag shader has run and after the stencil test //(pass ? frag is rendered and depth buffer is updated with new depth value : frag is discarded)

	glEnable(GL_BLEND); //Colour resulting from blend eqn replaces prev colour stored in the colour buffer

	glEnable(GL_CULL_FACE); //Specify vertices in CCW winding order so front faces are rendered in CW order while... //Actual winding order is calculated at the rasterization stage after the vertex shader has run //Vertices are then seen from the viewer's POV

	glEnable(GL_PROGRAM_POINT_SIZE);
	//glEnable(GL_MULTISAMPLE); //Enabled by default on most OpenGL drivers //Multisample buffer (stores a given amt of sample pts per pixel) for MSAA
	//Multisampling algs are implemented in the rasterizer (combination of all algs and processes that transform vertices of primitives into frags, frags are bound by screen resolution unlike vertices so there is almost nvr a 1-on-1 mapping between... and it must decide at what screen coords will each frag of each interpolated vertex end up at) of the OpenGL drivers
	//Poor screen resolution leads to aliasing as the limited amt of screen pixels causes some pixels to not be rendered along an edge of a fig //Colour output is stored directly in the framebuffer if pixel is fully covered and blending is disabled

	//glEnable(GL_FRAMEBUFFER_SRGB); //Colours from sRGB colour space are gamma corrected after each frag shader run before they are stored in colour buffers of all framebuffers
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	return true;
}

void App::Update(){
	if(glfwWindowShouldClose(App::win)){
		endLoop = true;
		return;
	}

	float currFrameTime = (float)glfwGetTime();
	dt = currFrameTime - lastFrameTime;
	lastFrameTime = currFrameTime;

	elapsedTime += dt;
	static float toggleFullscreenBT = 0.f;
	if(Key(VK_F1) && toggleFullscreenBT <= elapsedTime){
		if(fullscreen){
			glfwSetWindowMonitor(win, 0, optimalWinXPos, optimalWinYPos, optimalWinWidth, optimalWinHeight, GLFW_DONT_CARE);
			fullscreen = false;
		} else{
			glfwSetWindowMonitor(win, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
			fullscreen = true;
		}
		toggleFullscreenBT = elapsedTime + .5f;
	}

	scene.Update();
}

void App::PreRender() const{
	//glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); //Frags update stencil buffer with their ref value when... //++params and options??
	//glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_DST_ALPHA, GL_DST_ALPHA);
	//glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_SUBTRACT);
}

void App::Render(){
	glViewport(0, 0, 1024, 1024);

	glBindFramebuffer(GL_FRAMEBUFFER, FBORefIDs[(int)FBO::DepthD]);
	glClear(GL_DEPTH_BUFFER_BIT);
	scene.DepthRender(0);

	glBindFramebuffer(GL_FRAMEBUFFER, FBORefIDs[(int)FBO::DepthS]);
	glClear(GL_DEPTH_BUFFER_BIT);
	scene.DepthRender(1);

	#ifdef RENDER_DEPTH_MAP
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(1.f, 0.82f, 0.86f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	scene.DefaultRender(texRefIDs[(int)Tex::DepthD], texRefIDs[(int)Tex::DepthD]);
	#else
	glViewport(0, 0, winWidth, winHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(1.f, 0.82f, 0.86f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	scene.ForwardRender(texRefIDs[(int)Tex::DepthD], texRefIDs[(int)Tex::DepthS]);
	#endif
}

void App::PostRender() const{
	glfwSwapBuffers(win); //Swap the large 2D colour buffer containing colour values for each pixel in GLFW's window
	glfwPollEvents(); //Check for triggered events and call corresponding functions registered via callback methods
}