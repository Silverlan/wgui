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
#include "wgui/shaders/witexturedsubrect.hpp"
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

import pragma.string.unicode;

static std::unique_ptr<WGUI> s_wgui = nullptr;
prosper::SampleCountFlags WGUI::MSAA_SAMPLE_COUNT = prosper::SampleCountFlags::e1Bit;
WGUI &WGUI::Open(prosper::IPrContext &context, const std::weak_ptr<msys::MaterialManager> &wpMatManager)
{
	s_wgui = nullptr;
	s_wgui = std::make_unique<WGUI>(context, wpMatManager);
	return GetInstance();
}
void WGUI::ClearWindow(const prosper::Window &window)
{
	auto it = std::find_if(m_windowRootElements.begin(), m_windowRootElements.end(), [&window](const WIHandle &hRoot) { return hRoot.IsValid() && static_cast<const WIRoot *>(hRoot.get())->GetWindow() == &window; });
	if(it == m_windowRootElements.end())
		return;
	auto *elRoot = static_cast<WIRoot *>(it->get());
	m_windowRootElements.erase(it);
	if(elRoot) {
		auto it = std::find_if(m_rootElements.begin(), m_rootElements.end(), [elRoot](const WIHandle &handle) { return handle.get() == elRoot; });
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
	for(auto &hEl : s_wgui->m_windowRootElements) {
		if(hEl.expired())
			continue;
		auto &root = *static_cast<WIRoot *>(hEl.get());
		auto &window = root.GetWindowPtr();
		if(window.expired())
			continue;
		windows.push(window);
	}

	while(!windows.empty()) {
		auto &w = windows.front();
		if(w.expired() == false)
			s_wgui->ClearWindow(*windows.front().lock());
		windows.pop();
	}

	s_wgui = nullptr;
	FontManager::Close();
	WIText::ClearTextBuffer();
	WIContextMenu::SetKeyBindHandler(nullptr, nullptr);
}
bool WGUI::IsOpen() { return s_wgui != nullptr; }
WGUI &WGUI::GetInstance() { return *s_wgui; }

WGUI::WGUI(prosper::IPrContext &context, const std::weak_ptr<msys::MaterialManager> &wpMatManager) : prosper::ContextObject(context), m_matManager(wpMatManager)
{
	SetMaterialLoadHandler([this](const std::string &path) -> Material * { return m_matManager.lock()->LoadAsset(path).get(); });
}

WGUI::~WGUI() {}

void WGUI::RegisterElement(WIBase &el, const std::string &className, WIBase *parent)
{
	el.SetIndex(m_nextGuiElementIndex++);
	el.InitializeHandle();
	if(parent != nullptr)
		el.SetParent(parent);
	else {
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

void WGUI::SetMaterialLoadHandler(const std::function<Material *(const std::string &)> &handler) { m_materialLoadHandler = handler; }
const std::function<Material *(const std::string &)> &WGUI::GetMaterialLoadHandler() const { return m_materialLoadHandler; }

double WGUI::GetDeltaTime() const { return m_tDelta; }

wgui::ShaderColored *WGUI::GetColoredShader() { return static_cast<wgui::ShaderColored *>(m_shaderColored.get()); }
wgui::ShaderColoredRect *WGUI::GetColoredRectShader() { return static_cast<wgui::ShaderColoredRect *>(m_shaderColoredCheap.get()); }
wgui::ShaderColoredLine *WGUI::GetColoredLineShader() { return static_cast<wgui::ShaderColoredLine *>(m_shaderColoredLine.get()); }
wgui::ShaderText *WGUI::GetTextShader() { return static_cast<wgui::ShaderText *>(m_shaderText.get()); }
wgui::ShaderTextRect *WGUI::GetTextRectShader() { return static_cast<wgui::ShaderTextRect *>(m_shaderTextCheap.get()); }
wgui::ShaderTextRectColor *WGUI::GetTextRectColorShader() { return static_cast<wgui::ShaderTextRectColor *>(m_shaderTextCheapColor.get()); }
wgui::ShaderTextured *WGUI::GetTexturedShader() { return static_cast<wgui::ShaderTextured *>(m_shaderTextured.get()); }
wgui::ShaderTexturedRect *WGUI::GetTexturedRectShader() { return static_cast<wgui::ShaderTexturedRect *>(m_shaderTexturedCheap.get()); }
wgui::ShaderTexturedRectExpensive *WGUI::GetTexturedRectExpensiveShader() { return static_cast<wgui::ShaderTexturedRectExpensive *>(m_shaderTexturedExpensive.get()); }
wgui::ShaderStencil *WGUI::GetStencilShader() { return static_cast<wgui::ShaderStencil *>(m_shaderStencil.get()); }
wgui::ShaderTexturedSubRect *WGUI::GetTexturedSubRectShader() { return static_cast<wgui::ShaderTexturedSubRect *>(m_shaderTexturedSubRect.get()); }

void wgui::DrawState::SetScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
#ifdef WGUI_ENABLE_SANITY_EXCEPTIONS
	if(x + w > std::numeric_limits<int32_t>::max() || y + h > std::numeric_limits<int32_t>::max())
		throw std::logic_error("Scissor out of bounds!");
#endif
	scissor = {x, y, w, h};
}
void wgui::DrawState::GetScissor(uint32_t &x, uint32_t &y, uint32_t &w, uint32_t &h)
{
	x = scissor.at(0u);
	y = scissor.at(1u);
	w = scissor.at(2u);
	h = scissor.at(3u);
}
bool wgui::detail::UpdatePriority::operator()(const UpdateInfo &h0, const UpdateInfo &h1) const { return h0.depth < h1.depth; }

WGUI::ResultCode WGUI::Initialize(std::optional<Vector2i> resolution, std::optional<std::string> fontFileName) { return Initialize(resolution, fontFileName, {}); }
WGUI::ResultCode WGUI::Initialize(std::optional<Vector2i> resolution, std::optional<std::string> fontFileName, const std::vector<std::string> &fallbackFontFileNames)
{
	if(!FontManager::Initialize())
		return ResultCode::UnableToInitializeFontManager;
	m_cursors.reserve(6);
	m_cursors.push_back(pragma::platform::Cursor::Create(pragma::platform::Cursor::Shape::Arrow));
	m_cursors.push_back(pragma::platform::Cursor::Create(pragma::platform::Cursor::Shape::IBeam));
	m_cursors.push_back(pragma::platform::Cursor::Create(pragma::platform::Cursor::Shape::Crosshair));
	m_cursors.push_back(pragma::platform::Cursor::Create(pragma::platform::Cursor::Shape::Hand));
	m_cursors.push_back(pragma::platform::Cursor::Create(pragma::platform::Cursor::Shape::HResize));
	m_cursors.push_back(pragma::platform::Cursor::Create(pragma::platform::Cursor::Shape::VResize));

	m_time.Update();
	m_tLastThink = static_cast<double>(m_time());

	auto &context = GetContext();
	prosper::ImageFormatPropertiesQuery query {prosper::ImageCreateFlags::None, prosper::Format::R8G8B8A8_UNorm, prosper::ImageType::e2D, prosper::ImageTiling::Optimal, prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit};
	auto limits = context.GetPhysicalDeviceImageFormatProperties(query);
	if(limits.has_value()) {
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
	shaderManager.RegisterShader("wguicolored", [](prosper::IPrContext &context, const std::string &identifier) { return new wgui::ShaderColored(context, identifier); });
	shaderManager.RegisterShader("wguicolored_cheap", [](prosper::IPrContext &context, const std::string &identifier) { return new wgui::ShaderColoredRect(context, identifier); });
	shaderManager.RegisterShader("wguicoloredline", [](prosper::IPrContext &context, const std::string &identifier) { return new wgui::ShaderColoredLine(context, identifier); });
	shaderManager.RegisterShader("wguitext", [](prosper::IPrContext &context, const std::string &identifier) { return new wgui::ShaderText(context, identifier); });
	shaderManager.RegisterShader("wguitext_cheap", [](prosper::IPrContext &context, const std::string &identifier) { return new wgui::ShaderTextRect(context, identifier); });
	shaderManager.RegisterShader("wguitext_cheap_color", [](prosper::IPrContext &context, const std::string &identifier) { return new wgui::ShaderTextRectColor(context, identifier); });
	shaderManager.RegisterShader("wguitextured", [](prosper::IPrContext &context, const std::string &identifier) { return new wgui::ShaderTextured(context, identifier); });
	shaderManager.RegisterShader("wguitextured_cheap", [](prosper::IPrContext &context, const std::string &identifier) { return new wgui::ShaderTexturedRect(context, identifier); });
	shaderManager.RegisterShader("wguitextured_expensive", [](prosper::IPrContext &context, const std::string &identifier) { return new wgui::ShaderTexturedRectExpensive(context, identifier); });
	shaderManager.RegisterShader("wguistencil", [](prosper::IPrContext &context, const std::string &identifier) { return new wgui::ShaderStencil(context, identifier); });
	shaderManager.RegisterShader("wguisubtexturedrect", [](prosper::IPrContext &context, const std::string &identifier) { return new wgui::ShaderTexturedSubRect(context, identifier); });

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
	m_shaderTexturedSubRect = shaderManager.GetShader("wguisubtexturedrect");

	GetContext().GetPipelineLoader().Flush();

	for(auto &fileName : fallbackFontFileNames)
		FontManager::AddFallbackFont(fileName);

	// Font has to be loaded AFTER shaders have been initialized (Requires wguitext shader)
	if(!fontFileName.has_value())
		fontFileName = "dejavu/DejaVuSans-Bold.ttf";
	FontSettings settings {};
	settings.fontSize = 14;
	auto font = FontManager::LoadFont("default", *fontFileName, settings);
	settings.fontSize = 18;
	FontManager::LoadFont("default_large", *fontFileName, settings);
	settings.fontSize = 10;
	FontManager::LoadFont("default_small", *fontFileName, settings);
	settings.fontSize = 8;
	FontManager::LoadFont("default_tiny", *fontFileName, settings);
	if(font != nullptr)
		FontManager::SetDefaultFont(*font);
	else
		return ResultCode::FontNotFound;
	auto *base = AddBaseElement(&context.GetWindow());
	if(resolution.has_value())
		base->SetSize(*resolution);
	return ResultCode::Ok;
}

prosper::IRenderPass &WGUI::GetMsaaRenderPass() const { return *m_msaaRenderPass; }

msys::MaterialManager &WGUI::GetMaterialManager() { return *m_matManager.lock(); }

void WGUI::SetCursor(pragma::platform::Cursor::Shape cursor, prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return;
	auto *elRoot = FindWindowRootElement(*window);
	if(!elRoot)
		return;
	if(elRoot->GetMainCustomCursor() == nullptr && cursor == elRoot->GetMainCursor())
		return;
	if(cursor == pragma::platform::Cursor::Shape::Hidden) {
		SetCursorInputMode(pragma::platform::CursorMode::Hidden, window);
		return;
	}
	else if(elRoot->GetMainCursor() == pragma::platform::Cursor::Shape::Hidden)
		SetCursorInputMode(pragma::platform::CursorMode::Normal, window);
	auto icursor = static_cast<uint32_t>(cursor) - static_cast<uint32_t>(pragma::platform::Cursor::Shape::Arrow);
	if(icursor > static_cast<uint32_t>(pragma::platform::Cursor::Shape::VResize))
		return;
	SetCursor(*m_cursors[icursor].get(), window);
	elRoot->SetMainCursor(cursor);
	elRoot->SetMainCustomCursor(pragma::platform::CursorHandle());
}
void WGUI::SetCursor(pragma::platform::Cursor &cursor, prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return;
	auto *elRoot = FindWindowRootElement(*window);
	if(!elRoot)
		return;
	auto &customCursor = elRoot->GetMainCustomCursor();
	if(customCursor.IsValid() && customCursor.get() == &cursor)
		return;
	(*window)->SetCursor(cursor);
	elRoot->SetMainCustomCursor(cursor.GetHandle());
}
void WGUI::SetCursorInputMode(pragma::platform::CursorMode mode, prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return;
	(*window)->SetCursorInputMode(mode);
}
pragma::platform::Cursor::Shape WGUI::GetCursor(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return pragma::platform::Cursor::Shape::Default;
	auto *elRoot = FindWindowRootElement(*window);
	return elRoot ? elRoot->GetMainCursor() : pragma::platform::Cursor::Shape::Default;
}
pragma::platform::CursorMode WGUI::GetCursorInputMode(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return pragma::platform::CursorMode::Normal;
	return (*window)->GetCursorInputMode();
}
void WGUI::ResetCursor(prosper::Window *window) { SetCursor(pragma::platform::Cursor::Shape::Arrow, window); }

void WGUI::GetMousePos(int &x, int &y, const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window) {
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
	auto *elRoot = FindFocusedWindowRootElement();
	if(!elRoot)
		return nullptr;
	return const_cast<prosper::Window *>(elRoot->GetWindow());
}
WIRoot *WGUI::FindFocusedWindowRootElement()
{
	for(auto &hEl : m_windowRootElements) {
		auto window = hEl.IsValid() ? static_cast<WIRoot *>(hEl.get())->GetWindow() : nullptr;
		if(!window || (*window)->IsFocused() == false)
			continue;
		return static_cast<WIRoot *>(hEl.get());
	}
	return nullptr;
}
prosper::Window *WGUI::FindWindowUnderCursor()
{
	auto *elRoot = FindWindowRootElementUnderCursor();
	if(!elRoot)
		return nullptr;
	return const_cast<prosper::Window *>(elRoot->GetWindow());
}
WIRoot *WGUI::FindRootElementUnderCursor() { return FindWindowRootElementUnderCursor(); }
size_t WGUI::GetLastThinkIndex() const { return m_thinkIndex; }
void WGUI::BeginDraw()
{
	WGUI::GetInstance().SetLockedForDrawing(true);
	FontManager::UpdateDirtyFonts();
}
void WGUI::EndDraw() { WGUI::GetInstance().SetLockedForDrawing(false); }
void WGUI::Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd)
{
	while(!m_removeQueue.empty()) {
		auto &hEl = m_removeQueue.front();
		if(hEl.IsValid())
			hEl->Remove();
		m_removeQueue.pop();
	}

	if(m_bGUIUpdateRequired) {
		m_bGUIUpdateRequired = false;

		while(!m_updateQueue.empty()) {
			auto updateInfo = m_updateQueue.top();
			m_updateQueue.pop();
			auto &hEl = updateInfo.element;
			m_currentUpdateDepth = updateInfo.depth;
			if(hEl.valid()) {
				auto *el = const_cast<WIBase *>(hEl.get());
				if(updateInfo.element.valid() && umath::is_flag_set(updateInfo.element->m_stateFlags, WIBase::StateFlags::UpdateScheduledBit)) {
					auto &el = *updateInfo.element;
					if(el.IsVisible() || el.ShouldThinkIfInvisible()) {
						el.m_lastThinkUpdateIndex = m_thinkIndex;
						el.Update();
						umath::set_flag(el.m_stateFlags, WIBase::StateFlags::UpdateScheduledBit, false);
					}
					else {
						umath::set_flag(el.m_stateFlags, WIBase::StateFlags::ScheduleUpdateOnVisible, true);
						umath::set_flag(el.m_stateFlags, WIBase::StateFlags::UpdateScheduledBit, false);
					}
				}
			}
		}
		m_currentUpdateDepth = {};
	}

	m_time.Update();
	auto t = m_time();
	m_tDelta = static_cast<double>(t - m_tLastThink);
	m_tLastThink = static_cast<double>(t);

	for(auto i = decltype(m_thinkingElements.size()) {0u}; i < m_thinkingElements.size();) {
		auto &hEl = m_thinkingElements.at(i);
		if(hEl.IsValid() == false) {
			m_thinkingElements.erase(m_thinkingElements.begin() + i);
			continue;
		}
		auto *pEl = hEl.get();
		pEl->Think(drawCmd);

		// Calling 'Think' may have removed the element from the thinking elements,
		// so we must only increment i if that wasn't the case.
		if(i < m_thinkingElements.size() && m_thinkingElements.at(i).get() == pEl)
			++i;
	}

	auto *window = FindWindowUnderCursor();
	auto *elBase = window ? GetBaseElement(window) : nullptr;
	if(!elBase) {
		++m_thinkIndex;
		return;
	}
	auto *el = GetCursorGUIElement(elBase, [](WIBase *el) -> bool { return el->GetCursor() != pragma::platform::Cursor::Shape::Default; }, window);
	while(el && el->GetCursor() == pragma::platform::Cursor::Shape::Default)
		el = el->GetParent();
	SetCursor(el ? el->GetCursor() : pragma::platform::Cursor::Shape::Arrow, window);

	++m_thinkIndex;
}

void WGUI::ScheduleElementForUpdate(WIBase &el, bool force)
{
	if(!force && umath::is_flag_set(el.m_stateFlags, WIBase::StateFlags::UpdateScheduledBit))
		return;
	m_bGUIUpdateRequired = true;
	umath::set_flag(el.m_stateFlags, WIBase::StateFlags::UpdateScheduledBit, true);
	m_updateQueue.push({el.GetHandle(), el.GetDepth()});
}

void WGUI::Draw(WIBase &el, prosper::IRenderPass &rp, prosper::IFramebuffer &fb, prosper::ICommandBuffer &drawCmd)
{
	auto baseCmd = drawCmd.shared_from_this();
	if(el.IsVisible() == false)
		return;
	el.Draw(el.GetWidth(), el.GetHeight(), baseCmd);
}

void WGUI::Draw(const prosper::Window &window, prosper::ICommandBuffer &drawCmd)
{
	auto rt = window.GetStagingRenderTarget();
	auto *el = GetBaseElement(&window);
	if(rt && el)
		Draw(*el, rt->GetRenderPass(), rt->GetFramebuffer(), drawCmd);
}

WIBase *WGUI::Create(std::string classname, WIBase *parent)
{
	StringToLower(classname);
	WGUIClassMap *map = GetWGUIClassMap();
	WIBase *(*factory)(void) = map->FindFactory(classname);
	if(factory != NULL) {
		WIBase *p = factory();
		p->m_class = classname;
		if(parent != NULL)
			p->SetParent(parent);
		return p;
	}
	return NULL;
}

WIRoot *WGUI::GetBaseElement(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return nullptr;
	return FindWindowRootElement(*window);
}

WIBase *WGUI::GetFocusedElement(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return GetFocusedElement(static_cast<WIRoot *>(nullptr));
	auto *el = FindWindowRootElement(*window);
	if(!el)
		return nullptr;
	return GetFocusedElement(el);
}
uint32_t WGUI::GetFocusCount(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return GetFocusCount(static_cast<WIRoot *>(nullptr));
	auto *el = FindWindowRootElement(*window);
	if(!el)
		return 0;
	return GetFocusCount(el);
}
void WGUI::IncrementFocusCount(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return IncrementFocusCount(static_cast<WIRoot *>(nullptr));
	auto *el = FindWindowRootElement(*window);
	if(!el)
		return;
	return IncrementFocusCount(el);
}

WIRoot *WGUI::AddBaseElement(const prosper::Window *window)
{
	if(window) {
		auto *el = GetBaseElement(window);
		if(el)
			return el;
		el = AddBaseElement();
		if(!el)
			return nullptr;
		auto size = (*window)->GetSize();
		el->SetSize(size);
		el->SetWindow(window->shared_from_this());
		m_windowRootElements.push_back(el->GetHandle());
		const_cast<prosper::Window *>(window)->AddClosedListener([this, window]() { ClearWindow(*window); });
		el->AddCallback("OnRemove", FunctionCallback<void>::Create([this, window]() { ClearWindow(*window); }));
		return el;
	}
	auto *el = new WIRoot;
	el->InitializeHandle();
	el->Initialize();
	m_rootElements.push_back(el->GetHandle());
	el->Setup();
	return el;
}

static std::unordered_set<WIBase *> g_exemptFromFocus;
void WGUI::SetFocusEnabled(const prosper::Window &window, bool enabled)
{
	auto *elRoot = const_cast<WGUI *>(this)->FindWindowRootElement(window);
	if(!elRoot)
		return;
	elRoot->SetFocusEnabled(enabled);
}
bool WGUI::IsFocusEnabled(const prosper::Window &window) const
{
	auto *elRoot = const_cast<WGUI *>(this)->FindWindowRootElement(window);
	if(!elRoot)
		return false;
	return elRoot->IsFocusEnabled();
}
void WGUI::ClearFocus(WIBase &el)
{
	auto *window = el.GetRootWindow();
	if(!window)
		return;
	auto *elRoot = FindWindowRootElement(*window);
	if(!elRoot)
		return;
	std::function<void(WIBase &)> fAddElement = nullptr;
	fAddElement = [&fAddElement](WIBase &el) {
		g_exemptFromFocus.insert(&el);
		**el.m_bHasFocus = false;
		el.m_stateFlags &= ~WIBase::StateFlags::TrapFocusBit;
		for(auto &hChild : *el.GetChildren()) {
			if(!hChild.IsValid())
				continue;
			fAddElement(*hChild);
		}
	};
	fAddElement(el);

	auto *focusedElement = elRoot->GetFocusedElement();
	if(g_exemptFromFocus.find(focusedElement) != g_exemptFromFocus.end())
		elRoot->SetFocusedElement(nullptr);
	auto &focusTrapStack = elRoot->GetFocusTrapStack();
	for(auto it = focusTrapStack.begin(); it != focusTrapStack.end();) {
		auto &hEl = *it;
		if(g_exemptFromFocus.find(hEl.get()) == g_exemptFromFocus.end()) {
			++it;
			continue;
		}
		it = focusTrapStack.erase(it);
	}

	elRoot->RestoreTrappedFocus(); // g_exemptFromFocus will ensure that during this call none of the elements will regain focus
	g_exemptFromFocus.clear();
}

bool WGUI::SetFocusedElement(WIBase *gui, WIRoot *optElRoot)
{
	if(g_exemptFromFocus.find(gui) != g_exemptFromFocus.end())
		return false;
	auto *elRoot = GetRootElement(optElRoot);
	if(!elRoot)
		return false;
	auto *window = elRoot->GetWindow();
	auto *pPrevFocused = elRoot->GetFocusedElement();
	auto &context = GetContext();
	if(gui != NULL && pPrevFocused != nullptr) {
		WIBase *root = elRoot;
		WIBase *parent = pPrevFocused;
		while(parent != NULL && parent != root && parent != elRoot->GetFocusedElement()) {
			if(parent->IsFocusTrapped())
				return false;
			parent = parent->GetParent();
		}
		elRoot->GetFocusedElement()->KillFocus(true);
	}
	if(gui == NULL) {
		if(window)
			(*window)->SetCursorInputMode(pragma::platform::CursorMode::Hidden);
		elRoot->SetFocusedElement(nullptr);
		if(m_onFocusChangedCallback != nullptr)
			m_onFocusChangedCallback(pPrevFocused, elRoot->GetFocusedElement());
		return true;
	}
	if(window)
		(*window)->SetCursorInputMode(pragma::platform::CursorMode::Normal);

	elRoot->SetFocusedElement(gui);
	elRoot->SetFocusCount(elRoot->GetFocusCount() + 1);
	if(m_onFocusChangedCallback != nullptr)
		m_onFocusChangedCallback(pPrevFocused, elRoot->GetFocusedElement());
	return true;
}

void WGUI::IncrementFocusCount(WIRoot *optElRoot)
{
	optElRoot = GetRootElement(optElRoot);
	if(!optElRoot)
		return;
	optElRoot->SetFocusCount(optElRoot->GetFocusCount() + 1);
}

uint32_t WGUI::GetFocusCount(const WIRoot *optElRoot)
{
	optElRoot = GetRootElement(optElRoot);
	if(!optElRoot)
		return 0;
	return optElRoot->GetFocusCount();
}

WIBase *WGUI::GetFocusedElement(const WIRoot *optElRoot)
{
	optElRoot = GetRootElement(optElRoot);
	if(!optElRoot)
		return nullptr;
	return optElRoot->IsFocusEnabled() ? const_cast<WIBase *>(optElRoot->GetFocusedElement()) : nullptr;
}

WIBase *WGUI::FindByFilter(const std::function<bool(WIBase &)> &filter, const prosper::Window *window) const
{
	std::function<WIBase *(WIBase &)> fIterate = nullptr;
	fIterate = [&filter, &fIterate](WIBase &el) -> WIBase * {
		for(auto &hChild : *el.GetChildren()) {
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
	if(!window) {
		for(auto &hEl : m_windowRootElements) {
			if(hEl.expired())
				continue;
			auto *window = static_cast<const WIRoot *>(hEl.get())->GetWindow();
			if(!window)
				continue;
			auto *el = FindByFilter(filter, window);
			if(el)
				return el;
		}
		return nullptr;
	}
	auto *root = const_cast<WGUI *>(this)->GetBaseElement(window);
	return root ? fIterate(*root) : nullptr;
}
WIBase *WGUI::FindByIndex(uint64_t index) const
{
	return FindByFilter([index](WIBase &el) -> bool { return el.GetIndex() == index; });
}

prosper::Window *WGUI::FindWindow(WIBase &elRoot)
{
	auto it = std::find_if(m_windowRootElements.begin(), m_windowRootElements.end(), [&elRoot](const WIHandle &hEl) { return hEl.get() == &elRoot; });
	auto *window = (it != m_windowRootElements.end()) ? static_cast<WIRoot *>(it->get())->GetWindow() : nullptr;
	return window && window->IsValid() ? window : nullptr;
}

void WGUI::RemoveSafely(WIBase &gui) { m_removeQueue.push(gui.GetHandle()); }

void WGUI::Remove(WIBase &gui)
{
	if(typeid(gui) == typeid(WIRoot)) {
		auto it = std::find_if(m_rootElements.begin(), m_rootElements.end(), [&gui](const WIHandle &h) { return h.get() == &gui; });
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
	if(&gui == GetFocusedElement(gui.GetRootWindow())) {
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
	if(!elBase) {
		m_skin = nullptr;
		return;
	}
	for(auto &hEl : m_rootElements) {
		if(!hEl.IsValid())
			continue;
		hEl->ResetSkin();
	}
	m_skin = nullptr;
}

WISkin *WGUI::RegisterSkin(std::string id, std::unique_ptr<WISkin> &&skin)
{
	ustring::to_lower(id);
	auto it = m_skins.find(id);
	if(it != m_skins.end()) {
		auto curSkin = std::move(it->second);
		m_skins.erase(it);

		std::vector<WIHandle> elements;
		std::function<void(WIBase &)> findSkinElements = nullptr;
		findSkinElements = [&findSkinElements, &curSkin, &elements](WIBase &el) {
			for(auto &hChild : *el.GetChildren()) {
				if(!hChild.IsValid())
					continue;
				findSkinElements(*hChild.get());
			}
			if(el.m_skin != curSkin.get())
				return;
			if(elements.size() == elements.capacity())
				elements.reserve(elements.size() * 1.5 + 100);
			elements.push_back(el.GetHandle());
			el.m_skin = nullptr;
		};
		for(auto &hEl : WGUI::GetInstance().GetBaseElements()) {
			if(!hEl.IsValid())
				continue;
			findSkinElements(const_cast<WIBase &>(*hEl.get()));
		}
		skin->SetIdentifier(id);
		auto *pSkin = skin.get();
		m_skins[id] = std::move(skin);

		if(m_skin == curSkin.get())
			ClearSkin();

		for(auto &hEl : elements) {
			if(!hEl.IsValid())
				continue;
			hEl->SetSkin(id);
			// hEl->RefreshSkin();
		}
		return pSkin;
	}
	auto *pSkin = skin.get();
	m_skins[id] = std::move(skin);
	return pSkin;
}

void WGUI::SetSkin(std::string skin)
{
	for(auto &hEl : m_rootElements) {
		if(hEl.IsValid() == false)
			continue;
		ustring::to_lower(skin);
		auto it = m_skins.find(skin);
		if(m_skin != nullptr && it != m_skins.end() && it->second.get() == m_skin)
			return;
		hEl->ResetSkin();
		if(it == m_skins.end()) {
			m_skin = nullptr;
			return;
		}
		m_skin = it->second.get();
		if(m_skin == nullptr)
			return;
		hEl->ApplySkin(m_skin);
	}
}
WISkin *WGUI::GetSkin() { return m_skin; }
WISkin *WGUI::GetSkin(std::string name)
{
	auto it = m_skins.find(name);
	if(it == m_skins.end())
		return NULL;
	return it->second.get();
}
std::string WGUI::GetSkinName()
{
	if(m_skin == NULL)
		return "";
	return m_skin->m_identifier;
}
void WGUI::ClearSkins()
{
	m_skin = nullptr;
	m_skins.clear();
}

void WGUI::SetCreateCallback(const std::function<void(WIBase &)> &onCreate) { m_createCallback = onCreate; }
void WGUI::SetRemoveCallback(const std::function<void(WIBase &)> &onRemove) { m_removeCallback = onRemove; }
void WGUI::SetFocusCallback(const std::function<void(WIBase *, WIBase *)> &onFocusChanged) { m_onFocusChangedCallback = onFocusChanged; }

WIRoot *WGUI::GetRootElement(WIRoot *elRoot)
{
	if(!elRoot) {
		auto &window = GetContext().GetWindow();
		elRoot = FindWindowRootElement(window);
	}
	return elRoot;
}
prosper::Window *WGUI::GetWindow(prosper::Window *window)
{
	if(window && window->IsValid() == false)
		window = nullptr;
	else if(!window) {
		window = &GetContext().GetWindow();
		if(window && window->IsValid() == false)
			window = nullptr;
	}
	return window;
}
WIRoot *WGUI::FindWindowRootElement(const prosper::Window &window)
{
	auto it = std::find_if(m_windowRootElements.begin(), m_windowRootElements.end(), [&window](const WIHandle &hEl) {
		if(hEl.expired())
			return false;
		auto *ptrWindow = static_cast<const WIRoot *>(hEl.get())->GetWindow();
		return ptrWindow ? ptrWindow == &window : false;
	});
	return (it != m_windowRootElements.end()) ? static_cast<WIRoot *>(it->get()) : nullptr;
}

WIRoot *WGUI::FindWindowRootElementUnderCursor()
{
	WIRoot *rootCandidate = nullptr;
	for(auto &hEl : m_windowRootElements) {
		auto *elRoot = static_cast<WIRoot *>(hEl.get());
		if(!elRoot)
			continue;
		auto *window = elRoot->GetWindow();
		if(!window || window->IsValid() == false)
			continue;
		auto pos = (*window)->GetCursorPos();
		auto size = (*window)->GetSize();
		if(pos.x >= 0 && pos.y >= 0 && pos.x < size.x && pos.y < size.y) {
			rootCandidate = elRoot;
			if((*window)->IsFocused())
				return rootCandidate;
		}
	}
	return rootCandidate;
}

bool WGUI::HandleJoystickInput(prosper::Window &window, const pragma::platform::Joystick &joystick, uint32_t key, pragma::platform::KeyState state) { return WIBase::__wiJoystickCallback(window, joystick, key, state); }

bool WGUI::HandleMouseInput(prosper::Window &window, pragma::platform::MouseButton button, pragma::platform::KeyState state, pragma::platform::Modifier mods) { return WIBase::__wiMouseButtonCallback(window, button, state, mods); }

bool WGUI::HandleKeyboardInput(prosper::Window &window, pragma::platform::Key key, int scanCode, pragma::platform::KeyState state, pragma::platform::Modifier mods) { return WIBase::__wiKeyCallback(window, key, scanCode, state, mods); }

bool WGUI::HandleCharInput(prosper::Window &window, unsigned int c) { return WIBase::__wiCharCallback(window, c); }

bool WGUI::HandleScrollInput(prosper::Window &window, Vector2 offset) { return WIBase::__wiScrollCallback(window, offset); }

bool WGUI::HandleFileDragEnter(prosper::Window &window) { return WIBase::__wiFileDragEnterCallback(window); }
bool WGUI::HandleFileDragExit(prosper::Window &window) { return WIBase::__wiFileDragExitCallback(window); }
bool WGUI::HandleFileDrop(prosper::Window &window, const std::vector<std::string> &files) { return WIBase::__wiFileDropCallback(window, files); }

void WGUI::HandleIMEStatusChanged(prosper::Window &window, bool imeEnabled)
{
	auto *elRoot = FindWindowRootElement(window);
	if(!elRoot)
		return;
	elRoot->UpdateIMETarget();
}

static WIBase *check_children(WIBase *gui, int x, int y, int32_t &bestZPos, const std::function<bool(WIBase *)> &condition)
{
	if(gui->IsVisible() == false || gui->PosInBounds(x, y) == false)
		return nullptr;
	auto children = *gui->GetChildren();

	// Children are sorted by zpos, with the highest zpos being at the end, so we have
	// to reverse iterate (since higher zpos has higher priority).
	for(auto it = children.rbegin(); it != children.rend(); ++it) {
		auto &hnd = *it;
		if(hnd.IsValid() == false)
			continue;
		auto *el = hnd.get();
		int childBestZpos = -1;
		auto *pChild = check_children(el, x, y, childBestZpos, condition);
		if(pChild)
			return pChild;
	}

	// None of our children were viable, check if we're viable
	if(condition == nullptr || condition(gui)) {
		auto zPos = gui->GetZPos();
		if(zPos > bestZPos)
			bestZPos = zPos;
		return gui;
	}
	return nullptr;
}
static WIBase *check_children(WIBase *gui, int x, int y, const std::function<bool(WIBase *)> &condition)
{
	int32_t bestZPos = -1;
	return check_children(gui, x, y, bestZPos, condition);
}

WIBase *WGUI::GetGUIElement(WIBase *el, int32_t x, int32_t y, const std::function<bool(WIBase *)> &condition, const prosper::Window *window) { return check_children(el ? el : GetBaseElement(window), x, y, condition); }

WIBase *WGUI::GetCursorGUIElement(WIBase *el, const std::function<bool(WIBase *)> &condition, const prosper::Window *window)
{
	int32_t x, y;
	GetMousePos(x, y, window);
	return GetGUIElement(el, x, y, condition, window);
}
