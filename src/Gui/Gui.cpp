#include "Gui.hpp"

#include <SDL.h>
#include <SDL_opengl.h>
#include <unistd.h>

#include "ElfReader.hpp"
#include "VarReader.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "implot.h"
#include "iostream"

std::mutex mtx;

Gui::Gui(PlotHandler* plotHandler, std::mutex* mtx) : plotHandler(plotHandler), mtx(mtx)
{
	std::string file("~/STMViewer/test/STMViewer_test/Debug/STMViewer_test.elf");

	ElfReader* elf = new ElfReader(file);
	std::vector<std::string> names({"sinTest", "cosTest", "test.ua", "test.ub"});
	addresses = elf->getVariableAddressBatch(names);
	std::cout << "Variables addresses: " << std::endl;
	for (auto& adr : addresses)
		std::cout << " - " << unsigned(adr) << std::endl;

	threadHandle = std::thread(&Gui::mainThread, this);

	plotHandler->addPlot("test1");
	plotHandler->getPlot("test1")->addSeries(std::string("testsin"), addresses[0]);
	plotHandler->getPlot("test1")->addSeries(std::string("testcos"), addresses[1]);

	plotHandler->addPlot("test2");
	plotHandler->getPlot("test2")->addSeries(std::string("testsin1"), addresses[2]);
	plotHandler->getPlot("test2")->addSeries(std::string("testcos2"), addresses[3]);

	plotHandler->addPlot("test3");
	plotHandler->getPlot("test3")->addSeries(std::string("testsin1"), addresses[0]);
	plotHandler->getPlot("test3")->addSeries(std::string("testcos2"), addresses[1]);
}

Gui::~Gui()
{
	if (threadHandle.joinable())
		threadHandle.join();
}

void Gui::mainThread()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return;
	}
	const char* glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_Window* window;

	window = SDL_CreateWindow("STMViewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 1000, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1);	// Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGuiStyle& style = ImGui::GetStyle();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	bool show_demo_window = false;
	bool p_open = true;

	std::cout << "STARTING IMGUI THREAD" << std::endl;

	while (!done)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		if (show_demo_window)
			ImPlot::ShowDemoWindow();

		ImGui::Begin("STMViewer", &p_open, 0);

		static int clicked = 0;
		if (ImGui::Button("Button"))
			clicked++;
		if (clicked & 1)
		{
			if (viewerState == state::STOP)
			{
				viewerState = state::RUN;
				plotHandler->setViewerState((PlotHandler::state)state::RUN);
			}
			ImGui::SameLine();
			ImGui::Text("RUN");
		}
		else
		{
			plotHandler->setViewerState((PlotHandler::state)state::STOP);
			viewerState = state::STOP;
			ImGui::SameLine();
			ImGui::Text("STOP");
		}

		plotHandler->drawAll();

		// drawMenu();
		ImGui::End();
		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
			SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
		}

		SDL_GL_SwapWindow(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Gui::drawMenu()
{
	ImGui::BeginMainMenuBar();

	if (ImGui::BeginMenu("File"))
	{
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}