/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/wgui.h"
#include "wgui/fontmanager.h"
#include "wgui/wibase.h"
#include "wgui/types/wiroot.h"
#include "wgui/wihandle.h"
#include <algorithm>
#include "wgui/wiskin.h"
#include "wgui/types/wiarrow.h"
#include "wgui/types/witext.h"
#include "wgui/shaders/wishader_colored.hpp"
#include "wgui/shaders/wishader_coloredline.hpp"
#include "wgui/shaders/wishader_text.hpp"
#include "wgui/shaders/wishader_textured.hpp"
#include "wgui/types/wicontextmenu.hpp"
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <buffers/prosper_buffer.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <prosper_command_buffer.hpp>
#include <prosper_window.hpp>
#include <prosper_render_pass.hpp>
#include <shader/prosper_pipeline_loader.hpp>
#include <buffers/prosper_uniform_resizable_buffer.hpp>
#pragma optimize("",off)
static std::unique_ptr<WGUI> s_wgui = nullptr;
prosper::SampleCountFlags WGUI::MSAA_SAMPLE_COUNT = prosper::SampleCountFlags::e1Bit;
WGUI &WGUI::Open(prosper::IPrContext &context,const std::weak_ptr<msys::MaterialManager> &wpMatManager)
{
	s_wgui = nullptr;
	s_wgui = std::make_unique<WGUI>(context,wpMatManager);
	return GetInstance();
}
void WGUI::ClearWindow(const prosper::Window &window)
{
	auto it = std::find_if(m_windowRootElements.begin(),m_windowRootElements.end(),[&window](const WindowRootPair &pair) {
		return pair.window.lock().get() == &window;
	});
	if(it == m_windowRootElements.end())
		return;
	auto &pair = *it;
	auto *elRoot = pair.rootElement.get();
	m_windowRootElements.erase(it);
	if(elRoot)
	{
		auto it = std::find_if(m_rootElements.begin(),m_rootElements.end(),[elRoot](const WIHandle &handle) {
			return handle.get() == elRoot;
		});
		if(it != m_rootElements.end())
			m_rootElements.erase(it);
	}
	delete elRoot;
}
void WGUI::Close()
{
	if(s_wgui == nullptr)
		return;
	std::queue<std::weak_ptr<const prosper::Window>> windows;
	for(auto &pair : s_wgui->m_windowRootElements)
	{
		if(pair.window.expired())
			continue;
		windows.push(pair.window);
	}
	
	while(!windows.empty())
	{
		auto &w = windows.front();
		if(w.expired() == false)
			s_wgui->ClearWindow(*windows.front().lock());
		windows.pop();
	}

	s_wgui = nullptr;
	FontManager::Close();
	WIText::ClearTextBuffer();
	WIContextMenu::SetKeyBindHandler(nullptr,nullptr);
}
bool WGUI::IsOpen() {return s_wgui != nullptr;}
WGUI &WGUI::GetInstance() {return *s_wgui;}

WGUI::WGUI(prosper::IPrContext &context,const std::weak_ptr<msys::MaterialManager> &wpMatManager)
	: prosper::ContextObject(context),m_matManager(wpMatManager)
{
	SetMaterialLoadHandler([this](const std::string &path) -> Material* {
		return m_matManager.lock()->LoadAsset(path).get();
	});
}

WGUI::~WGUI() {}

void WGUI::RegisterElement(WIBase &el,const std::string &className,WIBase *parent)
{
	el.SetIndex(m_nextGuiElementIndex++);
	el.InitializeHandle();
	if(parent != nullptr)
		el.SetParent(parent);
	else
	{
		auto *elBase = GetBaseElement();
		if(elBase)
			el.SetParent(elBase);
	}
	el.m_class = className;
	el.AddStyleClass(el.GetClass());
	el.Initialize();
	if(m_createCallback != nullptr)
		m_createCallback(el);
}

void WGUI::SetMaterialLoadHandler(const std::function<Material*(const std::string&)> &handler) {m_materialLoadHandler = handler;}
const std::function<Material*(const std::string&)> &WGUI::GetMaterialLoadHandler() const {return m_materialLoadHandler;}

double WGUI::GetDeltaTime() const {return m_tDelta;}

wgui::ShaderColored *WGUI::GetColoredShader() {return static_cast<wgui::ShaderColored*>(m_shaderColored.get());}
wgui::ShaderColoredRect *WGUI::GetColoredRectShader() {return static_cast<wgui::ShaderColoredRect*>(m_shaderColoredCheap.get());}
wgui::ShaderColoredLine *WGUI::GetColoredLineShader() {return static_cast<wgui::ShaderColoredLine*>(m_shaderColoredLine.get());}
wgui::ShaderText *WGUI::GetTextShader() {return static_cast<wgui::ShaderText*>(m_shaderText.get());}
wgui::ShaderTextRect *WGUI::GetTextRectShader() {return static_cast<wgui::ShaderTextRect*>(m_shaderTextCheap.get());}
wgui::ShaderTextRectColor *WGUI::GetTextRectColorShader() {return static_cast<wgui::ShaderTextRectColor*>(m_shaderTextCheapColor.get());}
wgui::ShaderTextured *WGUI::GetTexturedShader() {return static_cast<wgui::ShaderTextured*>(m_shaderTextured.get());}
wgui::ShaderTexturedRect *WGUI::GetTexturedRectShader() {return static_cast<wgui::ShaderTexturedRect*>(m_shaderTexturedCheap.get());}
wgui::ShaderTexturedRectExpensive *WGUI::GetTexturedRectExpensiveShader() {return static_cast<wgui::ShaderTexturedRectExpensive*>(m_shaderTexturedExpensive.get());}
wgui::ShaderStencil *WGUI::GetStencilShader() {return static_cast<wgui::ShaderStencil*>(m_shaderStencil.get());}

void wgui::DrawState::SetScissor(uint32_t x,uint32_t y,uint32_t w,uint32_t h)
{
#ifdef WGUI_ENABLE_SANITY_EXCEPTIONS
	if(x +w > std::numeric_limits<int32_t>::max() || y +h > std::numeric_limits<int32_t>::max())
		throw std::logic_error("Scissor out of bounds!");
#endif
	scissor = {x,y,w,h};
}
void wgui::DrawState::GetScissor(uint32_t &x,uint32_t &y,uint32_t &w,uint32_t &h)
{
	x = scissor.at(0u);
	y = scissor.at(1u);
	w = scissor.at(2u);
	h = scissor.at(3u);
}

WGUI::ResultCode WGUI::Initialize(std::optional<Vector2i> resolution,std::optional<std::string> fontFileName)
{
	if(!FontManager::Initialize())
		return ResultCode::UnableToInitializeFontManager;
	m_cursors.reserve(6);
	m_cursors.push_back(GLFW::Cursor::Create(GLFW::Cursor::Shape::Arrow));
	m_cursors.push_back(GLFW::Cursor::Create(GLFW::Cursor::Shape::IBeam));
	m_cursors.push_back(GLFW::Cursor::Create(GLFW::Cursor::Shape::Crosshair));
	m_cursors.push_back(GLFW::Cursor::Create(GLFW::Cursor::Shape::Hand));
	m_cursors.push_back(GLFW::Cursor::Create(GLFW::Cursor::Shape::HResize));
	m_cursors.push_back(GLFW::Cursor::Create(GLFW::Cursor::Shape::VResize));

	m_time.Update();
	m_tLastThink = static_cast<double>(m_time());

	auto &context = GetContext();
	prosper::ImageFormatPropertiesQuery query {
		prosper::ImageCreateFlags::None,prosper::Format::R8G8B8A8_UNorm,
		prosper::ImageType::e2D,prosper::ImageTiling::Optimal,
		prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit
	};
	auto limits = context.GetPhysicalDeviceImageFormatProperties(query);
	if(limits.has_value())
	{
		MSAA_SAMPLE_COUNT = limits->sampleCount;
		if(MSAA_SAMPLE_COUNT > prosper::SampleCountFlags::e8Bit)
			MSAA_SAMPLE_COUNT = prosper::SampleCountFlags::e8Bit;
	}
	else
		MSAA_SAMPLE_COUNT = prosper::SampleCountFlags::e1Bit;

	auto rpCreateInfo = context.GetWindow().GetStagingRenderPass().GetCreateInfo();
	for(auto &att : rpCreateInfo.attachments)
		att.sampleCount = MSAA_SAMPLE_COUNT;
	m_msaaRenderPass = context.CreateRenderPass(rpCreateInfo);

	auto &shaderManager = context.GetShaderManager();
	shaderManager.RegisterShader("wguicolored",[](prosper::IPrContext &context,const std::string &identifier) {return new wgui::ShaderColored(context,identifier);});
	shaderManager.RegisterShader("wguicolored_cheap",[](prosper::IPrContext &context,const std::string &identifier) {return new wgui::ShaderColoredRect(context,identifier);});
	shaderManager.RegisterShader("wguicoloredline",[](prosper::IPrContext &context,const std::string &identifier) {return new wgui::ShaderColoredLine(context,identifier);});
	shaderManager.RegisterShader("wguitext",[](prosper::IPrContext &context,const std::string &identifier) {return new wgui::ShaderText(context,identifier);});
	shaderManager.RegisterShader("wguitext_cheap",[](prosper::IPrContext &context,const std::string &identifier) {return new wgui::ShaderTextRect(context,identifier);});
	shaderManager.RegisterShader("wguitext_cheap_color",[](prosper::IPrContext &context,const std::string &identifier) {return new wgui::ShaderTextRectColor(context,identifier);});
	shaderManager.RegisterShader("wguitextured",[](prosper::IPrContext &context,const std::string &identifier) {return new wgui::ShaderTextured(context,identifier);});
	shaderManager.RegisterShader("wguitextured_cheap",[](prosper::IPrContext &context,const std::string &identifier) {return new wgui::ShaderTexturedRect(context,identifier);});
	shaderManager.RegisterShader("wguitextured_expensive",[](prosper::IPrContext &context,const std::string &identifier) {return new wgui::ShaderTexturedRectExpensive(context,identifier);});
	shaderManager.RegisterShader("wguistencil",[](prosper::IPrContext &context,const std::string &identifier) {return new wgui::ShaderStencil(context,identifier);});

	m_shaderColored = shaderManager.GetShader("wguicolored");
	m_shaderColoredCheap = shaderManager.GetShader("wguicolored_cheap");
	m_shaderColoredLine = shaderManager.GetShader("wguicoloredline");
	m_shaderText = shaderManager.GetShader("wguitext");
	m_shaderTextCheap = shaderManager.GetShader("wguitext_cheap");
	m_shaderTextCheapColor = shaderManager.GetShader("wguitext_cheap_color");
	m_shaderTextured = shaderManager.GetShader("wguitextured");
	m_shaderTexturedCheap = shaderManager.GetShader("wguitextured_cheap");
	m_shaderTexturedExpensive = shaderManager.GetShader("wguitextured_expensive");
	m_shaderStencil = shaderManager.GetShader("wguistencil");

	GetContext().GetPipelineLoader().Flush();
	
	if(wgui::Shader::DESCRIPTOR_SET.IsValid() == false)
		return ResultCode::ErrorInitializingShaders;

	// Font has to be loaded AFTER shaders have been initialized (Requires wguitext shader)
	if(!fontFileName.has_value())
		fontFileName = "vera/VeraBd.ttf";
	auto font = FontManager::LoadFont("default",*fontFileName,14);
	FontManager::LoadFont("default_large",*fontFileName,18);
	FontManager::LoadFont("default_small",*fontFileName,10);
	FontManager::LoadFont("default_tiny",*fontFileName,8);
	if(font != nullptr)
		FontManager::SetDefaultFont(*font);
	else
		return ResultCode::FontNotFound;
	auto *base = AddBaseElement(&context.GetWindow());
	if(resolution.has_value())
		base->SetSize(*resolution);
	return ResultCode::Ok;
}

prosper::IRenderPass &WGUI::GetMsaaRenderPass() const {return *m_msaaRenderPass;}

msys::MaterialManager &WGUI::GetMaterialManager() {return *m_matManager.lock();}

void WGUI::SetCursor(GLFW::Cursor::Shape cursor,prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return;
	auto *pair = FindWindowRootPair(*window);
	if(!pair)
		return;
	if(pair->customCursor == nullptr && cursor == pair->cursor)
		return;
	if(cursor == GLFW::Cursor::Shape::Hidden)
	{
		SetCursorInputMode(GLFW::CursorMode::Hidden,window);
		return;
	}
	else if(pair->cursor == GLFW::Cursor::Shape::Hidden)
		SetCursorInputMode(GLFW::CursorMode::Normal,window);
	auto icursor = static_cast<uint32_t>(cursor) -static_cast<uint32_t>(GLFW::Cursor::Shape::Arrow);
	if(icursor > static_cast<uint32_t>(GLFW::Cursor::Shape::VResize))
		return;
	SetCursor(*m_cursors[icursor].get(),window);
	pair->cursor = cursor;
	pair->customCursor = GLFW::CursorHandle();
}
void WGUI::SetCursor(GLFW::Cursor &cursor,prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return;
	auto *pair = FindWindowRootPair(*window);
	if(!pair)
		return;
	if(pair->customCursor.IsValid() && pair->customCursor.get() == &cursor)
		return;
	(*window)->SetCursor(cursor);
	pair->customCursor = cursor.GetHandle();
}
void WGUI::SetCursorInputMode(GLFW::CursorMode mode,prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return;
	(*window)->SetCursorInputMode(mode);
}
GLFW::Cursor::Shape WGUI::GetCursor(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return GLFW::Cursor::Shape::Default;
	auto *pair = FindWindowRootPair(*window);
	return pair ? pair->cursor : GLFW::Cursor::Shape::Default;
}
GLFW::CursorMode WGUI::GetCursorInputMode(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return GLFW::CursorMode::Normal;
	return (*window)->GetCursorInputMode();
}
void WGUI::ResetCursor(prosper::Window *window) {SetCursor(GLFW::Cursor::Shape::Arrow,window);}

void WGUI::GetMousePos(int &x,int &y,const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
	{
		x = 0;
		y = 0;
		return;
	}
	auto cursorPos = (*window)->GetCursorPos();
	x = static_cast<int>(cursorPos.x);
	y = static_cast<int>(cursorPos.y);
}

prosper::Window *WGUI::FindFocusedWindow()
{
	auto *pair = FindFocusedWindowRootPair();
	if(!pair)
		return nullptr;
	return const_cast<prosper::Window*>(pair->window.lock().get());
}
WIBase *WGUI::FindFocusedWindowRootElement()
{
	auto *pair = FindFocusedWindowRootPair();
	if(!pair)
		return nullptr;
	return pair->rootElement.get();
}
prosper::Window *WGUI::FindWindowUnderCursor()
{
	auto *pair = FindWindowRootPairUnderCursor();
	if(!pair)
		return nullptr;
	return const_cast<prosper::Window*>(pair->window.lock().get());
}
WIBase *WGUI::FindRootElementUnderCursor()
{
	auto *pair = FindWindowRootPairUnderCursor();
	if(!pair)
		return nullptr;
	return pair->rootElement.get();
}

void WGUI::Think()
{
	while(!m_removeQueue.empty())
	{
		auto &hEl = m_removeQueue.front();
		if(hEl.IsValid())
			hEl->Remove();
		m_removeQueue.pop();
	}

	if(m_bGUIUpdateRequired)
	{
		m_bGUIUpdateRequired = false;

		std::vector<WIHandle> tmpUpdateQueue = std::move(m_updateQueue);
		m_updateQueue.clear();
		for(auto &hEl : tmpUpdateQueue)
		{
			if(hEl.IsValid() == false)
				continue;
			if(umath::is_flag_set(hEl.get()->m_stateFlags,WIBase::StateFlags::UpdateScheduledBit) == false)
				continue; // Update is no longer scheduled; Ignore this element
			if(hEl->IsVisible() == false && hEl->ShouldThinkIfInvisible() == false)
			{
				// We don't want to update hidden elements, but we have to remember that we have to
				// update them later!
				m_updateQueue.push_back(hEl);
				continue;
			}
			hEl->Update();
		}
	}

	m_time.Update();
	auto t = m_time();
	m_tDelta = static_cast<double>(t -m_tLastThink);
	m_tLastThink = static_cast<double>(t);

	for(auto i=decltype(m_thinkingElements.size()){0u};i<m_thinkingElements.size();)
	{
		auto &hEl = m_thinkingElements.at(i);
		if(hEl.IsValid() == false)
		{
			m_thinkingElements.erase(m_thinkingElements.begin() +i);
			continue;
		}
		auto *pEl = hEl.get();
		pEl->Think();
		
		// Calling 'Think' may have removed the element from the thinking elements,
		// so we must only increment i if that wasn't the case.
		if(i < m_thinkingElements.size() && m_thinkingElements.at(i).get() == pEl)
			++i;
	}

	auto *window = FindWindowUnderCursor();
	auto *elBase = window ? GetBaseElement(window) : nullptr;
	if(!elBase)
		return;
	auto *el = GetCursorGUIElement(elBase,[](WIBase *el) -> bool {return el->GetCursor() != GLFW::Cursor::Shape::Default;},window);
	while(el && el->GetCursor() == GLFW::Cursor::Shape::Default)
		el = el->GetParent();
	SetCursor(el ? el->GetCursor() : GLFW::Cursor::Shape::Arrow,window);
}

void WGUI::ScheduleElementForUpdate(WIBase &el)
{
	if(umath::is_flag_set(el.m_stateFlags,WIBase::StateFlags::UpdateScheduledBit))
		return;
	m_bGUIUpdateRequired = true;
	umath::set_flag(el.m_stateFlags,WIBase::StateFlags::UpdateScheduledBit,true);

	if(umath::is_flag_set(el.m_stateFlags,WIBase::StateFlags::UpdateScheduledBit))
	{
		// Element is already scheduled for an updated, but we'll want to make sure it's at the end of the list, so we'll
		// remove the previous entry! We'll iterate backwards because it's likely the last update-schedule was very recently.
		auto it = std::find_if(m_updateQueue.rbegin(),m_updateQueue.rend(),[&el](const WIHandle &hEl) -> bool {
			return hEl.get() == &el;
		});
		if(it != m_updateQueue.rend())
		{
			// We don't erase the element from the vector because that would re-order the entire vector, which is expensive.
			// Instead we'll just invalidate it, the vector will be cleared during the next ::Think anyway!
			*it = WIHandle{};
		}
	}
	m_updateQueue.push_back(el.GetHandle());
}

void WGUI::Draw(WIBase &el,prosper::IRenderPass &rp,prosper::IFramebuffer &fb,prosper::ICommandBuffer &drawCmd)
{
	auto baseCmd = drawCmd.shared_from_this();
	if(el.IsVisible() == false)
		return;
	el.Draw(el.GetWidth(),el.GetHeight(),baseCmd);
}

void WGUI::Draw(const prosper::Window &window,prosper::ICommandBuffer &drawCmd)
{
	auto rt = window.GetStagingRenderTarget();
	auto *el = GetBaseElement(&window);
	if(rt && el)
		Draw(*el,rt->GetRenderPass(),rt->GetFramebuffer(),drawCmd);
}

WIBase *WGUI::Create(std::string classname,WIBase *parent)
{
	StringToLower(classname);
	WGUIClassMap *map = GetWGUIClassMap();
	WIBase*(*factory)(void) = map->FindFactory(classname);
	if(factory != NULL)
	{
		WIBase *p = factory();
		p->m_class = classname;
		if(parent != NULL)
			p->SetParent(parent);
		return p;
	}
	return NULL;
}

WIBase *WGUI::GetBaseElement(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return nullptr;
	auto *pair = FindWindowRootPair(*window);
	return pair ? pair->rootElement.get() : nullptr;
}

WIBase *WGUI::AddBaseElement(const prosper::Window *window)
{
	if(window)
	{
		auto *el = GetBaseElement(window);
		if(el)
			return el;
		el = AddBaseElement();
		if(!el)
			return nullptr;
		auto size = (*window)->GetSize();
		el->SetSize(size);
		m_windowRootElements.push_back({window->shared_from_this(),el->GetHandle()});
		const_cast<prosper::Window*>(window)->AddClosedListener([this,window]() {
			ClearWindow(*window);
		});
		el->AddCallback("OnRemove",FunctionCallback<void>::Create([this,window]() {
			ClearWindow(*window);
		}));
		return el;
	}
	auto *el = new WIRoot;
	el->InitializeHandle();
	el->Initialize();
	m_rootElements.push_back(el->GetHandle());
	el->Setup();
	return el;
}

static std::unordered_set<WIBase*> g_exemptFromFocus;
void WGUI::ClearFocus(WIBase &el)
{
	auto *window = el.GetRootWindow();
	if(!window)
		return;
	auto *pair = FindWindowRootPair(*window);
	if(!pair)
		return;
	std::function<void(WIBase&)> fAddElement = nullptr;
	fAddElement = [&fAddElement](WIBase &el) {
		g_exemptFromFocus.insert(&el);
		**el.m_bHasFocus = false;
		el.m_stateFlags &= ~WIBase::StateFlags::TrapFocusBit;
		for(auto &hChild : *el.GetChildren())
		{
			if(!hChild.IsValid())
				continue;
			fAddElement(*hChild);
		}
	};
	fAddElement(el);

	if(g_exemptFromFocus.find(pair->elFocused.get()) != g_exemptFromFocus.end())
		pair->elFocused = WIHandle{};
	for(auto it=pair->focusTrapStack.begin();it!=pair->focusTrapStack.end();)
	{
		auto &hEl = *it;
		if(g_exemptFromFocus.find(hEl.get()) == g_exemptFromFocus.end())
		{
			++it;
			continue;
		}
		it = pair->focusTrapStack.erase(it);
	}

	pair->RestoreTrappedFocus(); // g_exemptFromFocus will ensure that during this call none of the elements will regain focus
	g_exemptFromFocus.clear();
}

bool is_valid(const WIHandle &hEl);
void WGUI::WindowRootPair::RestoreTrappedFocus(WIBase *elRef)
{
	for(auto it=focusTrapStack.rbegin();it!=focusTrapStack.rend();)
	{
		auto &hEl = *it;
		++it;
		if(!is_valid(hEl))
			it = std::deque<WIHandle>::reverse_iterator(focusTrapStack.erase(it.base()));
		else if(hEl.get() != elRef && hEl->IsVisible())
		{
			hEl->RequestFocus();
			break;
		}
	}
}

bool WGUI::SetFocusedElement(WIBase *gui,prosper::Window *window)
{
	if(g_exemptFromFocus.find(gui) != g_exemptFromFocus.end())
		return false;
	window = GetWindow(window);
	if(!window)
		return false;
	auto *pair = FindWindowRootPair(*window);
	if(!pair)
		return false;
	auto *pPrevFocused = pair->elFocused.get();
	auto &context = GetContext();
	if(gui != NULL && pair->elFocused.IsValid())
	{
		WIBase *root = GetBaseElement(window);
		WIBase *parent = pair->elFocused.get();
		while(parent != NULL && parent != root && parent != pair->elFocused.get())
		{
			if(parent->IsFocusTrapped())
				return false;
			parent = parent->GetParent();
		}
		pair->elFocused->KillFocus(true);
	}
	if(gui == NULL)
	{
		(*window)->SetCursorInputMode(GLFW::CursorMode::Hidden);
		pair->elFocused = WIHandle{};
		if(m_onFocusChangedCallback != nullptr)
			m_onFocusChangedCallback(pPrevFocused,pair->elFocused.get());
		return true;
	}
	(*window)->SetCursorInputMode(GLFW::CursorMode::Normal);

	pair->elFocused = gui->GetHandle();
	++pair->focusCount;
	if(m_onFocusChangedCallback != nullptr)
		m_onFocusChangedCallback(pPrevFocused,pair->elFocused.get());
	return true;
}

void WGUI::IncrementFocusCount(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return;
	auto *pair = FindWindowRootPair(*window);
	if(!pair)
		return;
	++pair->focusCount;
}

uint32_t WGUI::GetFocusCount(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return 0;
	auto *pair = FindWindowRootPair(*window);
	return pair ? pair->focusCount : 0u;
}

WIBase *WGUI::GetFocusedElement(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return nullptr;
	auto *pair = FindWindowRootPair(*window);
	return pair ? pair->elFocused.get() : nullptr;
}

WIBase *WGUI::FindByFilter(const std::function<bool(WIBase&)> &filter,const prosper::Window *window) const
{
	std::function<WIBase*(WIBase&)> fIterate = nullptr;
	fIterate = [&filter,&fIterate](WIBase &el) -> WIBase* {
		for(auto &hChild : *el.GetChildren())
		{
			if(hChild.IsValid() == false)
				continue;
			if(filter(*hChild.get()))
				return hChild.get();
			auto *child = fIterate(*hChild.get());
			if(child)
				return child;
		}
		return nullptr;
	};
	if(!window)
	{
		for(auto &pair : m_windowRootElements)
		{
			if(pair.rootElement.IsExpired() || pair.window.expired())
				continue;
			auto *el = FindByFilter(filter,pair.window.lock().get());
			if(el)
				return el;
		}
		return nullptr;
	}
	auto *root = const_cast<WGUI*>(this)->GetBaseElement(window);
	return root ? fIterate(*root) : nullptr;
}
WIBase *WGUI::FindByIndex(uint64_t index) const
{
	return FindByFilter([index](WIBase &el) -> bool {
		return el.GetIndex() == index;
	});
}

prosper::Window *WGUI::FindWindow(WIBase &elRoot)
{
	auto it = std::find_if(m_windowRootElements.begin(),m_windowRootElements.end(),[&elRoot](const WindowRootPair &pair) {
		return pair.rootElement.get() == &elRoot;
	});
	auto *window = (it != m_windowRootElements.end()) ? const_cast<prosper::Window*>(it->window.lock().get()) : nullptr;
	return window && window->IsValid() ? window : nullptr;
}

void WGUI::RemoveSafely(WIBase &gui)
{
	m_removeQueue.push(gui.GetHandle());
}

void WGUI::Remove(WIBase &gui)
{
	if(typeid(gui) == typeid(WIRoot))
	{
		auto it = std::find_if(m_rootElements.begin(),m_rootElements.end(),[&gui](const WIHandle &h) {
			return h.get() == &gui;
		});
		if(it != m_rootElements.end())
			m_rootElements.erase(it);
		delete &gui;
		return;
	}
	auto hEl = gui.GetHandle();
	if(m_removeCallback != NULL)
		m_removeCallback(gui);
	if(!hEl.IsValid())
		return;
	if(&gui == GetFocusedElement(gui.GetRootWindow()))
	{
		gui.TrapFocus(false);
		gui.KillFocus();
	}
	delete &gui;
}

void WGUI::ClearSkin()
{
	if(m_skin == nullptr)
		return;
	auto *elBase = GetBaseElement();
	if(!elBase)
	{
		m_skin = nullptr;
		return;
	}
	for(auto &hEl : m_rootElements)
	{
		if(!hEl.IsValid())
			continue;
		hEl->ResetSkin();
	}
	m_skin = nullptr;
}

void WGUI::SetSkin(std::string skin)
{
	for(auto &hEl : m_rootElements)
	{
		if(hEl.IsValid() == false)
			continue;
		StringToLower(skin);
		std::unordered_map<std::string,WISkin*>::iterator it = m_skins.find(skin);
		if(m_skin != nullptr && it != m_skins.end() && it->second == m_skin)
			return;
		hEl->ResetSkin();
		if(it == m_skins.end())
		{
			m_skin = nullptr;
			return;
		}
		m_skin = it->second;
		if(m_skin == nullptr)
			return;
		hEl->ApplySkin(m_skin);
	}
}
WISkin *WGUI::GetSkin() {return m_skin;}
WISkin *WGUI::GetSkin(std::string name)
{
	std::unordered_map<std::string,WISkin*>::iterator it = m_skins.find(name);
	if(it == m_skins.end())
		return NULL;
	return it->second;
}
std::string WGUI::GetSkinName()
{
	if(m_skin == NULL)
		return "";
	return m_skin->m_identifier;
}

void WGUI::SetCreateCallback(const std::function<void(WIBase&)> &onCreate) {m_createCallback = onCreate;}
void WGUI::SetRemoveCallback(const std::function<void(WIBase&)> &onRemove) {m_removeCallback = onRemove;}
void WGUI::SetFocusCallback(const std::function<void(WIBase*,WIBase*)> &onFocusChanged) {m_onFocusChangedCallback = onFocusChanged;}

prosper::Window *WGUI::GetWindow(prosper::Window *window)
{
	if(window && window->IsValid() == false)
		window = nullptr;
	else if(!window)
	{
		window = &GetContext().GetWindow();
		if(window && window->IsValid() == false)
			window = nullptr;
	}
	return window;
}
WGUI::WindowRootPair *WGUI::FindWindowRootPair(const prosper::Window &window)
{
	auto it = std::find_if(m_windowRootElements.begin(),m_windowRootElements.end(),[&window](const WindowRootPair &pair) {
		return !pair.window.expired() && pair.window.lock().get() == &window;
	});
	return (it != m_windowRootElements.end()) ? &*it : nullptr;
}

WGUI::WindowRootPair *WGUI::FindFocusedWindowRootPair()
{
	for(auto &pair : m_windowRootElements)
	{
		auto window = pair.window.lock();
		if(!window || window->IsValid() == false || (*window)->IsFocused() == false)
			continue;
		return &pair;
	}
	return nullptr;
}
WGUI::WindowRootPair *WGUI::FindWindowRootPairUnderCursor()
{
	WGUI::WindowRootPair *pairCandidate = nullptr;
	for(auto &pair : m_windowRootElements)
	{
		auto window = pair.window.lock();
		if(!window || window->IsValid() == false)
			continue;
		auto pos = (*window)->GetCursorPos();
		auto size = (*window)->GetSize();
		if(pos.x >= 0 && pos.y >= 0 && pos.x < size.x && pos.y < size.y)
		{
			pairCandidate = &pair;
			if((*window)->IsFocused())
				return pairCandidate;
		}
	}
	return pairCandidate;
}

bool WGUI::HandleJoystickInput(prosper::Window &window,const GLFW::Joystick &joystick,uint32_t key,GLFW::KeyState state)
{
	return WIBase::__wiJoystickCallback(window,joystick,key,state);
}

bool WGUI::HandleMouseInput(prosper::Window &window,GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	return WIBase::__wiMouseButtonCallback(window,button,state,mods);
}

bool WGUI::HandleKeyboardInput(prosper::Window &window,GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods)
{
	return WIBase::__wiKeyCallback(window,key,scanCode,state,mods);
}

bool WGUI::HandleCharInput(prosper::Window &window,unsigned int c)
{
	return WIBase::__wiCharCallback(window,c);
}

bool WGUI::HandleScrollInput(prosper::Window &window,Vector2 offset)
{
	return WIBase::__wiScrollCallback(window,offset);
}

static WIBase *check_children(WIBase *gui,int x,int y,int32_t &bestZPos,const std::function<bool(WIBase*)> &condition)
{
	if(gui->IsVisible() == false || gui->PosInBounds(x,y) == false)
		return nullptr;
	auto children = *gui->GetChildren();

	// Children are sorted by zpos, with the highest zpos being at the end, so we have
	// to reverse iterate (since higher zpos has higher priority).
	for(auto it=children.rbegin();it!=children.rend();++it)
	{
		auto &hnd = *it;
		if(hnd.IsValid() == false)
			continue;
		auto *el = hnd.get();
		int childBestZpos = -1;
		auto *pChild = check_children(el,x,y,childBestZpos,condition);
		if(pChild)
			return pChild;
	}

	// None of our children were viable, check if we're viable
	if(condition == nullptr || condition(gui))
	{
		auto zPos = gui->GetZPos();
		if(zPos > bestZPos)
			bestZPos = zPos;
		return gui;
	}
	return nullptr;
}
static WIBase *check_children(WIBase *gui,int x,int y,const std::function<bool(WIBase*)> &condition)
{
	int32_t bestZPos = -1;
	return check_children(gui,x,y,bestZPos,condition);
}

WIBase *WGUI::GetGUIElement(WIBase *el,int32_t x,int32_t y,const std::function<bool(WIBase*)> &condition,const prosper::Window *window)
{
	return check_children(el ? el : GetBaseElement(window),x,y,condition);
}

WIBase *WGUI::GetCursorGUIElement(WIBase *el,const std::function<bool(WIBase*)> &condition,const prosper::Window *window)
{
	int32_t x,y;
	GetMousePos(x,y,window);
	return GetGUIElement(el,x,y,condition,window);
}
#pragma optimize("",on)
