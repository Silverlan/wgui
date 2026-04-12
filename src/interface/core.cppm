// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "util_enum_flags.hpp"

export module pragma.gui:core;

import :display_text;
import :handle;
import :skin;
import :tooltip_manager;
import :type_factory;

export import pragma.materialsystem;
export import pragma.platform;
export import pragma.prosper;
export import pragma.string.unicode;

#undef GetClassName
#undef FindWindow
#undef DrawState

namespace pragma::gui {
	struct DLLWGUI UpdateInfo {
		WIHandle element;
		uint32_t depth;
	};
	struct DLLWGUI UpdatePriority {
		bool operator()(const UpdateInfo &h0, const UpdateInfo &h1) const;
	};
};

export namespace pragma::gui {
	namespace types {
		class WIRoot;
	}
	namespace shaders {
		class Shader;
		class ShaderColored;
		class ShaderColoredRect;
		class ShaderColoredLine;
		class ShaderText;
		class ShaderTextRect;
		class ShaderTextRectColor;
		class ShaderTextured;
		class ShaderTexturedSubRect;
		class ShaderTexturedNineSlice;
		class ShaderTexturedRect;
		class ShaderTexturedRectExpensive;
		class ShaderStencil;

		enum class ShaderType : uint8_t {
			Colored = 0,
			ColoredRect,
			ColoredLine,
			Text,
			TextCheap,
			TextCheapColor,
			Textured,
			TexturedSubRect,
			TexturedNineSlice,
			TexturedCheap,
			TexturedExpensive,
			Stencil,

			Count,
		};
		template<ShaderType S>
		struct ShaderTraits;
		template<>
		struct ShaderTraits<ShaderType::Colored> {
			using type = ShaderColored;
		};
		template<>
		struct ShaderTraits<ShaderType::ColoredRect> {
			using type = ShaderColoredRect;
		};
		template<>
		struct ShaderTraits<ShaderType::ColoredLine> {
			using type = ShaderColoredLine;
		};
		template<>
		struct ShaderTraits<ShaderType::Text> {
			using type = ShaderText;
		};
		template<>
		struct ShaderTraits<ShaderType::TextCheap> {
			using type = ShaderTextRect;
		};
		template<>
		struct ShaderTraits<ShaderType::TextCheapColor> {
			using type = ShaderTextRectColor;
		};
		template<>
		struct ShaderTraits<ShaderType::Textured> {
			using type = ShaderTextured;
		};
		template<>
		struct ShaderTraits<ShaderType::TexturedSubRect> {
			using type = ShaderTexturedSubRect;
		};
		template<>
		struct ShaderTraits<ShaderType::TexturedNineSlice> {
			using type = ShaderTexturedNineSlice;
		};
		template<>
		struct ShaderTraits<ShaderType::TexturedCheap> {
			using type = ShaderTexturedRect;
		};
		template<>
		struct ShaderTraits<ShaderType::TexturedExpensive> {
			using type = ShaderTexturedRectExpensive;
		};
		template<>
		struct ShaderTraits<ShaderType::Stencil> {
			using type = ShaderStencil;
		};
	}
	namespace wGUI {
#ifdef WINDOWS_CLANG_COMPILER_FIX
		DLLWGUI prosper::SampleCountFlags &GET_MSAA_SAMPLE_COUNT();
#else
		CLASS_ENUM_COMPAT prosper::SampleCountFlags MSAA_SAMPLE_COUNT;
#endif
	}

	class DLLWGUI MemoryTracker {
	  public:
		static size_t totalAllocated;
		static size_t totalFreed;
		static size_t currentUsage() { return totalAllocated - totalFreed; }
		static size_t peakUsage;
	};

	using Element = types::WIBase;
	class DLLWGUI WGUI : public prosper::ContextObject {
	  public:
		friend Element;
		using LocaleResolver = std::function<string::Utf8String(const std::string &, const std::vector<string::Utf8String> &)>;
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
		WGUI(prosper::IPrContext &context, const std::weak_ptr<material::MaterialManager> &wpMatManager, util::HeapManager *heapManager);
		WGUI(const WGUI &) = delete;
		~WGUI();
		WGUI &operator=(const WGUI &) = delete;

		static WGUI &Open(prosper::IPrContext &context, const std::weak_ptr<material::MaterialManager> &wpMatManager, util::HeapManager *heapManager = nullptr);
		static void Close();
		static WGUI &GetInstance();
		static bool IsOpen();

		ResultCode Initialize(std::optional<Vector2i> resolution, std::optional<std::string> fontFileName, const std::vector<std::string> &fallbackFontFileNames = {});
		ResultCode Initialize(std::optional<Vector2i> resolution = {}, std::optional<std::string> fontFileName = {});
		template<class TElement>
		TElement *Create(Element *parent = nullptr)
		{
			util::HeapScope heapScope {m_guiHeap};
			auto *el = new TElement;
			Setup<TElement>(*el, parent);
			return el;
		}
		template<class TElement>
		void Setup(Element &el, Element *parent = nullptr)
		{
			util::HeapScope heapScope {m_guiHeap};
			auto &map = GetTypeFactory();
			auto className = map.FindClassName(typeid(TElement));
			if(className)
				SetupElement(el, *className);
			else
				className = "";
			RegisterElement(el, *className, parent);
		}
		void RegisterElement(Element &el, const std::string &className, Element *parent = nullptr);
		Element *Create(std::string classname, Element *parent = nullptr);
		void RemoveSafely(Element &gui);
		void Remove(Element &gui);
		void ClearFocus(Element &el);
		void SetFocusEnabled(const prosper::Window &window, bool enabled);
		bool IsFocusEnabled(const prosper::Window &window) const;
		const std::vector<WIHandle> &GetBaseElements() const { return m_rootElements; }
		types::WIRoot *GetBaseElement(const prosper::Window *optWindow = nullptr);
		types::WIRoot *AddBaseElement(const prosper::Window *optWindow = nullptr);
		Element *GetFocusedElement(const prosper::Window *window);
		uint32_t GetFocusCount(const prosper::Window *window);
		void IncrementFocusCount(const prosper::Window *window);
		Element *GetFocusedElement(const types::WIRoot *optElRoot = nullptr);
		uint32_t GetFocusCount(const types::WIRoot *optElRoot = nullptr);
		void IncrementFocusCount(types::WIRoot *optElRoot = nullptr);
		Element *FindByFilter(const std::function<bool(Element &)> &filter, const prosper::Window *optWindow = nullptr) const;
		Element *FindByIndex(uint64_t index) const;
		prosper::Window *FindWindow(Element &elRoot);
		const prosper::Window *FindWindow(Element &elRoot) const { return const_cast<WGUI *>(this)->FindWindow(elRoot); }
		void Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd);
		void Draw(const prosper::Window &window, prosper::ICommandBuffer &drawCmd);
		void Draw(Element &el, prosper::IRenderPass &rp, prosper::IFramebuffer &fb, prosper::ICommandBuffer &drawCmd);
		bool HandleJoystickInput(prosper::Window &window, const platform::Joystick &joystick, uint32_t key, platform::KeyState state);
		bool HandleMouseInput(prosper::Window &window, platform::MouseButton button, platform::KeyState state, platform::Modifier mods);
		bool HandleKeyboardInput(prosper::Window &window, platform::Key key, int scanCode, platform::KeyState state, platform::Modifier mods);
		bool HandleCharInput(prosper::Window &window, unsigned int c);
		bool HandleScrollInput(prosper::Window &window, Vector2 offset);
		bool HandleFileDragEnter(prosper::Window &window);
		bool HandleFileDragExit(prosper::Window &window);
		bool HandleFileDrop(prosper::Window &window, const std::vector<std::string> &files);
		void HandleIMEStatusChanged(prosper::Window &window, bool imeEnabled);
		void SetCreateCallback(const std::function<void(Element &)> &onCreate);
		void SetRemoveCallback(const std::function<void(Element &)> &onRemove);
		void SetFocusCallback(const std::function<void(Element *, Element *)> &onFocusChanged);

		string::Utf8String GetLocalizedText(const std::string &key, const std::vector<string::Utf8String> &args = {}) const;
		void SetLocaleResolver(const LocaleResolver &resolver);
		void RefreshLocale();

		types::WIRoot *FindWindowRootElement(const prosper::Window &window);
		types::WIRoot *FindWindowRootElementUnderCursor();

		void SetUiMouseButtonCallback(const std::function<void(Element &, platform::MouseButton, platform::KeyState, platform::Modifier)> &onMouseButton);
		void SetUiKeyboardCallback(const std::function<void(Element &, platform::Key, int, platform::KeyState, platform::Modifier)> &onKeyEvent);
		void SetUiCharCallback(const std::function<void(Element &, unsigned int)> &onCharEvent);
		void SetUiScrollCallback(const std::function<void(Element &, Vector2)> &onScrollCallback);

		void GetMousePos(int &x, int &y, const prosper::Window *optWindow = nullptr);
		prosper::Window *FindFocusedWindow();
		types::WIRoot *FindFocusedWindowRootElement();
		prosper::Window *FindWindowUnderCursor();
		types::WIRoot *FindRootElementUnderCursor();
		template<class TSkin>
		TSkin *RegisterSkin(std::string id, bool bReload = false)
		{
			string::to_lower(id);
			if(m_skins.find(id) != m_skins.end() && !bReload)
				return nullptr;
			auto skin = std::unique_ptr<WISkin> {new TSkin {id}};
			return static_cast<TSkin *>(RegisterSkin(id, std::move(skin)));
		}
		WISkin *RegisterSkin(std::string id, std::unique_ptr<WISkin> &&skin);
		void SetSkin(std::string skin);
		std::string GetSkinName();
		WISkin *GetSkin();
		WISkin *GetSkin(std::string name);
		void ClearSkins();
		void SetCursor(platform::Cursor::Shape cursor, prosper::Window *optWindow = nullptr);
		void SetCursor(platform::Cursor &cursor, prosper::Window *optWindow = nullptr);
		void ResetCursor(prosper::Window *optWindow = nullptr);
		void SetCursorInputMode(platform::CursorMode mode, prosper::Window *optWindow = nullptr);
		platform::Cursor::Shape GetCursor(const prosper::Window *optWindow = nullptr);
		platform::CursorMode GetCursorInputMode(const prosper::Window *optWindow = nullptr);
		material::MaterialManager &GetMaterialManager();
		void SetMaterialLoadHandler(const std::function<material::Material *(const std::string &)> &handler);
		const std::function<material::Material *(const std::string &)> &GetMaterialLoadHandler() const;
		Element *GetGUIElement(Element *el, int32_t x, int32_t y, const std::function<bool(Element *)> &condition, const prosper::Window *optWindow = nullptr);
		Element *GetCursorGUIElement(Element *el, const std::function<bool(Element *)> &condition, const prosper::Window *optWindow = nullptr);
		double GetDeltaTime() const;
		prosper::IRenderPass &GetMsaaRenderPass() const;
		//prosper::util::record_set_scissor(*drawCmd,szScissor[0],szScissor[1],posScissor[0],posScissor[1]); // Use parent scissor values
		bool IsLockedForDrawing() const { return m_lockedForDrawing; }
		void SetLockedForDrawing(bool locked) { m_lockedForDrawing = locked; }
		uint64_t GetNextGuiElementIndex() const { return m_nextGuiElementIndex; }
		void BeginDraw();
		void EndDraw();

		size_t GetLastThinkIndex() const;
		TooltipManager &GetTooltipManager() { return m_tooltipManager; }

		template<shaders::ShaderType S>
		shaders::ShaderTraits<S>::type *GetShader()
		{
			return static_cast<shaders::ShaderTraits<S>::type *>(m_shaders[math::to_integral(S)].get());
		}

		template<class T>
		void RegisterType(const std::string &name)
		{
			m_typeFactory->AddClass(name, typeid(T), []() -> Element * { return GetInstance().Create<T>(); });
		}
		const TypeFactory &GetTypeFactory() const { return *m_typeFactory; }
		const util::Heap *GetHeap() const { return m_guiHeap; }
	  private:
		void ScheduleElementForUpdate(Element &el, bool force = false);
		void RegisterTypes();
		void SetupElement(Element &el, const std::string className);
		friend Element;
		friend shaders::Shader;
		friend shaders::ShaderColoredRect;

		std::unique_ptr<TypeFactory> m_typeFactory;
		std::atomic<bool> m_lockedForDrawing = false;
		WISkin *m_skin = nullptr;
		bool m_bGUIUpdateRequired = false;
		std::unordered_map<std::string, std::unique_ptr<WISkin>> m_skins = {};
		std::weak_ptr<material::MaterialManager> m_matManager = {};
		std::function<material::Material *(const std::string &)> m_materialLoadHandler = nullptr;
		std::shared_ptr<prosper::IUniformResizableBuffer> m_elementBuffer = nullptr;
		std::vector<WIHandle> m_rootElements {};
		std::shared_ptr<prosper::IRenderPass> m_msaaRenderPass = nullptr;
		const prosper::Window *GetWindow(const prosper::Window *window) const { return const_cast<WGUI *>(this)->GetWindow(const_cast<prosper::Window *>(window)); }
		prosper::Window *GetWindow(prosper::Window *window);
		const types::WIRoot *GetRootElement(const types::WIRoot *elRoot) const { return const_cast<WGUI *>(this)->GetRootElement(const_cast<types::WIRoot *>(elRoot)); }
		types::WIRoot *GetRootElement(types::WIRoot *elRoot);
		void ClearWindow(const prosper::Window &window);
		std::vector<WIHandle> m_windowRootElements {};
		uint64_t m_nextGuiElementIndex = 0u;
		LocaleResolver m_localeResolver;
		TooltipManager m_tooltipManager;
		util::HeapManager *m_heapManager = nullptr;
		const util::Heap *m_guiHeap = nullptr;

		// In general very few elements actually need to apply any continuous logic,
		// so we keep a separate reference to those elements for better efficiency.
		std::vector<WIHandle> m_thinkingElements;

		std::priority_queue<UpdateInfo, std::vector<UpdateInfo>, UpdatePriority> m_updateQueue;
		std::optional<uint32_t> m_currentUpdateDepth = {};
		std::queue<WIHandle> m_removeQueue;

		std::function<void(Element &)> m_createCallback = nullptr;
		std::function<void(Element &)> m_removeCallback = nullptr;
		std::function<void(Element *, Element *)> m_onFocusChangedCallback = nullptr;

		std::function<void(Element &, platform::MouseButton, platform::KeyState, platform::Modifier)> m_mouseButtonCallback = nullptr;
		std::function<void(Element &, platform::Key, int, platform::KeyState, platform::Modifier)> m_keyboardCallback;
		std::function<void(Element &, unsigned int)> m_charCallback;
		std::function<void(Element &, Vector2)> m_scrollCallback;

		ChronoTime m_time = {};
		double m_tLastThink = 0;
		double m_tDelta = 0.f;
		size_t m_thinkIndex = 0;

		std::array<util::WeakHandle<prosper::Shader>, math::to_integral(shaders::ShaderType::Count)> m_shaders;

		bool SetFocusedElement(Element *gui, types::WIRoot *optElRoot = nullptr);
		void ClearSkin();
	};
};
export {
	REGISTER_ENUM_FLAGS(pragma::gui::WGUI::ElementBuffer)
}
