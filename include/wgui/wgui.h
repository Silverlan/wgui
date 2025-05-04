/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WGUI_H__
#define __WGUI_H__
#include <string>
#include "wguidefinitions.h"
#include "wiskin.h"
#include "wguifactories.h"
#include "wihandle.h"
#include <material_manager2.hpp>
#include <unordered_map>
#include <algorithm>
#include <queue>
#include <prosper_context_object.hpp>
#include <sharedutils/chronotime.h>
#include "types.hpp"

import pragma.platform;
#ifdef __linux__
import pragma.string.unicode;
#else
namespace pragma::string {
	class Utf8String;
};
#endif

#undef GetClassName
#undef FindWindow
#undef DrawState

namespace GLFW {
	class Joystick;
};
namespace prosper {
	class IUniformResizableBuffer;
	class IBuffer;
	class IDescriptorSetGroup;
	class Shader;
	class IRenderPass;
	class IFramebuffer;
	class ISecondaryCommandBuffer;
	class ICommandBuffer;
	class Window;
	enum class SampleCountFlags : uint32_t;
};

namespace wgui {
	class Shader;
	class ShaderColored;
	class ShaderColoredRect;
	class ShaderColoredLine;
	class ShaderText;
	class ShaderTextRect;
	class ShaderTextRectColor;
	class ShaderTextured;
	class ShaderTexturedRect;
	class ShaderTexturedRectExpensive;
	class ShaderStencil;
	class ShaderTexturedSubRect;
};

namespace wgui {
	struct DLLWGUI DrawState {
		std::array<uint32_t, 4> scissor = {0u, 0, 0u, 0u};
		float renderAlpha = 1.f;

		void GetScissor(uint32_t &x, uint32_t &y, uint32_t &w, uint32_t &h);
		void SetScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
	};

	namespace detail {
		struct DLLWGUI UpdateInfo {
			WIHandle element;
			uint32_t depth;
		};
		struct DLLWGUI UpdatePriority {
			bool operator()(const UpdateInfo &h0, const UpdateInfo &h1) const;
		};
	};
};

class WIRoot;
class DLLWGUI WGUI : public prosper::ContextObject {
  public:
	friend WIBase;
	static prosper::SampleCountFlags MSAA_SAMPLE_COUNT;
	enum class ElementBuffer : uint32_t {
		SizeColor = sizeof(Vector4),
		SizeMVP = sizeof(Mat4),

		OffsetColor = 0,
		OffsetMVP = OffsetColor + SizeColor,

		Size = OffsetMVP + SizeMVP
	};
	enum class ElementVertex : uint32_t {
		SizePos = sizeof(Vector2),
		SizeUV = sizeof(Vector2),

		Size = SizePos + SizeUV
	};
	enum class ResultCode : uint8_t {
		Ok = 0u,
		UnableToInitializeFontManager,
		ErrorInitializingShaders,
		FontNotFound,
	};
	WGUI(prosper::IPrContext &context, const std::weak_ptr<msys::MaterialManager> &wpMatManager);
	WGUI(const WGUI &) = delete;
	~WGUI();
	WGUI &operator=(const WGUI &) = delete;

	static WGUI &Open(prosper::IPrContext &context, const std::weak_ptr<msys::MaterialManager> &wpMatManager);
	static void Close();
	static WGUI &GetInstance();
	static bool IsOpen();

	ResultCode Initialize(std::optional<Vector2i> resolution, std::optional<std::string> fontFileName, const std::vector<std::string> &fallbackFontFileNames = {});
	ResultCode Initialize(std::optional<Vector2i> resolution = {}, std::optional<std::string> fontFileName = {});
	template<class TElement>
	TElement *Create(WIBase *parent = nullptr);
	template<class TElement>
	void Setup(WIBase &el, WIBase *parent = nullptr);
	void RegisterElement(WIBase &el, const std::string &className, WIBase *parent = nullptr);
	WIBase *Create(std::string classname, WIBase *parent = nullptr);
	void RemoveSafely(WIBase &gui);
	void Remove(WIBase &gui);
	void ClearFocus(WIBase &el);
	void SetFocusEnabled(const prosper::Window &window, bool enabled);
	bool IsFocusEnabled(const prosper::Window &window) const;
	const std::vector<WIHandle> &GetBaseElements() const { return m_rootElements; }
	WIRoot *GetBaseElement(const prosper::Window *optWindow = nullptr);
	WIRoot *AddBaseElement(const prosper::Window *optWindow = nullptr);
	WIBase *GetFocusedElement(const prosper::Window *window);
	uint32_t GetFocusCount(const prosper::Window *window);
	void IncrementFocusCount(const prosper::Window *window);
	WIBase *GetFocusedElement(const WIRoot *optElRoot = nullptr);
	uint32_t GetFocusCount(const WIRoot *optElRoot = nullptr);
	void IncrementFocusCount(WIRoot *optElRoot = nullptr);
	WIBase *FindByFilter(const std::function<bool(WIBase &)> &filter, const prosper::Window *optWindow = nullptr) const;
	WIBase *FindByIndex(uint64_t index) const;
	prosper::Window *FindWindow(WIBase &elRoot);
	const prosper::Window *FindWindow(WIBase &elRoot) const { return const_cast<WGUI *>(this)->FindWindow(elRoot); }
	void Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd);
	void Draw(const prosper::Window &window, prosper::ICommandBuffer &drawCmd);
	void Draw(WIBase &el, prosper::IRenderPass &rp, prosper::IFramebuffer &fb, prosper::ICommandBuffer &drawCmd);
	bool HandleJoystickInput(prosper::Window &window, const pragma::platform::Joystick &joystick, uint32_t key, pragma::platform::KeyState state);
	bool HandleMouseInput(prosper::Window &window, pragma::platform::MouseButton button, pragma::platform::KeyState state, pragma::platform::Modifier mods);
	bool HandleKeyboardInput(prosper::Window &window, pragma::platform::Key key, int scanCode, pragma::platform::KeyState state, pragma::platform::Modifier mods);
	bool HandleCharInput(prosper::Window &window, unsigned int c);
	bool HandleScrollInput(prosper::Window &window, Vector2 offset);
	bool HandleFileDragEnter(prosper::Window &window);
	bool HandleFileDragExit(prosper::Window &window);
	bool HandleFileDrop(prosper::Window &window, const std::vector<std::string> &files);
	void HandleIMEStatusChanged(prosper::Window &window, bool imeEnabled);
	void SetCreateCallback(const std::function<void(WIBase &)> &onCreate);
	void SetRemoveCallback(const std::function<void(WIBase &)> &onRemove);
	void SetFocusCallback(const std::function<void(WIBase *, WIBase *)> &onFocusChanged);
	void GetMousePos(int &x, int &y, const prosper::Window *optWindow = nullptr);
	prosper::Window *FindFocusedWindow();
	WIRoot *FindFocusedWindowRootElement();
	prosper::Window *FindWindowUnderCursor();
	WIRoot *FindRootElementUnderCursor();
	template<class TSkin>
	TSkin *RegisterSkin(std::string id, bool bReload = false);
	WISkin *RegisterSkin(std::string id, std::unique_ptr<WISkin> &&skin);
	void SetSkin(std::string skin);
	std::string GetSkinName();
	WISkin *GetSkin();
	WISkin *GetSkin(std::string name);
	void ClearSkins();
	void SetCursor(pragma::platform::Cursor::Shape cursor, prosper::Window *optWindow = nullptr);
	void SetCursor(pragma::platform::Cursor &cursor, prosper::Window *optWindow = nullptr);
	void ResetCursor(prosper::Window *optWindow = nullptr);
	void SetCursorInputMode(pragma::platform::CursorMode mode, prosper::Window *optWindow = nullptr);
	pragma::platform::Cursor::Shape GetCursor(const prosper::Window *optWindow = nullptr);
	pragma::platform::CursorMode GetCursorInputMode(const prosper::Window *optWindow = nullptr);
	msys::MaterialManager &GetMaterialManager();
	void SetMaterialLoadHandler(const std::function<Material *(const std::string &)> &handler);
	const std::function<Material *(const std::string &)> &GetMaterialLoadHandler() const;
	WIBase *GetGUIElement(WIBase *el, int32_t x, int32_t y, const std::function<bool(WIBase *)> &condition, const prosper::Window *optWindow = nullptr);
	WIBase *GetCursorGUIElement(WIBase *el, const std::function<bool(WIBase *)> &condition, const prosper::Window *optWindow = nullptr);
	double GetDeltaTime() const;
	prosper::IRenderPass &GetMsaaRenderPass() const;
	//prosper::util::record_set_scissor(*drawCmd,szScissor[0],szScissor[1],posScissor[0],posScissor[1]); // Use parent scissor values
	bool IsLockedForDrawing() const { return m_lockedForDrawing; }
	void SetLockedForDrawing(bool locked) { m_lockedForDrawing = locked; }
	uint64_t GetNextGuiElementIndex() const { return m_nextGuiElementIndex; }
	void BeginDraw();
	void EndDraw();

	size_t GetLastThinkIndex() const;

	wgui::ShaderColored *GetColoredShader();
	wgui::ShaderColoredRect *GetColoredRectShader();
	wgui::ShaderColoredLine *GetColoredLineShader();
	wgui::ShaderText *GetTextShader();
	wgui::ShaderTextRect *GetTextRectShader();
	wgui::ShaderTextRectColor *GetTextRectColorShader();
	wgui::ShaderTextured *GetTexturedShader();
	wgui::ShaderTexturedSubRect *GetTexturedSubRectShader();
	wgui::ShaderTexturedRect *GetTexturedRectShader();
	wgui::ShaderTexturedRectExpensive *GetTexturedRectExpensiveShader();
	wgui::ShaderStencil *GetStencilShader();
  private:
	void ScheduleElementForUpdate(WIBase &el, bool force = false);
	friend WIBase;
	friend wgui::Shader;
	friend wgui::ShaderColoredRect;

	std::atomic<bool> m_lockedForDrawing = false;
	WISkin *m_skin = nullptr;
	bool m_bGUIUpdateRequired = false;
	std::unordered_map<std::string, std::unique_ptr<WISkin>> m_skins = {};
	std::weak_ptr<msys::MaterialManager> m_matManager = {};
	std::function<Material *(const std::string &)> m_materialLoadHandler = nullptr;
	std::shared_ptr<prosper::IUniformResizableBuffer> m_elementBuffer = nullptr;
	std::vector<WIHandle> m_rootElements {};
	std::shared_ptr<prosper::IRenderPass> m_msaaRenderPass = nullptr;
	WIRoot *FindWindowRootElement(const prosper::Window &window);
	WIRoot *FindWindowRootElementUnderCursor();
	const prosper::Window *GetWindow(const prosper::Window *window) const { return const_cast<WGUI *>(this)->GetWindow(const_cast<prosper::Window *>(window)); }
	prosper::Window *GetWindow(prosper::Window *window);
	const WIRoot *GetRootElement(const WIRoot *elRoot) const { return const_cast<WGUI *>(this)->GetRootElement(const_cast<WIRoot *>(elRoot)); }
	WIRoot *GetRootElement(WIRoot *elRoot);
	void ClearWindow(const prosper::Window &window);
	std::vector<WIHandle> m_windowRootElements {};
	uint64_t m_nextGuiElementIndex = 0u;

	// In general very few elements actually need to apply any continuous logic,
	// so we keep a separate reference to those elements for better efficiency.
	std::vector<WIHandle> m_thinkingElements;

	std::priority_queue<wgui::detail::UpdateInfo, std::vector<wgui::detail::UpdateInfo>, wgui::detail::UpdatePriority> m_updateQueue;
	std::optional<uint32_t> m_currentUpdateDepth = {};
	std::queue<WIHandle> m_removeQueue;
	std::vector<std::unique_ptr<pragma::platform::Cursor>> m_cursors;
	std::function<void(WIBase &)> m_createCallback = nullptr;
	std::function<void(WIBase &)> m_removeCallback = nullptr;
	std::function<void(WIBase *, WIBase *)> m_onFocusChangedCallback = nullptr;
	ChronoTime m_time = {};
	double m_tLastThink = 0;
	double m_tDelta = 0.f;
	size_t m_thinkIndex = 0;

	util::WeakHandle<prosper::Shader> m_shaderColored = {};
	util::WeakHandle<prosper::Shader> m_shaderColoredCheap = {};
	util::WeakHandle<prosper::Shader> m_shaderColoredLine = {};
	util::WeakHandle<prosper::Shader> m_shaderText = {};
	util::WeakHandle<prosper::Shader> m_shaderTextCheap = {};
	util::WeakHandle<prosper::Shader> m_shaderTextCheapColor = {};
	util::WeakHandle<prosper::Shader> m_shaderTextured = {};
	util::WeakHandle<prosper::Shader> m_shaderTexturedSubRect = {};
	util::WeakHandle<prosper::Shader> m_shaderTexturedCheap = {};
	util::WeakHandle<prosper::Shader> m_shaderTexturedExpensive = {};
	util::WeakHandle<prosper::Shader> m_shaderStencil = {};

	bool SetFocusedElement(WIBase *gui, WIRoot *optElRoot = nullptr);
	void ClearSkin();
};
REGISTER_BASIC_ARITHMETIC_OPERATORS(WGUI::ElementBuffer);

#include <wgui/wibase.h>
#include "wihandle.h"

template<class TSkin>
TSkin *WGUI::RegisterSkin(std::string id, bool bReload)
{
	ustring::to_lower(id);
	if(m_skins.find(id) != m_skins.end() && !bReload)
		return nullptr;
	auto skin = std::unique_ptr<WISkin> {new TSkin {id}};
	return static_cast<TSkin *>(RegisterSkin(id, std::move(skin)));
}

template<class TElement>
void WGUI::Setup(WIBase &el, WIBase *parent)
{
	std::string classname;
	auto *map = GetWGUIClassMap();
	if(map->GetClassName(typeid(TElement), &classname))
		el.m_class = classname;
	RegisterElement(el, classname, parent);
}

template<class TElement>
TElement *WGUI::Create(WIBase *parent)
{
	auto *el = new TElement;
	Setup<TElement>(*el, parent);
	return el;
}

#endif
