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
#include <materialmanager.h>
#include <unordered_map>
#include <algorithm>
#include <queue>
#include <iglfw/glfw_window.h>
#include <prosper_context_object.hpp>
#include <sharedutils/chronotime.h>

#undef GetClassName
#undef FindWindow
class WIBase;
class WISkin;
class WIHandle;
namespace GLFW {class Joystick;};
namespace prosper
{
	class IUniformResizableBuffer;
	class IBuffer;
	class IDescriptorSetGroup;
	class Shader;
	class IRenderPass;
	class IFramebuffer;
	class ISecondaryCommandBuffer;
	class ICommandBuffer;
	class Window;
};

namespace wgui
{
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
};

class DLLWGUI WGUI
	: public prosper::ContextObject
{
public:
	friend WIBase;
	enum class ElementBuffer : uint32_t
	{
		SizeColor = sizeof(Vector4),
		SizeMVP = sizeof(Mat4),

		OffsetColor = 0,
		OffsetMVP = OffsetColor +SizeColor,

		Size = OffsetMVP +SizeMVP
	};
	enum class ElementVertex : uint32_t
	{
		SizePos = sizeof(Vector2),
		SizeUV = sizeof(Vector2),

		Size = SizePos +SizeUV
	};
	enum class ResultCode : uint8_t
	{
		Ok = 0u,
		UnableToInitializeFontManager,
		ErrorInitializingShaders,
		FontNotFound,
	};
	WGUI(prosper::IPrContext &context,const std::weak_ptr<MaterialManager> &wpMatManager);
	WGUI(const WGUI&)=delete;
	~WGUI();
	WGUI &operator=(const WGUI&)=delete;

	static WGUI &Open(prosper::IPrContext &context,const std::weak_ptr<MaterialManager> &wpMatManager);
	static void Close();
	static WGUI &GetInstance();
	static bool IsOpen();

	ResultCode Initialize(std::optional<Vector2i> resolution={});
	template<class TElement>
		TElement *Create(WIBase *parent=nullptr);
	template<class TElement>
		void Setup(WIBase &el,WIBase *parent=nullptr);
	WIBase *Create(std::string classname,WIBase *parent=nullptr);
	void RemoveSafely(WIBase &gui);
	void Remove(WIBase &gui);
	const std::vector<WIHandle> &GetBaseElements() const {return m_rootElements;}
	WIBase *GetBaseElement(const prosper::Window *optWindow=nullptr);
	WIBase *AddBaseElement(const prosper::Window *optWindow=nullptr);
	WIBase *GetFocusedElement(const prosper::Window *optWindow=nullptr);
	WIBase *FindByFilter(const std::function<bool(WIBase&)> &filter,const prosper::Window *optWindow=nullptr) const;
	WIBase *FindByIndex(uint64_t index) const;
	prosper::Window *FindWindow(WIBase &elRoot);
	const prosper::Window *FindWindow(WIBase &elRoot) const {return const_cast<WGUI*>(this)->FindWindow(elRoot);}
	void Think();
	void Draw(const prosper::Window &window,prosper::ICommandBuffer &drawCmd);
	void Draw(WIBase &el,prosper::IRenderPass &rp,prosper::IFramebuffer &fb,prosper::ICommandBuffer &drawCmd);
	bool HandleJoystickInput(prosper::Window &window,const GLFW::Joystick &joystick,uint32_t key,GLFW::KeyState state);
	bool HandleMouseInput(prosper::Window &window,GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods);
	bool HandleKeyboardInput(prosper::Window &window,GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods);
	bool HandleCharInput(prosper::Window &window,unsigned int c);
	bool HandleScrollInput(prosper::Window &window,Vector2 offset);
	void SetHandleFactory(const std::function<std::shared_ptr<WIHandle>(WIBase&)> &handleFactory);
	void SetCreateCallback(const std::function<void(WIBase&)> &onCreate);
	void SetRemoveCallback(const std::function<void(WIBase&)> &onRemove);
	void SetFocusCallback(const std::function<void(WIBase*,WIBase*)> &onFocusChanged);
	void GetMousePos(int &x,int &y,const prosper::Window *optWindow=nullptr);
	prosper::Window *FindFocusedWindow();
	WIBase *FindFocusedWindowRootElement();
	prosper::Window *FindWindowUnderCursor();
	WIBase *FindRootElementUnderCursor();
	template<class TSkin>
		TSkin *RegisterSkin(std::string id,bool bReload=false);
	void SetSkin(std::string skin);
	std::string GetSkinName();
	WISkin *GetSkin();
	WISkin *GetSkin(std::string name);
	void SetCursor(GLFW::Cursor::Shape cursor);
	void SetCursor(GLFW::Cursor &cursor);
	void ResetCursor();
	void SetCursorInputMode(GLFW::CursorMode mode);
	GLFW::Cursor::Shape GetCursor();
	GLFW::CursorMode GetCursorInputMode();
	MaterialManager &GetMaterialManager();
	void SetMaterialLoadHandler(const std::function<Material*(const std::string&)> &handler);
	const std::function<Material*(const std::string&)> &GetMaterialLoadHandler() const;
	WIBase *GetGUIElement(WIBase *el,int32_t x,int32_t y,const std::function<bool(WIBase*)> &condition,const prosper::Window *optWindow=nullptr);
	WIBase *GetCursorGUIElement(WIBase *el,const std::function<bool(WIBase*)> &condition,const prosper::Window *optWindow=nullptr);
	double GetDeltaTime() const;
	//prosper::util::record_set_scissor(*drawCmd,szScissor[0],szScissor[1],posScissor[0],posScissor[1]); // Use parent scissor values

	wgui::ShaderColored *GetColoredShader();
	wgui::ShaderColoredRect *GetColoredRectShader();
	wgui::ShaderColoredLine *GetColoredLineShader();
	wgui::ShaderText *GetTextShader();
	wgui::ShaderTextRect *GetTextRectShader();
	wgui::ShaderTextRectColor *GetTextRectColorShader();
	wgui::ShaderTextured *GetTexturedShader();
	wgui::ShaderTexturedRect *GetTexturedRectShader();
	wgui::ShaderTexturedRectExpensive *GetTexturedRectExpensiveShader();

	void GetScissor(uint32_t &x,uint32_t &y,uint32_t &w,uint32_t &h);
	void SetScissor(uint32_t x,uint32_t y,uint32_t w,uint32_t h);
private:
	void ScheduleElementForUpdate(WIBase &el);
	friend WIBase;
	friend wgui::Shader;
	friend wgui::ShaderColoredRect;

	WISkin *m_skin = nullptr;
	bool m_bGUIUpdateRequired = false;
	std::unordered_map<std::string,WISkin*> m_skins = {};
	std::weak_ptr<MaterialManager> m_matManager = {};
	std::function<Material*(const std::string&)> m_materialLoadHandler = nullptr;
	std::shared_ptr<prosper::IUniformResizableBuffer> m_elementBuffer = nullptr;
	std::vector<WIHandle> m_rootElements {};
	struct WindowRootPair
	{
		std::weak_ptr<const prosper::Window> window {};
		WIHandle rootElement {};
		WIHandle elFocused = {};
	};
	WindowRootPair *FindWindowRootPair(const prosper::Window &window);
	WindowRootPair *FindFocusedWindowRootPair();
	WindowRootPair *FindWindowRootPairUnderCursor();
	const prosper::Window *GetWindow(const prosper::Window *window) const {return const_cast<WGUI*>(this)->GetWindow(const_cast<prosper::Window*>(window));}
	prosper::Window *GetWindow(prosper::Window *window);
	std::vector<WindowRootPair> m_windowRootElements {};
	uint64_t m_nextGuiElementIndex = 0u;

	// In general very few elements actually need to apply any continuous logic,
	// so we keep a separate reference to those elements for better efficiency.
	std::vector<WIHandle> m_thinkingElements;

	std::vector<WIHandle> m_updateQueue;
	std::queue<WIHandle> m_removeQueue;
	std::function<std::shared_ptr<WIHandle>(WIBase&)> m_handleFactory = nullptr;
	std::vector<std::unique_ptr<GLFW::Cursor>> m_cursors;
	GLFW::Cursor::Shape m_cursor = GLFW::Cursor::Shape::Arrow;
	GLFW::CursorHandle m_customCursor = {};
	std::function<void(WIBase&)> m_createCallback = nullptr;
	std::function<void(WIBase&)> m_removeCallback = nullptr;
	std::function<void(WIBase*,WIBase*)> m_onFocusChangedCallback = nullptr;
	ChronoTime m_time = {};
	double m_tLastThink = 0;
	double m_tDelta = 0.f;

	util::WeakHandle<prosper::Shader> m_shaderColored = {};
	util::WeakHandle<prosper::Shader> m_shaderColoredCheap = {};
	util::WeakHandle<prosper::Shader> m_shaderColoredLine = {};
	util::WeakHandle<prosper::Shader> m_shaderText = {};
	util::WeakHandle<prosper::Shader> m_shaderTextCheap = {};
	util::WeakHandle<prosper::Shader> m_shaderTextCheapColor = {};
	util::WeakHandle<prosper::Shader> m_shaderTextured = {};
	util::WeakHandle<prosper::Shader> m_shaderTexturedCheap = {};
	util::WeakHandle<prosper::Shader> m_shaderTexturedExpensive = {};

	bool SetFocusedElement(WIBase *gui,prosper::Window *optWindow=nullptr);
	void ClearSkin();
};
REGISTER_BASIC_ARITHMETIC_OPERATORS(WGUI::ElementBuffer);

#include <wgui/wibase.h>
#include "wihandle.h"

template<class TSkin>
	TSkin *WGUI::RegisterSkin(std::string id,bool bReload)
{
	std::transform(id.begin(),id.end(),id.begin(),::tolower);
	std::unordered_map<std::string,WISkin*>::iterator it = m_skins.find(id);
	if(it != m_skins.end())
	{
		if(bReload == false)
			return NULL;
		if(m_skin == it->second)
			ClearSkin();
		delete it->second;
		m_skins.erase(it);
	}
	TSkin *skin = new TSkin(id);
	m_skins.insert(std::unordered_map<std::string,WISkin*>::value_type(id,skin));
	return skin;
}

template<class TElement>
	void WGUI::Setup(WIBase &el,WIBase *parent)
{
	el.SetIndex(m_nextGuiElementIndex++);
	if(m_handleFactory != nullptr)
	{
		auto handle = m_handleFactory(el);
		if(handle != nullptr && handle->IsValid())
			el.InitializeHandle(std::move(handle));
		else
			el.InitializeHandle();
	}
	else
		el.InitializeHandle();
	if(parent != nullptr)
		el.SetParent(parent);
	else
	{
		auto *elBase = GetBaseElement();
		if(elBase)
			el.SetParent(elBase);
	}
	auto *map = GetWGUIClassMap();
	std::string classname;
	if(map->GetClassName(typeid(TElement),&classname))
		el.m_class = classname;
	el.AddStyleClass(el.GetClass());
	el.Initialize();
	if(m_createCallback != nullptr)
		m_createCallback(el);
}

template<class TElement>
	TElement *WGUI::Create(WIBase *parent)
{
	auto *el = new TElement;
	Setup<TElement>(*el,parent);
	return el;
}

#endif