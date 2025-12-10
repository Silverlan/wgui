// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :core;
import pragma.string.unicode;

#undef DrawState
#undef FindWindow

static std::unique_ptr<pragma::gui::WGUI> s_wgui = nullptr;
prosper::SampleCountFlags pragma::gui::wGUI::MSAA_SAMPLE_COUNT = prosper::SampleCountFlags::e1Bit;
pragma::gui::WGUI &pragma::gui::WGUI::Open(prosper::IPrContext &context, const std::weak_ptr<msys::MaterialManager> &wpMatManager)
{
	s_wgui = nullptr;
	s_wgui = std::make_unique<WGUI>(context, wpMatManager);
	return GetInstance();
}
void pragma::gui::WGUI::ClearWindow(const prosper::Window &window)
{
	auto it = std::find_if(m_windowRootElements.begin(), m_windowRootElements.end(), [&window](const WIHandle &hRoot) { return hRoot.IsValid() && static_cast<const types::WIRoot *>(hRoot.get())->GetWindow() == &window; });
	if(it == m_windowRootElements.end())
		return;
	auto *elRoot = static_cast<types::WIRoot *>(it->get());
	m_windowRootElements.erase(it);
	if(elRoot) {
		auto it = std::find_if(m_rootElements.begin(), m_rootElements.end(), [elRoot](const WIHandle &handle) { return handle.get() == elRoot; });
		if(it != m_rootElements.end())
			m_rootElements.erase(it);
	}
	delete elRoot;
}
void pragma::gui::WGUI::Close()
{
	if(s_wgui == nullptr)
		return;
	std::queue<std::weak_ptr<const prosper::Window>> windows;
	for(auto &hEl : s_wgui->m_windowRootElements) {
		if(hEl.expired())
			continue;
		auto &root = *static_cast<types::WIRoot *>(hEl.get());
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
	types::WIText::ClearTextBuffer();
	types::WIContextMenu::SetKeyBindHandler(nullptr, nullptr);
}
bool pragma::gui::WGUI::IsOpen() { return s_wgui != nullptr; }
pragma::gui::WGUI &pragma::gui::WGUI::GetInstance() { return *s_wgui; }

pragma::gui::WGUI::WGUI(prosper::IPrContext &context, const std::weak_ptr<msys::MaterialManager> &wpMatManager) : prosper::ContextObject(context), m_matManager(wpMatManager)
{
	m_typeFactory = std::make_unique<pragma::gui::TypeFactory>();
	SetMaterialLoadHandler([this](const std::string &path) -> msys::Material * { return m_matManager.lock()->LoadAsset(path).get(); });

	RegisterTypes();
}

pragma::gui::WGUI::~WGUI() {}

void pragma::gui::WGUI::RegisterTypes()
{
	using namespace types;
	RegisterType<types::WIArrow>("WIArrow");
	RegisterType<types::WIBase>("WIBase");
	RegisterType<types::WIButton>("WIButton");
	RegisterType<types::WIContentWrapper>("WIContentWrapper");
	RegisterType<types::WIContextMenu>("WIContextMenu");
	RegisterType<types::WIDropDownMenu>("WIDropDownMenu");
	RegisterType<types::WILine>("WILine");
	RegisterType<types::WIMenuItem>("WIMenuItem");
	RegisterType<types::WI9SliceRectSegment>("WI9SliceRectSegment");
	RegisterType<types::WI9SliceRect>("WI9SliceRect");
	RegisterType<types::WIRect>("WIRect");
	RegisterType<types::WIOutlinedRect>("WIOutlinedRect");
	RegisterType<types::WITexturedRect>("WITexturedRect");
	RegisterType<types::WIRoundedRect>("WIRoundedRect");
	RegisterType<types::WIRoundedTexturedRect>("WIRoundedTexturedRect");
	RegisterType<types::WIScrollBar>("WIScrollBar");
	RegisterType<types::WIShape>("WIShape");
	RegisterType<types::WITexturedShape>("WITexturedShape");
	RegisterType<types::WITextEntryBase>("WITextEntryBase");
	RegisterType<types::WITextEntry>("WITextEntry");
	RegisterType<types::WINumericEntry>("WINumericEntry");
	RegisterType<types::WIText>("WIText");
	RegisterType<types::WITooltip>("WITooltip");
}

void pragma::gui::WGUI::RegisterElement(types::WIBase &el, const std::string &className, types::WIBase *parent)
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

void pragma::gui::WGUI::SetMaterialLoadHandler(const std::function<msys::Material *(const std::string &)> &handler) { m_materialLoadHandler = handler; }
const std::function<msys::Material *(const std::string &)> &pragma::gui::WGUI::GetMaterialLoadHandler() const { return m_materialLoadHandler; }

double pragma::gui::WGUI::GetDeltaTime() const { return m_tDelta; }

pragma::gui::shaders::ShaderColored *pragma::gui::WGUI::GetColoredShader() { return static_cast<pragma::gui::shaders::ShaderColored *>(m_shaderColored.get()); }
pragma::gui::shaders::ShaderColoredRect *pragma::gui::WGUI::GetColoredRectShader() { return static_cast<pragma::gui::shaders::ShaderColoredRect *>(m_shaderColoredCheap.get()); }
pragma::gui::shaders::ShaderColoredLine *pragma::gui::WGUI::GetColoredLineShader() { return static_cast<pragma::gui::shaders::ShaderColoredLine *>(m_shaderColoredLine.get()); }
pragma::gui::shaders::ShaderText *pragma::gui::WGUI::GetTextShader() { return static_cast<pragma::gui::shaders::ShaderText *>(m_shaderText.get()); }
pragma::gui::shaders::ShaderTextRect *pragma::gui::WGUI::GetTextRectShader() { return static_cast<pragma::gui::shaders::ShaderTextRect *>(m_shaderTextCheap.get()); }
pragma::gui::shaders::ShaderTextRectColor *pragma::gui::WGUI::GetTextRectColorShader() { return static_cast<pragma::gui::shaders::ShaderTextRectColor *>(m_shaderTextCheapColor.get()); }
pragma::gui::shaders::ShaderTextured *pragma::gui::WGUI::GetTexturedShader() { return static_cast<pragma::gui::shaders::ShaderTextured *>(m_shaderTextured.get()); }
pragma::gui::shaders::ShaderTexturedRect *pragma::gui::WGUI::GetTexturedRectShader() { return static_cast<pragma::gui::shaders::ShaderTexturedRect *>(m_shaderTexturedCheap.get()); }
pragma::gui::shaders::ShaderTexturedRectExpensive *pragma::gui::WGUI::GetTexturedRectExpensiveShader() { return static_cast<pragma::gui::shaders::ShaderTexturedRectExpensive *>(m_shaderTexturedExpensive.get()); }
pragma::gui::shaders::ShaderStencil *pragma::gui::WGUI::GetStencilShader() { return static_cast<pragma::gui::shaders::ShaderStencil *>(m_shaderStencil.get()); }
pragma::gui::shaders::ShaderTexturedSubRect *pragma::gui::WGUI::GetTexturedSubRectShader() { return static_cast<pragma::gui::shaders::ShaderTexturedSubRect *>(m_shaderTexturedSubRect.get()); }

void pragma::gui::DrawState::SetScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
#ifdef WGUI_ENABLE_SANITY_EXCEPTIONS
	if(x + w > std::numeric_limits<int32_t>::max() || y + h > std::numeric_limits<int32_t>::max())
		throw std::logic_error("Scissor out of bounds!");
#endif
	scissor = {x, y, w, h};
}
void pragma::gui::DrawState::GetScissor(uint32_t &x, uint32_t &y, uint32_t &w, uint32_t &h)
{
	x = scissor.at(0u);
	y = scissor.at(1u);
	w = scissor.at(2u);
	h = scissor.at(3u);
}
bool pragma::gui::UpdatePriority::operator()(const UpdateInfo &h0, const UpdateInfo &h1) const { return h0.depth < h1.depth; }

pragma::gui::WGUI::ResultCode pragma::gui::WGUI::Initialize(std::optional<Vector2i> resolution, std::optional<std::string> fontFileName) { return Initialize(resolution, fontFileName, {}); }
pragma::gui::WGUI::ResultCode pragma::gui::WGUI::Initialize(std::optional<Vector2i> resolution, std::optional<std::string> fontFileName, const std::vector<std::string> &fallbackFontFileNames)
{
	if(!FontManager::Initialize())
		return ResultCode::UnableToInitializeFontManager;

	m_time.Update();
	m_tLastThink = static_cast<double>(m_time());

	auto &context = GetContext();
	prosper::ImageFormatPropertiesQuery query {prosper::ImageCreateFlags::None, prosper::Format::R8G8B8A8_UNorm, prosper::ImageType::e2D, prosper::ImageTiling::Optimal, prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit};
	auto limits = context.GetPhysicalDeviceImageFormatProperties(query);
	if(limits.has_value()) {
		wGUI::MSAA_SAMPLE_COUNT = limits->sampleCount;
		if(wGUI::MSAA_SAMPLE_COUNT > prosper::SampleCountFlags::e8Bit)
			wGUI::MSAA_SAMPLE_COUNT = prosper::SampleCountFlags::e8Bit;
	}
	else
		wGUI::MSAA_SAMPLE_COUNT = prosper::SampleCountFlags::e1Bit;

	auto rpCreateInfo = context.GetWindow().GetStagingRenderPass().GetCreateInfo();
	for(auto &att : rpCreateInfo.attachments)
		att.sampleCount = wGUI::MSAA_SAMPLE_COUNT;
	m_msaaRenderPass = context.CreateRenderPass(rpCreateInfo);

	auto &shaderManager = context.GetShaderManager();
	shaderManager.RegisterShader("wguicolored", [](prosper::IPrContext &context, const std::string &identifier) { return new shaders::ShaderColored(context, identifier); });
	shaderManager.RegisterShader("wguicolored_cheap", [](prosper::IPrContext &context, const std::string &identifier) { return new shaders::ShaderColoredRect(context, identifier); });
	shaderManager.RegisterShader("wguicoloredline", [](prosper::IPrContext &context, const std::string &identifier) { return new shaders::ShaderColoredLine(context, identifier); });
	shaderManager.RegisterShader("wguitext", [](prosper::IPrContext &context, const std::string &identifier) { return new shaders::ShaderText(context, identifier); });
	shaderManager.RegisterShader("wguitext_cheap", [](prosper::IPrContext &context, const std::string &identifier) { return new shaders::ShaderTextRect(context, identifier); });
	shaderManager.RegisterShader("wguitext_cheap_color", [](prosper::IPrContext &context, const std::string &identifier) { return new shaders::ShaderTextRectColor(context, identifier); });
	shaderManager.RegisterShader("wguitextured", [](prosper::IPrContext &context, const std::string &identifier) { return new shaders::ShaderTextured(context, identifier); });
	shaderManager.RegisterShader("wguitextured_cheap", [](prosper::IPrContext &context, const std::string &identifier) { return new shaders::ShaderTexturedRect(context, identifier); });
	shaderManager.RegisterShader("wguitextured_expensive", [](prosper::IPrContext &context, const std::string &identifier) { return new shaders::ShaderTexturedRectExpensive(context, identifier); });
	shaderManager.RegisterShader("wguistencil", [](prosper::IPrContext &context, const std::string &identifier) { return new shaders::ShaderStencil(context, identifier); });
	shaderManager.RegisterShader("wguisubtexturedrect", [](prosper::IPrContext &context, const std::string &identifier) { return new shaders::ShaderTexturedSubRect(context, identifier); });

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

prosper::IRenderPass &pragma::gui::WGUI::GetMsaaRenderPass() const { return *m_msaaRenderPass; }

msys::MaterialManager &pragma::gui::WGUI::GetMaterialManager() { return *m_matManager.lock(); }

void pragma::gui::WGUI::SetCursor(pragma::platform::Cursor::Shape cursor, prosper::Window *window)
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
	SetCursor(pragma::platform::Cursor::GetStandardCursor(cursor), window);
	elRoot->SetMainCursor(cursor);
	elRoot->SetMainCustomCursor(pragma::platform::CursorHandle());
}
void pragma::gui::WGUI::SetCursor(pragma::platform::Cursor &cursor, prosper::Window *window)
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
void pragma::gui::WGUI::SetCursorInputMode(pragma::platform::CursorMode mode, prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return;
	(*window)->SetCursorInputMode(mode);
}
pragma::platform::Cursor::Shape pragma::gui::WGUI::GetCursor(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return pragma::platform::Cursor::Shape::Default;
	auto *elRoot = FindWindowRootElement(*window);
	return elRoot ? elRoot->GetMainCursor() : pragma::platform::Cursor::Shape::Default;
}
pragma::platform::CursorMode pragma::gui::WGUI::GetCursorInputMode(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return pragma::platform::CursorMode::Normal;
	return (*window)->GetCursorInputMode();
}
void pragma::gui::WGUI::ResetCursor(prosper::Window *window) { SetCursor(pragma::platform::Cursor::Shape::Arrow, window); }

void pragma::gui::WGUI::GetMousePos(int &x, int &y, const prosper::Window *window)
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

prosper::Window *pragma::gui::WGUI::FindFocusedWindow()
{
	auto *elRoot = FindFocusedWindowRootElement();
	if(!elRoot)
		return nullptr;
	return const_cast<prosper::Window *>(elRoot->GetWindow());
}
pragma::gui::types::WIRoot *pragma::gui::WGUI::FindFocusedWindowRootElement()
{
	for(auto &hEl : m_windowRootElements) {
		auto window = hEl.IsValid() ? static_cast<types::WIRoot *>(hEl.get())->GetWindow() : nullptr;
		if(!window || (*window)->IsFocused() == false)
			continue;
		return static_cast<types::WIRoot *>(hEl.get());
	}
	return nullptr;
}
prosper::Window *pragma::gui::WGUI::FindWindowUnderCursor()
{
	auto *elRoot = FindWindowRootElementUnderCursor();
	if(!elRoot)
		return nullptr;
	return const_cast<prosper::Window *>(elRoot->GetWindow());
}
pragma::gui::types::WIRoot *pragma::gui::WGUI::FindRootElementUnderCursor() { return FindWindowRootElementUnderCursor(); }
size_t pragma::gui::WGUI::GetLastThinkIndex() const { return m_thinkIndex; }
void pragma::gui::WGUI::BeginDraw()
{
	pragma::gui::WGUI::GetInstance().SetLockedForDrawing(true);
	FontManager::UpdateDirtyFonts();
}
void pragma::gui::WGUI::EndDraw() { pragma::gui::WGUI::GetInstance().SetLockedForDrawing(false); }
void pragma::gui::WGUI::Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd)
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
				auto *el = const_cast<types::WIBase *>(hEl.get());
				if(updateInfo.element.valid() && umath::is_flag_set(updateInfo.element->m_stateFlags, types::WIBase::StateFlags::UpdateScheduledBit)) {
					auto &el = *updateInfo.element;
					if(el.IsVisible() || el.ShouldThinkIfInvisible()) {
						el.m_lastThinkUpdateIndex = m_thinkIndex;
						el.Update();
						umath::set_flag(el.m_stateFlags, types::WIBase::StateFlags::UpdateScheduledBit, false);
					}
					else {
						umath::set_flag(el.m_stateFlags, types::WIBase::StateFlags::ScheduleUpdateOnVisible, true);
						umath::set_flag(el.m_stateFlags, types::WIBase::StateFlags::UpdateScheduledBit, false);
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
	auto *el = GetCursorGUIElement(elBase, [](types::WIBase *el) -> bool { return el->GetCursor() != pragma::platform::Cursor::Shape::Default; }, window);
	while(el && el->GetCursor() == pragma::platform::Cursor::Shape::Default)
		el = el->GetParent();
	SetCursor(el ? el->GetCursor() : pragma::platform::Cursor::Shape::Arrow, window);

	++m_thinkIndex;
}

void pragma::gui::WGUI::ScheduleElementForUpdate(types::WIBase &el, bool force)
{
	if(!force && umath::is_flag_set(el.m_stateFlags, types::WIBase::StateFlags::UpdateScheduledBit))
		return;
	m_bGUIUpdateRequired = true;
	umath::set_flag(el.m_stateFlags, types::WIBase::StateFlags::UpdateScheduledBit, true);
	m_updateQueue.push({el.GetHandle(), el.GetDepth()});
}

void pragma::gui::WGUI::Draw(types::WIBase &el, prosper::IRenderPass &rp, prosper::IFramebuffer &fb, prosper::ICommandBuffer &drawCmd)
{
	auto baseCmd = drawCmd.shared_from_this();
	if(el.IsVisible() == false)
		return;
	el.Draw(el.GetWidth(), el.GetHeight(), baseCmd);
}

void pragma::gui::WGUI::Draw(const prosper::Window &window, prosper::ICommandBuffer &drawCmd)
{
	auto rt = window.GetStagingRenderTarget();
	auto *el = GetBaseElement(&window);
	if(rt && el)
		Draw(*el, rt->GetRenderPass(), rt->GetFramebuffer(), drawCmd);
}

pragma::gui::types::WIBase *pragma::gui::WGUI::Create(std::string classname, types::WIBase *parent)
{
	ustring::to_lower(classname);
	auto &map = GetTypeFactory();
	types::WIBase *(*factory)(void) = map.FindFactory(classname);
	if(factory != nullptr) {
		auto *p = factory();
		p->m_class = classname;
		if(parent != nullptr)
			p->SetParent(parent);
		return p;
	}
	return nullptr;
}

void pragma::gui::WGUI::SetupElement(types::WIBase &el, const std::string className) { el.m_class = className; }

pragma::gui::types::WIRoot *pragma::gui::WGUI::GetBaseElement(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return nullptr;
	return FindWindowRootElement(*window);
}

pragma::gui::types::WIBase *pragma::gui::WGUI::GetFocusedElement(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return GetFocusedElement(static_cast<types::WIRoot *>(nullptr));
	auto *el = FindWindowRootElement(*window);
	if(!el)
		return nullptr;
	return GetFocusedElement(el);
}
uint32_t pragma::gui::WGUI::GetFocusCount(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return GetFocusCount(static_cast<types::WIRoot *>(nullptr));
	auto *el = FindWindowRootElement(*window);
	if(!el)
		return 0;
	return GetFocusCount(el);
}
void pragma::gui::WGUI::IncrementFocusCount(const prosper::Window *window)
{
	window = GetWindow(window);
	if(!window)
		return IncrementFocusCount(static_cast<types::WIRoot *>(nullptr));
	auto *el = FindWindowRootElement(*window);
	if(!el)
		return;
	return IncrementFocusCount(el);
}

pragma::gui::types::WIRoot *pragma::gui::WGUI::AddBaseElement(const prosper::Window *window)
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
	auto *el = new types::WIRoot;
	el->InitializeHandle();
	el->Initialize();
	m_rootElements.push_back(el->GetHandle());
	el->Setup();
	return el;
}

static std::unordered_set<pragma::gui::types::WIBase *> g_exemptFromFocus;
void pragma::gui::WGUI::SetFocusEnabled(const prosper::Window &window, bool enabled)
{
	auto *elRoot = const_cast<WGUI *>(this)->FindWindowRootElement(window);
	if(!elRoot)
		return;
	elRoot->SetFocusEnabled(enabled);
}
bool pragma::gui::WGUI::IsFocusEnabled(const prosper::Window &window) const
{
	auto *elRoot = const_cast<WGUI *>(this)->FindWindowRootElement(window);
	if(!elRoot)
		return false;
	return elRoot->IsFocusEnabled();
}
void pragma::gui::WGUI::ClearFocus(types::WIBase &el)
{
	auto *window = el.GetRootWindow();
	if(!window)
		return;
	auto *elRoot = FindWindowRootElement(*window);
	if(!elRoot)
		return;
	std::function<void(types::WIBase &)> fAddElement = nullptr;
	fAddElement = [&fAddElement](types::WIBase &el) {
		g_exemptFromFocus.insert(&el);
		**el.m_bHasFocus = false;
		el.m_stateFlags &= ~types::WIBase::StateFlags::TrapFocusBit;
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

bool pragma::gui::WGUI::SetFocusedElement(types::WIBase *gui, types::WIRoot *optElRoot)
{
	if(g_exemptFromFocus.find(gui) != g_exemptFromFocus.end())
		return false;
	auto *elRoot = GetRootElement(optElRoot);
	if(!elRoot)
		return false;
	auto *window = elRoot->GetWindow();
	auto *pPrevFocused = elRoot->GetFocusedElement();
	auto &context = GetContext();
	if(gui != nullptr && pPrevFocused != nullptr) {
		types::WIBase *root = elRoot;
		types::WIBase *parent = pPrevFocused;
		while(parent != nullptr && parent != root && parent != elRoot->GetFocusedElement()) {
			if(parent->IsFocusTrapped())
				return false;
			parent = parent->GetParent();
		}
		elRoot->GetFocusedElement()->KillFocus(true);
	}
	if(gui == nullptr) {
		if(window)
			(*window)->SetCursorInputMode(pragma::platform::CursorMode::Disabled);
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

void pragma::gui::WGUI::IncrementFocusCount(types::WIRoot *optElRoot)
{
	optElRoot = GetRootElement(optElRoot);
	if(!optElRoot)
		return;
	optElRoot->SetFocusCount(optElRoot->GetFocusCount() + 1);
}

uint32_t pragma::gui::WGUI::GetFocusCount(const types::WIRoot *optElRoot)
{
	optElRoot = GetRootElement(optElRoot);
	if(!optElRoot)
		return 0;
	return optElRoot->GetFocusCount();
}

pragma::gui::types::WIBase *pragma::gui::WGUI::GetFocusedElement(const types::WIRoot *optElRoot)
{
	optElRoot = GetRootElement(optElRoot);
	if(!optElRoot)
		return nullptr;
	return optElRoot->IsFocusEnabled() ? const_cast<types::WIBase *>(optElRoot->GetFocusedElement()) : nullptr;
}

pragma::gui::types::WIBase *pragma::gui::WGUI::FindByFilter(const std::function<bool(types::WIBase &)> &filter, const prosper::Window *window) const
{
	std::function<types::WIBase *(types::WIBase &)> fIterate = nullptr;
	fIterate = [&filter, &fIterate](types::WIBase &el) -> types::WIBase * {
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
			auto *window = static_cast<const types::WIRoot *>(hEl.get())->GetWindow();
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
pragma::gui::types::WIBase *pragma::gui::WGUI::FindByIndex(uint64_t index) const
{
	return FindByFilter([index](types::WIBase &el) -> bool { return el.GetIndex() == index; });
}

prosper::Window *pragma::gui::WGUI::FindWindow(types::WIBase &elRoot)
{
	auto it = std::find_if(m_windowRootElements.begin(), m_windowRootElements.end(), [&elRoot](const WIHandle &hEl) { return hEl.get() == &elRoot; });
	auto *window = (it != m_windowRootElements.end()) ? static_cast<types::WIRoot *>(it->get())->GetWindow() : nullptr;
	return window && window->IsValid() ? window : nullptr;
}

void pragma::gui::WGUI::RemoveSafely(types::WIBase &gui) { m_removeQueue.push(gui.GetHandle()); }

void pragma::gui::WGUI::Remove(types::WIBase &gui)
{
	if(typeid(gui) == typeid(types::WIRoot)) {
		auto it = std::find_if(m_rootElements.begin(), m_rootElements.end(), [&gui](const WIHandle &h) { return h.get() == &gui; });
		if(it != m_rootElements.end())
			m_rootElements.erase(it);
		delete &gui;
		return;
	}
	auto hEl = gui.GetHandle();
	if(m_removeCallback != nullptr)
		m_removeCallback(gui);
	if(!hEl.IsValid())
		return;
	if(&gui == GetFocusedElement(gui.GetRootWindow())) {
		gui.TrapFocus(false);
		gui.KillFocus();
	}
	delete &gui;
}

void pragma::gui::WGUI::ClearSkin()
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

pragma::gui::WISkin *pragma::gui::WGUI::RegisterSkin(std::string id, std::unique_ptr<WISkin> &&skin)
{
	ustring::to_lower(id);
	auto it = m_skins.find(id);
	if(it != m_skins.end()) {
		auto curSkin = std::move(it->second);
		m_skins.erase(it);

		std::vector<WIHandle> elements;
		std::function<void(types::WIBase &)> findSkinElements = nullptr;
		findSkinElements = [&findSkinElements, &curSkin, &elements](types::WIBase &el) {
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
		for(auto &hEl : pragma::gui::WGUI::GetInstance().GetBaseElements()) {
			if(!hEl.IsValid())
				continue;
			findSkinElements(const_cast<types::WIBase &>(*hEl.get()));
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
	pSkin->SetIdentifier(id);
	m_skins[id] = std::move(skin);
	return pSkin;
}

void pragma::gui::WGUI::SetSkin(std::string skin)
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
pragma::gui::WISkin *pragma::gui::WGUI::GetSkin() { return m_skin; }
pragma::gui::WISkin *pragma::gui::WGUI::GetSkin(std::string name)
{
	auto it = m_skins.find(name);
	if(it == m_skins.end())
		return nullptr;
	return it->second.get();
}
std::string pragma::gui::WGUI::GetSkinName()
{
	if(m_skin == nullptr)
		return "";
	return m_skin->m_identifier;
}
void pragma::gui::WGUI::ClearSkins()
{
	m_skin = nullptr;
	m_skins.clear();
}

void pragma::gui::WGUI::SetCreateCallback(const std::function<void(types::WIBase &)> &onCreate) { m_createCallback = onCreate; }
void pragma::gui::WGUI::SetRemoveCallback(const std::function<void(types::WIBase &)> &onRemove) { m_removeCallback = onRemove; }
void pragma::gui::WGUI::SetFocusCallback(const std::function<void(types::WIBase *, types::WIBase *)> &onFocusChanged) { m_onFocusChangedCallback = onFocusChanged; }

void pragma::gui::WGUI::SetUiMouseButtonCallback(const std::function<void(types::WIBase &, pragma::platform::MouseButton, pragma::platform::KeyState, pragma::platform::Modifier)> &onMouseButton) { m_mouseButtonCallback = onMouseButton; }
void pragma::gui::WGUI::SetUiKeyboardCallback(const std::function<void(types::WIBase &, pragma::platform::Key, int, pragma::platform::KeyState, pragma::platform::Modifier)> &onKeyEvent) { m_keyboardCallback = onKeyEvent; }
void pragma::gui::WGUI::SetUiCharCallback(const std::function<void(types::WIBase &, unsigned int)> &onCharEvent) { m_charCallback = onCharEvent; }
void pragma::gui::WGUI::SetUiScrollCallback(const std::function<void(types::WIBase &, Vector2)> &onScrollCallback) { m_scrollCallback = onScrollCallback; }

pragma::gui::types::WIRoot *pragma::gui::WGUI::GetRootElement(types::WIRoot *elRoot)
{
	if(!elRoot) {
		auto &window = GetContext().GetWindow();
		elRoot = FindWindowRootElement(window);
	}
	return elRoot;
}
prosper::Window *pragma::gui::WGUI::GetWindow(prosper::Window *window)
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
pragma::gui::types::WIRoot *pragma::gui::WGUI::FindWindowRootElement(const prosper::Window &window)
{
	auto it = std::find_if(m_windowRootElements.begin(), m_windowRootElements.end(), [&window](const WIHandle &hEl) {
		if(hEl.expired())
			return false;
		auto *ptrWindow = static_cast<const types::WIRoot *>(hEl.get())->GetWindow();
		return ptrWindow ? ptrWindow == &window : false;
	});
	return (it != m_windowRootElements.end()) ? static_cast<types::WIRoot *>(it->get()) : nullptr;
}

pragma::gui::types::WIRoot *pragma::gui::WGUI::FindWindowRootElementUnderCursor()
{
	types::WIRoot *rootCandidate = nullptr;
	for(auto &hEl : m_windowRootElements) {
		auto *elRoot = static_cast<types::WIRoot *>(hEl.get());
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

bool pragma::gui::WGUI::HandleJoystickInput(prosper::Window &window, const pragma::platform::Joystick &joystick, uint32_t key, pragma::platform::KeyState state) { return types::WIBase::__wiJoystickCallback(window, joystick, key, state); }

bool pragma::gui::WGUI::HandleMouseInput(prosper::Window &window, pragma::platform::MouseButton button, pragma::platform::KeyState state, pragma::platform::Modifier mods) { return types::WIBase::__wiMouseButtonCallback(window, button, state, mods); }

bool pragma::gui::WGUI::HandleKeyboardInput(prosper::Window &window, pragma::platform::Key key, int scanCode, pragma::platform::KeyState state, pragma::platform::Modifier mods) { return types::WIBase::__wiKeyCallback(window, key, scanCode, state, mods); }

bool pragma::gui::WGUI::HandleCharInput(prosper::Window &window, unsigned int c) { return types::WIBase::__wiCharCallback(window, c); }

bool pragma::gui::WGUI::HandleScrollInput(prosper::Window &window, Vector2 offset) { return types::WIBase::__wiScrollCallback(window, offset); }

bool pragma::gui::WGUI::HandleFileDragEnter(prosper::Window &window) { return types::WIBase::__wiFileDragEnterCallback(window); }
bool pragma::gui::WGUI::HandleFileDragExit(prosper::Window &window) { return types::WIBase::__wiFileDragExitCallback(window); }
bool pragma::gui::WGUI::HandleFileDrop(prosper::Window &window, const std::vector<std::string> &files) { return types::WIBase::__wiFileDropCallback(window, files); }

void pragma::gui::WGUI::HandleIMEStatusChanged(prosper::Window &window, bool imeEnabled)
{
	auto *elRoot = FindWindowRootElement(window);
	if(!elRoot)
		return;
	elRoot->UpdateIMETarget();
}

static pragma::gui::types::WIBase *check_children(pragma::gui::types::WIBase *gui, int x, int y, int32_t &bestZPos, const std::function<bool(pragma::gui::types::WIBase *)> &condition)
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
static pragma::gui::types::WIBase *check_children(pragma::gui::types::WIBase *gui, int x, int y, const std::function<bool(pragma::gui::types::WIBase *)> &condition)
{
	int32_t bestZPos = -1;
	return check_children(gui, x, y, bestZPos, condition);
}

pragma::gui::types::WIBase *pragma::gui::WGUI::GetGUIElement(types::WIBase *el, int32_t x, int32_t y, const std::function<bool(types::WIBase *)> &condition, const prosper::Window *window) { return check_children(el ? el : GetBaseElement(window), x, y, condition); }

pragma::gui::types::WIBase *pragma::gui::WGUI::GetCursorGUIElement(types::WIBase *el, const std::function<bool(types::WIBase *)> &condition, const prosper::Window *window)
{
	int32_t x, y;
	GetMousePos(x, y, window);
	return GetGUIElement(el, x, y, condition, window);
}
