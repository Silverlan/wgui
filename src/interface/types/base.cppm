// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"
#include <vector>
#include <algorithm>
#include <chrono>
#include <deque>
#include <functional>

export module pragma.gui:types.base;

import :anchor;
import :attachment;
import :core;
import :draw_info;
import :draw_state;
import :enums;
import :handle;
import pragma.platform;
export import pragma.prosper;
export import pragma.util;

#undef DrawState

export {
	struct DLLWGUI WIFadeInfo {
		WIFadeInfo(float tFade) : timeLeft(tFade), removeOnFaded(false), alphaTarget(1.f) {}
		float timeLeft;
		float alphaTarget;
		bool removeOnFaded;
	};

	using ChronoTimePoint = util::Clock::time_point;
	using ChronoDuration = util::Clock::duration;

    class WGUI;
	class WIRoot;
	class WISkin;
	namespace wgui {
		class DLLWGUI WIBase : public CallbackHandler {
		public:
			friend WGUI;
		public:
			enum class StateFlags : uint64_t {
				None = 0u,
				AcceptMouseInputBit = 1u,
				AcceptKeyboardInputBit = AcceptMouseInputBit << 1u,
				AcceptScrollInputBit = AcceptKeyboardInputBit << 1u,
				MouseCheckEnabledBit = AcceptScrollInputBit << 1u,
				AutoAlignToParentXBit = MouseCheckEnabledBit << 1u,
				AutoAlignToParentYBit = AutoAlignToParentXBit << 1u,
				AutoCenterToParentXBit = AutoAlignToParentYBit << 1u,
				AutoCenterToParentYBit = AutoCenterToParentXBit << 1u,
				TrapFocusBit = AutoCenterToParentYBit << 1u,
				ShouldScissorBit = TrapFocusBit << 1u,
				UpdateScheduledBit = ShouldScissorBit << 1u,
				RemoveScheduledBit = UpdateScheduledBit << 1u,
				SkinAppliedBit = RemoveScheduledBit << 1u,
				ClickedBit = SkinAppliedBit << 1u,
				UpdateIfInvisibleBit = ClickedBit << 1u,
				IgnoreParentAlpha = UpdateIfInvisibleBit << 1u,
				RenderIfZeroAlpha = IgnoreParentAlpha << 1u,
				UpdatingAnchorTransform = RenderIfZeroAlpha << 1u,
				ThinkingEnabled = UpdatingAnchorTransform << 1u,
				ParentVisible = ThinkingEnabled << 1u,
				ParentUpdateIfInvisibleBit = ParentVisible << 1u,
				AutoSizeToContentsX = ParentUpdateIfInvisibleBit << 1u,
				AutoSizeToContentsY = AutoSizeToContentsX << 1u,
				IsBeingRemoved = AutoSizeToContentsY << 1u,
				IsBeingUpdated = IsBeingRemoved << 1u,
				IsBackgroundElement = IsBeingUpdated << 1u,
				FirstThink = IsBackgroundElement << 1u,
				IsBaseElement = FirstThink << 1u,
				DontRemoveOnParentRemoval = IsBaseElement << 1u,
				StencilEnabled = DontRemoveOnParentRemoval << 1u,
				FullyTransparent = StencilEnabled << 1u,
				ScheduleUpdateOnVisible = FullyTransparent << 1u,
				SkinCallbacksEnabled = ScheduleUpdateOnVisible << 1u,
				IsInThinkingList = SkinCallbacksEnabled << 1u,
				FileDropInputEnabled = IsInThinkingList << 1u,
				FileDropHover = FileDropInputEnabled << 1u,
			};
			static void CalcBounds(const Mat4 &mat, int32_t w, int32_t h, Vector2i &outPos, Vector2i &outSize);

			WIBase();
			virtual ~WIBase();
			WIBase(const WIBase &) = delete;
			WIBase &operator=(const WIBase &) = delete;
			virtual std::ostream &Print(std::ostream &stream) const;
			pragma::platform::Cursor::Shape GetCursor() const;
			void SetCursor(pragma::platform::Cursor::Shape cursor);
			void Resize();
			void SetSkin(std::string skin);
			void ResetSkin();
			std::optional<std::string> GetSkinName() const;
			void SetSkinCallbacksEnabled(bool enabled);
			const std::string &GetName() const;
			void SetName(const std::string &name);
			const std::shared_ptr<void> &GetUserData();
			const std::shared_ptr<void> &GetUserData2();
			const std::shared_ptr<void> &GetUserData3();
			const std::shared_ptr<void> &GetUserData4();
			void SetUserData(const std::shared_ptr<void> &userData);
			void SetUserData2(const std::shared_ptr<void> &userData);
			void SetUserData3(const std::shared_ptr<void> &userData);
			void SetUserData4(const std::shared_ptr<void> &userData);
			void SetShouldScissor(bool b);
			bool GetShouldScissor() const;
			void SetAutoAlignToParent(bool bX, bool bY);
			void SetAutoAlignToParent(bool b);
			void SetAutoCenterToParentX(bool b);
			void SetAutoCenterToParentY(bool b);
			void SetAutoCenterToParent(bool b);
			void CenterToParentX();
			void CenterToParentY();
			bool GetAutoAlignToParentX() const;
			bool GetAutoAlignToParentY() const;
			bool GetAutoAlignToParent() const;
			virtual void Initialize();
			void TrapFocus(bool b);
			bool IsFocusTrapped();
			virtual std::string GetClass() const;
			void CallOnRemove(CallbackHandle callback);
			// Anything with a higher z-position will be drawn in front of everything with a lower one.
			virtual void SetZPos(int zpos);
			int GetZPos() const;
			virtual void Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd);
			virtual bool HasFocus();
			virtual void RequestFocus();
			const util::PBoolProperty &GetFocusProperty() const;
			void KillFocus(bool bForceKill = false);
			virtual void OnFocusGained();
			virtual void OnFocusKilled();
			virtual void OnDescendantFocusGained(WIBase &el);
			virtual void OnDescendantFocusKilled(WIBase &el);
			const util::PBoolProperty &GetVisibilityProperty() const;
			bool IsVisible() const;
			bool IsParentVisible() const;
			bool IsSelfVisible() const;
			virtual void SetVisible(bool b);
			bool GetMouseInputEnabled() const;
			bool GetKeyboardInputEnabled() const;
			bool GetScrollInputEnabled() const;
			bool GetFileDropInputEnabled() const;
			bool GetMouseMovementCheckEnabled();
			virtual void SetMouseInputEnabled(bool b);
			virtual void SetKeyboardInputEnabled(bool b);
			virtual void SetScrollInputEnabled(bool b);
			virtual void SetFileDropInputEnabled(bool b);
			void SetMouseMovementCheckEnabled(bool b);
			void Update();
			virtual void SizeToContents(bool x = true, bool y = true);
			const util::PVector2iProperty &GetPosProperty() const;
			const Vector2i &GetPos() const;
			Vector2 GetCenter() const;
			float GetCenterX() const;
			float GetCenterY() const;
			void SetCenterPos(const Vector2i &pos);
			void CenterToParent(bool applyAnchor = false);
			Vector2 GetHalfSize() const;
			float GetHalfWidth() const;
			float GetHalfHeight() const;
			int GetX() const;
			int GetY() const;
			int GetLeft() const;
			int GetTop() const;
			int GetRight() const;
			int GetBottom() const;
			void SetLeft(int32_t pos);
			void SetRight(int32_t pos);
			void SetTop(int32_t pos);
			void SetBottom(int32_t pos);
			Vector2i GetEndPos() const;
			void SetX(int x);
			void SetY(int y);
			float GetAspectRatio() const;
			void SetWidth(int w, bool keepRatio = false);
			void SetHeight(int h, bool keepRatio = false);
			Mat4 GetAbsolutePose() const;
			Vector2 GetAbsolutePos(const Vector2 &localPos, bool includeRotation = true) const;
			Vector2 GetAbsolutePos(bool includeRotation = true) const;
			void SetAbsolutePos(const Vector2 &pos);
			Vector2 GetRelativePos(const Vector2 &absPos) const;
			std::vector<WIHandle> *GetChildren();
			void GetChildren(const std::string &className, std::vector<WIHandle> &children);
			WIBase *GetFirstChild(const std::string &className);
			WIBase *GetChild(unsigned int idx);
			WIBase *GetChild(const std::string &className, unsigned int idx);
			WIBase *FindChildByName(const std::string &name);
			void FindChildrenByName(const std::string &name, std::vector<WIHandle> &children);
			WIBase *FindDescendantByName(const std::string &name);
			void FindDescendantsByName(const std::string &name, std::vector<WIHandle> &children);
			void GetPos(int *x, int *y) const;
			void SetPos(const Vector2i &pos);
			virtual void SetPos(int x, int y);
			umath::intersection::Intersect IsInBounds(int x, int y, int w = 0, int h = 0) const;
			const Color &GetColor() const;
			const std::shared_ptr<util::ColorProperty> &GetColorProperty() const;
			void SetColor(const Vector4 &col);
			void SetColor(const Color &col);
			virtual void SetColor(float r, float g, float b, float a = 1.f);
			float GetAlpha() const;
			virtual void SetAlpha(float alpha);
			float GetLocalAlpha() const { return m_localAlpha; }
			void SetLocalAlpha(float a);
			int GetWidth() const;
			int GetHeight() const;
			const util::PVector2iProperty &GetSizeProperty() const;
			const Vector2i &GetSize() const;
			void GetSize(int *w, int *h);
			void SetSize(const Vector2i &size);
			virtual void SetSize(int x, int y);
			virtual void Draw(int w, int h, std::shared_ptr<prosper::ICommandBuffer> &cmdBuf);
			void Draw(const wgui::DrawInfo &drawInfo, wgui::DrawState &drawState, const Vector2i &offsetParent, const Vector2i &scissorOffset, const Vector2i &scissorSize, const Vector2 &scale, uint32_t testStencilLevel = 0u);
			void Draw(const wgui::DrawInfo &drawInfo, wgui::DrawState &drawState);
			virtual void SetParent(WIBase *base, std::optional<uint32_t> childIndex = {});
			void SetParentAndUpdateWindow(WIBase *base, std::optional<uint32_t> childIndex = {});
			WIBase *GetParent() const;
			void RemoveChild(WIBase *child);
			void AddChild(WIBase *child, std::optional<uint32_t> childIndex = {});
			bool HasChild(WIBase *child);
			std::optional<uint32_t> FindChildIndex(WIBase &child) const;
			virtual Mat4 GetTransformPose(const Vector2i &origin, int w, int h, const Mat4 &poseParent, const Vector2 &scale = {1.f, 1.f}) const;
			Mat4 GetTransformPose(int w, int h, const Mat4 &poseParent) const;
			Mat4 GetTransformPose(int w, int h) const;
			Mat4 GetTranslationPose(const Vector2i &origin, int w, int h, const Mat4 &poseParent) const;
			Mat4 GetTranslationPose(int w, int h, const Mat4 &poseParent) const;
			Mat4 GetTranslationPose(int w, int h) const;
			Mat4 GetScaledMatrix(int w, int h, const Mat4 &poseParent) const;
			Mat4 GetScaledMatrix(int w, int h) const;
			bool PosInBounds(int x, int y) const;
			bool PosInBounds(const Vector2i &pos) const;
			const util::PBoolProperty &GetMouseInBoundsProperty() const;
			bool MouseInBounds() const;
			void ScheduleUpdate();
			bool IsUpdateScheduled() const;
			bool IsRemovalScheduled() const;
			virtual void OnCursorEntered();
			virtual void OnCursorExited();
			virtual void OnCursorMoved(int x, int y);
			virtual void OnFileDragEntered();
			virtual void OnFileDragExited();
			virtual util::EventReply OnFilesDropped(const std::vector<std::string> &files);
			bool IsFileHovering() const;
			void SetFileHovering(bool hover);
			virtual util::EventReply MouseCallback(pragma::platform::MouseButton button, pragma::platform::KeyState state, pragma::platform::Modifier mods);
			virtual util::EventReply OnDoubleClick();
			virtual util::EventReply JoystickCallback(const pragma::platform::Joystick &joystick, uint32_t key, pragma::platform::KeyState state);
			virtual util::EventReply KeyboardCallback(pragma::platform::Key key, int scanCode, pragma::platform::KeyState state, pragma::platform::Modifier mods);
			virtual util::EventReply CharCallback(unsigned int c, pragma::platform::Modifier mods = pragma::platform::Modifier::None);
			virtual util::EventReply ScrollCallback(Vector2 offset, bool offsetAsPixels = false);

			void ClampToBounds(Vector2i &pos) const;
			void ClampToBounds(Vector2i &pos, Vector2i &size) const;
			void GetAbsoluteVisibleBounds(Vector2i &pos, Vector2i &size, Vector2i *optOutParentPos = nullptr) const;
			void GetVisibleBounds(Vector2i &pos, Vector2i &size) const;
			void ClampToVisibleBounds(Vector2i &pos) const;
			void ClampToVisibleBounds(Vector2i &pos, Vector2i &size) const;

			void SetStencilEnabled(bool enabled);
			bool IsStencilEnabled() const;

			void SetIMETarget();
			void ClearIMETarget();

			void SetBackgroundElement(bool backgroundElement, bool autoAlignToParent = true);
			bool IsBackgroundElement() const;

			void SetBaseElement(bool baseElement);
			bool IsBaseElement() const;

			void InjectMouseMoveInput(int32_t x, int32_t y);
			util::EventReply InjectMouseInput(pragma::platform::MouseButton button, pragma::platform::KeyState state, pragma::platform::Modifier mods);
			util::EventReply InjectKeyboardInput(pragma::platform::Key key, int scanCode, pragma::platform::KeyState state, pragma::platform::Modifier mods);
			util::EventReply InjectCharInput(unsigned int c, pragma::platform::Modifier mods = pragma::platform::Modifier::None);
			util::EventReply InjectScrollInput(Vector2 offset, bool offsetAsPixels = false);

			void GetMousePos(int *x, int *y) const;
			void Remove();
			void RemoveSafely();
			void RemoveOnRemoval(WIBase *other);
			void FadeIn(float tFade = 0.25f, float alphaTarget = 1.f);
			void FadeOut(float tFade = 0.25f, bool removeOnFaded = false);
			bool IsFading() const;
			bool IsFadingIn() const;
			bool IsFadingOut() const;
			void SetIgnoreParentAlpha(bool ignoreParentAlpha);
			bool ShouldIgnoreParentAlpha() const;

			void SetThinkIfInvisible(bool bThinkIfInvisible);
			bool ShouldThinkIfInvisible() const;
			void SetRenderIfZeroAlpha(bool renderIfZeroAlpha);

			virtual std::string GetDebugInfo() const;

			bool IsDescendant(WIBase *el);
			bool IsDescendantOf(WIBase *el);
			bool IsAncestor(WIBase *el);
			bool IsAncestorOf(WIBase *el);

			std::vector<std::string> &GetStyleClasses();
			void AddStyleClass(const std::string &className);
			void RemoveStyleClass(const std::string &className);
			void ClearStyleClasses();

			void SetTooltip(const std::string &msg);
			const std::string &GetTooltip() const;
			bool HasTooltip() const;

			WIAttachment *GetAttachment(const std::string &name);
			const WIAttachment *GetAttachment(const std::string &name) const;
			WIAttachment *AddAttachment(const std::string &name, const Vector2 &position = {});
			void SetAttachmentPos(const std::string &name, const Vector2 &position);
			const Vector2 *GetAttachmentPos(const std::string &name) const;
			const Vector2i *GetAbsoluteAttachmentPos(const std::string &name) const;
			const util::PVector2iProperty *GetAttachmentPosProperty(const std::string &name) const;

			void ClearAnchor();
			void SetAnchor(float left, float top, float right, float bottom);
			void SetAnchor(float left, float top, float right, float bottom, uint32_t refWidth, uint32_t refHeight);
			void AnchorWithMargin(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);
			void AnchorWithMargin(uint32_t margin);
			void SetAnchorLeft(float f);
			void SetAnchorRight(float f);
			void SetAnchorTop(float f);
			void SetAnchorBottom(float f);
			bool GetAnchor(float &outLeft, float &outTop, float &outRight, float &outBottom) const;
			bool HasAnchor() const;
			std::pair<Vector2, Vector2> GetAnchorBounds() const;
			std::pair<Vector2, Vector2> GetAnchorBounds(uint32_t refWidth, uint32_t refHeight) const;

			uint32_t GetDepth() const { return m_depth; }

			WIBase *Wrap(const std::string &wrapperClass);
			bool Wrap(WIBase &wrapper);

			void SetAutoSizeToContents(bool x, bool y, bool updateImmediately = true);
			void SetAutoSizeToContents(bool autoSize);
			bool ShouldAutoSizeToContentsX() const;
			bool ShouldAutoSizeToContentsY() const;

			void EnableThinking();
			void DisableThinking();
			void SetThinkingEnabled(bool enabled);
			void ApplySkin(WISkin *skin = nullptr);
			void RefreshSkin();

			void SetRemoveOnParentRemoval(bool b);

			uint64_t GetIndex() const;

			void SetScale(const Vector2 &scale);
			void SetScale(float x, float y);
			const Vector2 &GetScale() const;
			const util::PVector2Property &GetScaleProperty() const;

			void ResetRotation();
			void SetRotation(umath::Degree angle, const Vector2 &pivot);
			void SetRotation(const Mat4 &rotationMatrix);
			const Mat4 *GetRotationMatrix() const;

			WIBase *GetRootElement();
			const WIBase *GetRootElement() const { return const_cast<WIBase *>(this)->GetRootElement(); }
			WIRoot *GetBaseRootElement();
			const WIRoot *GetBaseRootElement() const { return const_cast<WIBase *>(this)->GetBaseRootElement(); }
			prosper::Window *GetRootWindow();
			const prosper::Window *GetRootWindow() const { return const_cast<WIBase *>(this)->GetRootWindow(); }

			void SetLocalRenderTransform(const umath::ScaledTransform &transform);
			void ClearLocalRenderTransform();
			umath::ScaledTransform *GetLocalRenderTransform();
			const umath::ScaledTransform *GetLocalRenderTransform() const { return const_cast<WIBase *>(this)->GetLocalRenderTransform(); }
			void UpdateAutoSizeToContents(bool updateX = true, bool updateY = true);

			// Handles
			WIHandle GetHandle() const;
		protected:
			virtual bool DoPosInBounds(const Vector2i &pos) const;
			Mat4 GetAbsolutePose(float x, float y) const;
			Mat4 GetRelativePose(float x, float y) const;
			void AbsolutePosToRelative(Vector2 &pos) const;
			void SetIndex(uint64_t idx);
			void UpdateParentAutoSizeToContents();
			void UpdateCursorMove(int x, int y);
			void ClearParent();
			void UpdateVisibilityUpdateState();
			virtual void OnSkinApplied();
			virtual void OnFirstThink();
			virtual void DoUpdate();
			virtual util::EventReply OnMousePressed();
			virtual util::EventReply OnMouseReleased();
			virtual void UpdateTransparencyState();
			bool IsFullyTransparent() const;
			void UpdateVisibility();
			void UpdateParentThink();
			void UpdateAnchorTransform();
			void UpdateAnchorTopLeftPixelOffsets();
			void UpdateAnchorBottomRightPixelOffsets();
			uint64_t m_index = std::numeric_limits<uint64_t>::max();
			size_t m_lastThinkUpdateIndex = std::numeric_limits<size_t>::max();
			uint32_t m_depth = 0;
			StateFlags m_stateFlags = StateFlags::ShouldScissorBit;
			std::array<std::shared_ptr<void>, 4> m_userData;
			std::unique_ptr<umath::ScaledTransform> m_localRenderTransform = nullptr;
			util::TSharedHandle<WIBase> m_handle {};
			std::string m_class = "WIBase";
			std::string m_name;
			std::string m_toolTip;
			std::unique_ptr<Mat4> m_rotationMatrix = nullptr;
			std::optional<WIAnchor> m_anchor = {};
			std::unordered_map<std::string, std::shared_ptr<WIAttachment>> m_attachments = {};
			std::unique_ptr<WIFadeInfo> m_fade = nullptr;
			pragma::platform::Cursor::Shape m_cursor = {};
			CallbackHandle m_callbackFocusGained = {};
			CallbackHandle m_callbackFocusKilled = {};
			Mat4 m_mvpLast = umat::identity();
			float m_localAlpha = 1.f;
			Vector4 m_colorLast = {0.f, 0.f, 0.f, 1.f};
			int m_zpos = -1;
			CallbackHandle m_cbAutoAlign = {};
			CallbackHandle m_cbAutoCenterX = {};
			CallbackHandle m_cbAutoCenterXOwn = {};
			CallbackHandle m_cbAutoCenterY = {};
			CallbackHandle m_cbAutoCenterYOwn = {};
			int m_lastMouseX = 0;
			int m_lastMouseY = 0;
			std::vector<WIHandle> m_children;
			mutable WIHandle m_parent = {};
			virtual void Render(const wgui::DrawInfo &drawInfo, wgui::DrawState &drawState, const Mat4 &matDraw, const Vector2 &scale = {1.f, 1.f}, uint32_t testStencilLevel = 0u, wgui::StencilPipeline stencilPipeline = wgui::StencilPipeline::Test);
			void UpdateChildOrder(WIBase *child = NULL);
			template<class TElement>
			WIHandle CreateChild();
			void InitializeHandle();
			void UpdateMouseInBounds(bool forceFalse = false);
			void DoUpdateChildrenMouseInBounds(const Mat4 &parentPose, const Vector2 &cursorPos, bool ignoreVisibility, bool forceFalse);
			void UpdateChildrenMouseInBounds(bool ignoreVisibility = false, bool forceFalse = false);
			virtual void OnVisibilityChanged(bool bVisible);
			WISkin *GetSkin();
			void SetAutoAlignToParent(bool bX, bool bY, bool bReload);
			void SetAutoCenterToParentX(bool b, bool bReload);
			void SetAutoCenterToParentY(bool b, bool bReload);
			virtual void OnChildAdded(WIBase *child);
			virtual void OnChildRemoved(WIBase *child);
			virtual void OnRemove();

			bool ShouldThink() const;
		private:
			void UpdateThink();
			void UpdateMouseInBounds(const Vector2 &relPos, bool forceFalse);
			WIBase *FindDeepestChild(const std::function<bool(const WIBase &)> &predInspect, const std::function<bool(const WIBase &)> &predValidCandidate);
			util::PVector2iProperty m_pos = nullptr;
			util::PVector2iProperty m_size = nullptr;
			util::PColorProperty m_color = nullptr;
			util::PBoolProperty m_bMouseInBounds = nullptr;
			util::PBoolProperty m_bVisible = nullptr;
			util::PBoolProperty m_bHasFocus = nullptr;
			util::PVector2Property m_scale = nullptr;
		private:
			std::vector<std::string> m_styleClasses;
			WISkin *m_skin = nullptr;
			ChronoTimePoint m_clickStart;
			static bool __wiJoystickCallback(prosper::Window &window, const pragma::platform::Joystick &joystick, uint32_t key, pragma::platform::KeyState state);
			static bool __wiKeyCallback(prosper::Window &window, pragma::platform::Key key, int scanCode, pragma::platform::KeyState state, pragma::platform::Modifier mods);
			static bool __wiCharCallback(prosper::Window &window, unsigned int c);
			static bool __wiMouseButtonCallback(prosper::Window &window, pragma::platform::MouseButton button, pragma::platform::KeyState state, pragma::platform::Modifier mods);
			static bool __wiScrollCallback(prosper::Window &window, Vector2 offset);
			static bool __wiFileDragEnterCallback(prosper::Window &window);
			static bool __wiFileDragExitCallback(prosper::Window &window);
			static bool __wiFileDropCallback(prosper::Window &window, const std::vector<std::string> &files);

			static util::EventReply InjectMouseButtonCallback(WIBase &el, pragma::platform::MouseButton button, pragma::platform::KeyState state, pragma::platform::Modifier mods);
		};
		using namespace umath::scoped_enum::bitwise;
	}
	namespace umath::scoped_enum::bitwise {
		template<>
		struct enable_bitwise_operators<wgui::WIBase::StateFlags> : std::true_type {};

		template<>
		struct enable_bitwise_operators<wgui::DrawInfo::Flags> : std::true_type {};
	}

	DLLWGUI std::ostream &operator<<(std::ostream &stream, wgui::WIBase &el);

	template<class TElement>
	wgui::WIHandle wgui::WIBase::CreateChild()
	{
		TElement *pGUI = WGUI::GetInstance().Create<TElement>(this);
		return pGUI->GetHandle();
	}
};
