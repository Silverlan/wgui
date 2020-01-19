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
#include <buffers/prosper_uniform_resizable_buffer.hpp>
#include <misc/memory_allocator.h>

#pragma optimize("",off)
static std::unique_ptr<WGUI> s_wgui = nullptr;
WGUI &WGUI::Open(prosper::Context &context,const std::weak_ptr<MaterialManager> &wpMatManager)
{
	s_wgui = nullptr;
	s_wgui = std::make_unique<WGUI>(context,wpMatManager);
	return GetInstance();
}
void WGUI::Close()
{
	if(s_wgui == nullptr)
		return;
	if(s_wgui->m_base.IsValid())
		delete s_wgui->m_base.get();
	s_wgui = nullptr;
	FontManager::Close();
	WIText::ClearTextBuffer();
	WIContextMenu::SetKeyBindHandler(nullptr,nullptr);
}
bool WGUI::IsOpen() {return s_wgui != nullptr;}
WGUI &WGUI::GetInstance() {return *s_wgui;}

WGUI::WGUI(prosper::Context &context,const std::weak_ptr<MaterialManager> &wpMatManager)
	: prosper::ContextObject(context),m_matManager(wpMatManager)
{}

WGUI::~WGUI() {}

double WGUI::GetDeltaTime() const {return m_tDelta;}

wgui::ShaderColored *WGUI::GetColoredShader() {return static_cast<wgui::ShaderColored*>(m_shaderColored.get());}
wgui::ShaderColoredRect *WGUI::GetColoredRectShader() {return static_cast<wgui::ShaderColoredRect*>(m_shaderColoredCheap.get());}
wgui::ShaderColoredLine *WGUI::GetColoredLineShader() {return static_cast<wgui::ShaderColoredLine*>(m_shaderColoredLine.get());}
wgui::ShaderText *WGUI::GetTextShader() {return static_cast<wgui::ShaderText*>(m_shaderText.get());}
wgui::ShaderTextRect *WGUI::GetTextRectShader() {return static_cast<wgui::ShaderTextRect*>(m_shaderTextCheap.get());}
wgui::ShaderTextRectColor *WGUI::GetTextRectColorShader() {return static_cast<wgui::ShaderTextRectColor*>(m_shaderTextCheapColor.get());}
wgui::ShaderTextured *WGUI::GetTexturedShader() {return static_cast<wgui::ShaderTextured*>(m_shaderTextured.get());}
wgui::ShaderTexturedRect *WGUI::GetTexturedRectShader() {return static_cast<wgui::ShaderTexturedRect*>(m_shaderTexturedCheap.get());}

static std::array<uint32_t,4> s_scissor = {0u,0,0u,0u};
void WGUI::SetScissor(uint32_t x,uint32_t y,uint32_t w,uint32_t h)
{
#ifdef WGUI_ENABLE_SANITY_EXCEPTIONS
	if(x +w > std::numeric_limits<int32_t>::max() || y +h > std::numeric_limits<int32_t>::max())
		throw std::logic_error("Scissor out of bounds!");
#endif
	s_scissor = {x,y,w,h};
}
void WGUI::GetScissor(uint32_t &x,uint32_t &y,uint32_t &w,uint32_t &h)
{
	x = s_scissor.at(0u);
	y = s_scissor.at(1u);
	w = s_scissor.at(2u);
	h = s_scissor.at(3u);
}

WGUI::ResultCode WGUI::Initialize(std::optional<Vector2i> resolution)
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
	auto &shaderManager = context.GetShaderManager();
	m_shaderColored = shaderManager.RegisterShader("wguicolored",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderColored(context,identifier);});
	m_shaderColoredCheap = shaderManager.RegisterShader("wguicolored_cheap",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderColoredRect(context,identifier);});
	m_shaderColoredLine = shaderManager.RegisterShader("wguicoloredline",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderColoredLine(context,identifier);});
	m_shaderText = shaderManager.RegisterShader("wguitext",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderText(context,identifier);});
	m_shaderTextCheap = shaderManager.RegisterShader("wguitext_cheap",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderTextRect(context,identifier);});
	m_shaderTextCheapColor = shaderManager.RegisterShader("wguitext_cheap_color",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderTextRectColor(context,identifier);});
	m_shaderTextured = shaderManager.RegisterShader("wguitextured",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderTextured(context,identifier);});
	m_shaderTexturedCheap = shaderManager.RegisterShader("wguitextured_cheap",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderTexturedRect(context,identifier);});
	
	if(wgui::Shader::DESCRIPTOR_SET.IsValid() == false)
		return ResultCode::ErrorInitializingShaders;

	// Font has to be loaded AFTER shaders have been initialized (Requires wguitext shader)
	auto font = FontManager::LoadFont("default","vera/VeraBd.ttf",14);
	FontManager::LoadFont("default_large","vera/VeraBd.ttf",18);
	FontManager::LoadFont("default_small","vera/VeraBd.ttf",10);
	FontManager::LoadFont("default_tiny","vera/VeraBd.ttf",8);
	if(font != nullptr)
		FontManager::SetDefaultFont(*font);
	else
		return ResultCode::FontNotFound;
	auto *base = new WIRoot;
	base->InitializeHandle();
	base->Initialize();
	m_base = base->GetHandle();
	Vector2i baseSize;
	if(resolution.has_value())
		baseSize = *resolution;
	else
		baseSize = {context.GetWindowWidth(),context.GetWindowHeight()};
	base->SetSize(baseSize);
	base->Setup();
	return ResultCode::Ok;
}

MaterialManager &WGUI::GetMaterialManager() {return *m_matManager.lock();}

void WGUI::SetCursor(GLFW::Cursor::Shape cursor)
{
	if(m_customCursor == nullptr && cursor == m_cursor)
		return;
	if(cursor == GLFW::Cursor::Shape::Hidden)
	{
		SetCursorInputMode(GLFW::CursorMode::Hidden);
		return;
	}
	else if(m_cursor == GLFW::Cursor::Shape::Hidden)
		SetCursorInputMode(GLFW::CursorMode::Normal);
	auto icursor = static_cast<uint32_t>(cursor) -static_cast<uint32_t>(GLFW::Cursor::Shape::Arrow);
	if(icursor > static_cast<uint32_t>(GLFW::Cursor::Shape::VResize))
		return;
	SetCursor(*m_cursors[icursor].get());
	m_cursor = cursor;
	m_customCursor = GLFW::CursorHandle();
}
void WGUI::SetCursor(GLFW::Cursor &cursor)
{
	if(m_customCursor.IsValid() && m_customCursor.get() == &cursor)
		return;
	GetContext().GetWindow().SetCursor(cursor);
	m_customCursor = cursor.GetHandle();
}
void WGUI::SetCursorInputMode(GLFW::CursorMode mode) {GetContext().GetWindow().SetCursorInputMode(mode);}
GLFW::Cursor::Shape WGUI::GetCursor() {return m_cursor;}
GLFW::CursorMode WGUI::GetCursorInputMode() {return GetContext().GetWindow().GetCursorInputMode();}
void WGUI::ResetCursor() {SetCursor(GLFW::Cursor::Shape::Arrow);}

void WGUI::GetMousePos(int &x,int &y)
{
	auto &window = GetContext().GetWindow();
	auto cursorPos = window.GetCursorPos();
	x = static_cast<int>(cursorPos.x);
	y = static_cast<int>(cursorPos.y);
}

void WGUI::SetHandleFactory(const std::function<std::shared_ptr<WIHandle>(WIBase&)> &handleFactory) {m_handleFactory = handleFactory;}

void WGUI::Think()
{
	while(!m_removeQueue.empty())
	{
		auto &hEl = m_removeQueue.front();
		if(hEl.IsValid())
			hEl->Remove();
		m_removeQueue.pop();
	}

	if(m_bGUIUpdateRequired && m_base.IsValid())
	{
		m_bGUIUpdateRequired = false;

		std::vector<WIHandle> tmpUpdateQueue = std::move(m_updateQueue);
		m_updateQueue.clear();
		for(auto &hEl : tmpUpdateQueue)
		{
			if(hEl.IsValid() == false)
				continue;
			if(hEl->IsVisible() == false)
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

	auto *el = GetCursorGUIElement(GetBaseElement(),[](WIBase *el) -> bool {return true;});
	while(el && el->GetCursor() == GLFW::Cursor::Shape::Default)
		el = el->GetParent();
	SetCursor(el ? el->GetCursor() : GLFW::Cursor::Shape::Arrow);
}

void WGUI::ScheduleElementForUpdate(WIBase &el)
{
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
			*it = {};
		}
	}
	m_updateQueue.push_back(el.GetHandle());
}

void WGUI::Draw()
{
	auto &context = GetContext();
	auto drawCmd = context.GetDrawCommandBuffer();
	WIBase::RENDER_ALPHA = 1.f;
	if(!m_base.IsValid())
		return;
	auto *p = m_base.get();
	if(p->IsVisible())
		p->Draw(p->GetWidth(),p->GetHeight());
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

WIBase *WGUI::GetBaseElement()
{
	return m_base.get();
}

bool WGUI::SetFocusedElement(WIBase *gui)
{
	auto *pPrevFocused = m_focused.get();
	auto &context = GetContext();
	auto &window = context.GetWindow();
	if(gui != NULL && m_focused.IsValid())
	{
		WIBase *root = GetBaseElement();
		WIBase *parent = m_focused.get();
		while(parent != NULL && parent != root && parent != m_focused.get())
		{
			if(parent->IsFocusTrapped())
				return false;
			parent = parent->GetParent();
		}
		m_focused->KillFocus(true);
	}
	if(gui == NULL)
	{
		window.SetCursorInputMode(GLFW::CursorMode::Hidden);
		m_focused = {};
		if(m_onFocusChangedCallback != nullptr)
			m_onFocusChangedCallback(pPrevFocused,m_focused.get());
		return true;
	}
	window.SetCursorInputMode(GLFW::CursorMode::Normal);
	m_focused = gui->GetHandle();
	if(m_onFocusChangedCallback != nullptr)
		m_onFocusChangedCallback(pPrevFocused,m_focused.get());
	return true;
}

WIBase *WGUI::GetFocusedElement() {return m_focused.get();}

WIBase *WGUI::FindByFilter(const std::function<bool(WIBase&)> &filter) const
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
	auto *root = const_cast<WGUI*>(this)->GetBaseElement();
	return root ? fIterate(*root) : nullptr;
}
WIBase *WGUI::FindByIndex(uint64_t index) const
{
	return FindByFilter([index](WIBase &el) -> bool {
		return el.GetIndex() == index;
	});
}

void WGUI::RemoveSafely(WIBase &gui)
{
	m_removeQueue.push(gui.GetHandle());
}

void WGUI::Remove(WIBase &gui)
{
	if(&gui == m_base.get())
		return;
	auto hEl = gui.GetHandle();
	if(m_removeCallback != NULL)
		m_removeCallback(gui);
	if(!hEl.IsValid())
		return;
	if(&gui == GetFocusedElement())
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
	if(!m_base.IsValid())
	{
		m_skin = nullptr;
		return;
	}
	m_base->ResetSkin();
	m_skin = nullptr;
}

void WGUI::SetSkin(std::string skin)
{
	if(!m_base.IsValid())
		return;
	StringToLower(skin);
	std::unordered_map<std::string,WISkin*>::iterator it = m_skins.find(skin);
	if(m_skin != nullptr && it != m_skins.end() && it->second == m_skin)
		return;
	m_base->ResetSkin();
	if(it == m_skins.end())
	{
		m_skin = nullptr;
		return;
	}
	m_skin = it->second;
	if(m_skin == nullptr)
		return;
	m_base->ApplySkin(m_skin);
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

bool WGUI::HandleJoystickInput(GLFW::Window &window,const GLFW::Joystick &joystick,uint32_t key,GLFW::KeyState state)
{
	return WIBase::__wiJoystickCallback(window,joystick,key,state);
}

bool WGUI::HandleMouseInput(GLFW::Window &window,GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	return WIBase::__wiMouseButtonCallback(window,button,state,mods);
}

bool WGUI::HandleKeyboardInput(GLFW::Window &window,GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods)
{
	return WIBase::__wiKeyCallback(window,key,scanCode,state,mods);
}

bool WGUI::HandleCharInput(GLFW::Window &window,unsigned int c)
{
	return WIBase::__wiCharCallback(window,c);
}

bool WGUI::HandleScrollInput(GLFW::Window &window,Vector2 offset)
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
		auto *pChild = check_children(el,x,y,bestZPos,condition);
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

WIBase *WGUI::GetGUIElement(WIBase *el,int32_t x,int32_t y,const std::function<bool(WIBase*)> &condition)
{
	return check_children(el ? el : GetBaseElement(),x,y,condition);
}

WIBase *WGUI::GetCursorGUIElement(WIBase *el,const std::function<bool(WIBase*)> &condition)
{
	int32_t x,y;
	GetMousePos(x,y);
	return GetGUIElement(el,x,y,condition);
}
#pragma optimize("",on)
