/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIBASE_H__
#define __WIBASE_H__
#include <vector>
#include "wiincludes.h"
#include "wguifactories.h"
#include <sharedutils/callback_handler.h>
#include <mathutil/umath.h>
#include "wgui/wihandle.h"
#include "wgui/wiattachment.hpp"
#include "wgui/wianchor.hpp"
#include <algorithm>
#include <chrono>
#include <deque>
#include <config.h>
#include <wrappers/buffer.h>
#include <iglfw/glfw_window.h>
#include <sharedutils/property/util_property_vector.h>
#include <sharedutils/property/util_property_color.hpp>
#include <sharedutils/util_event_reply.hpp>
#include <sharedutils/util_clock.hpp>

struct DLLWGUI WIFadeInfo
{
	WIFadeInfo(float tFade)
		: timeLeft(tFade),removeOnFaded(false),
		alphaTarget(1.f)
	{}
	float timeLeft;
	float alphaTarget;
	bool removeOnFaded;
};

using ChronoTimePoint = util::Clock::time_point;
using ChronoDuration = util::Clock::duration;

class WIHandle;
class WISkin;
class WGUI;
namespace util {class ColorProperty;};
namespace prosper {class Buffer;};
class DLLWGUI WIBase
	: public CallbackHandler
{
protected:
	static float RENDER_ALPHA;
	static std::deque<WIHandle> m_focusTrapStack;
public:
	friend WIHandle;
	friend WGUI;
public:
	enum class StateFlags : uint32_t
	{
		None = 0u,
		AcceptMouseInputBit = 1u,
		AcceptKeyboardInputBit = AcceptMouseInputBit<<1u,
		AcceptScrollInputBit = AcceptKeyboardInputBit<<1u,
		MouseCheckEnabledBit = AcceptScrollInputBit<<1u,
		AutoAlignToParentXBit = MouseCheckEnabledBit<<1u,
		AutoAlignToParentYBit = AutoAlignToParentXBit<<1u,
		AutoCenterToParentXBit = AutoAlignToParentYBit<<1u,
		AutoCenterToParentYBit = AutoCenterToParentXBit<<1u,
		TrapFocusBit = AutoCenterToParentYBit<<1u,
		ShouldScissorBit = TrapFocusBit<<1u,
		UpdateScheduledBit = ShouldScissorBit<<1u,
		RemoveScheduledBit = UpdateScheduledBit<<1u,
		SkinAppliedBit = RemoveScheduledBit<<1u,
		ClickedBit = SkinAppliedBit<<1u,
		ThinkIfInvisibleBit = ClickedBit<<1u,
		IgnoreParentAlpha = ThinkIfInvisibleBit<<1u,
		RenderIfZeroAlpha = IgnoreParentAlpha<<1u,
		UpdatingAnchorTransform = RenderIfZeroAlpha<<1u,
		ThinkingEnabled = UpdatingAnchorTransform<<1u,
		ParentVisible = ThinkingEnabled<<1u,
		AutoSizeToContentsX = ParentVisible<<1u,
		AutoSizeToContentsY = AutoSizeToContentsX<<1u
	};
	static void CalcBounds(const Mat4 &mat,int32_t w,int32_t h,Vector2i &outPos,Vector2i &outSize);

	WIBase();
	virtual ~WIBase();
	WIBase(const WIBase&)=delete;
	WIBase &operator=(const WIBase&)=delete;
	GLFW::Cursor::Shape GetCursor() const;
	void SetCursor(GLFW::Cursor::Shape cursor);
	void Resize();
	void SetSkin(std::string skin);
	void ResetSkin();
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
	void SetAutoAlignToParent(bool bX,bool bY);
	void SetAutoAlignToParent(bool b);
	void SetAutoCenterToParentX(bool b);
	void SetAutoCenterToParentY(bool b);
	void SetAutoCenterToParent(bool b);
	void CenterToParentX();
	void CenterToParentY();
	void CenterToParent();
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
	virtual void Think();
	virtual bool HasFocus();
	virtual void RequestFocus();
	const util::PBoolProperty &GetFocusProperty() const;
	void KillFocus(bool bForceKill=false);
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
	bool GetMouseMovementCheckEnabled();
	virtual void SetMouseInputEnabled(bool b);
	virtual void SetKeyboardInputEnabled(bool b);
	virtual void SetScrollInputEnabled(bool b);
	void SetMouseMovementCheckEnabled(bool b);
	void Update();
	virtual void SizeToContents(bool x=true,bool y=true);
	const util::PVector2iProperty &GetPosProperty() const;
	const Vector2i &GetPos() const;
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
	void SetWidth(int w);
	void SetHeight(int h);
	Vector2i GetAbsolutePos() const;
	void SetAbsolutePos(Vector2i pos);
	std::vector<WIHandle> *GetChildren();
	void GetChildren(const std::string &className,std::vector<WIHandle> &children);
	WIBase *GetFirstChild(const std::string &className);
	WIBase *GetChild(unsigned int idx);
	WIBase *GetChild(const std::string &className,unsigned int idx);
	WIBase *FindChildByName(const std::string &name);
	void FindChildrenByName(const std::string &name,std::vector<WIHandle> &children);
	WIBase *FindDescendantByName(const std::string &name);
	void FindDescendantsByName(const std::string &name,std::vector<WIHandle> &children);
	void GetPos(int *x,int *y) const;
	void SetPos(const Vector2i &pos);
	virtual void SetPos(int x,int y);
	const Color &GetColor() const;
	const std::shared_ptr<util::ColorProperty> &GetColorProperty() const;
	void SetColor(const Vector4 &col);
	void SetColor(const Color &col);
	virtual void SetColor(float r,float g,float b,float a=1.f);
	float GetAlpha() const;
	virtual void SetAlpha(float alpha);
	int GetWidth() const;
	int GetHeight() const;
	const util::PVector2iProperty &GetSizeProperty() const;
	const Vector2i &GetSize() const;
	void GetSize(int *w,int *h);
	void SetSize(const Vector2i &size);
	virtual void SetSize(int x,int y);
	virtual void Draw(int w,int h);
	void Draw(int w,int h,const Vector2i &origin,Vector2i offsetParent,Vector2i scissorOffset,Vector2i scissorSize,Mat4 mat,bool bUseScissor=true,const Mat4 *matPostTransform=nullptr);
	void Draw(int w,int h,const Vector2i &origin,const Vector2i &offsetParent,const Mat4 &mat=Mat4(1.f),bool bUseScissor=true,const Mat4 *matPostTransform=nullptr);
	void Draw(int w,int h,const Vector2i &offsetParent,const Vector2i &scissorOffset,const Vector2i &scissorSize,const Mat4 &mat,bool bUseScissor=true,const Mat4 *matPostTransform=nullptr);
	void Draw(int w,int h,const Vector2i &offsetParent,const Mat4 &mat,bool bUseScissor=true,const Mat4 *matPostTransform=nullptr);
	virtual void SetParent(WIBase *base);
	WIBase *GetParent() const;
	void ClearParent();
	void RemoveChild(WIBase *child);
	void AddChild(WIBase *child);
	bool HasChild(WIBase *child);
	virtual Mat4 GetTransformedMatrix(const Vector2i &origin,int w,int h,Mat4 mat) const;
	Mat4 GetTransformedMatrix(int w,int h,Mat4 mat) const;
	Mat4 GetTransformedMatrix(int w,int h) const;
	Mat4 GetTranslatedMatrix(const Vector2i &origin,int w,int h,Mat4 mat) const;
	Mat4 GetTranslatedMatrix(int w,int h,Mat4 mat) const;
	Mat4 GetTranslatedMatrix(int w,int h) const;
	Mat4 GetScaledMatrix(int w,int h,Mat4 mat) const;
	Mat4 GetScaledMatrix(int w,int h) const;
	bool PosInBounds(int x,int y) const;
	bool PosInBounds(Vector2i pos) const;
	const util::PBoolProperty &GetMouseInBoundsProperty() const;
	bool MouseInBounds() const;
	void ScheduleUpdate();
	virtual void OnCursorEntered();
	virtual void OnCursorExited();
	virtual void OnCursorMoved(int x,int y);
	virtual util::EventReply MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods);
	virtual util::EventReply OnDoubleClick();
	virtual util::EventReply JoystickCallback(const GLFW::Joystick &joystick,uint32_t key,GLFW::KeyState state);
	virtual util::EventReply KeyboardCallback(GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods);
	virtual util::EventReply CharCallback(unsigned int c,GLFW::Modifier mods=GLFW::Modifier::None);
	virtual util::EventReply ScrollCallback(Vector2 offset);

	void InjectMouseMoveInput(int32_t x,int32_t y);
	util::EventReply InjectMouseInput(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods);
	util::EventReply InjectKeyboardInput(GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods);
	util::EventReply InjectCharInput(unsigned int c,GLFW::Modifier mods=GLFW::Modifier::None);
	util::EventReply InjectScrollInput(Vector2 offset);

	void GetMousePos(int *x,int *y) const;
	void Remove();
	void RemoveSafely();
	void RemoveOnRemoval(WIBase *other);
	void FadeIn(float tFade=0.25f,float alphaTarget=1.f);
	void FadeOut(float tFade=0.25f,bool removeOnFaded=false);
	bool IsFading() const;
	bool IsFadingIn() const;
	bool IsFadingOut() const;
	void SetIgnoreParentAlpha(bool ignoreParentAlpha);
	bool ShouldIgnoreParentAlpha() const;

	void SetThinkIfInvisible(bool bThinkIfInvisible);
	void SetRenderIfZeroAlpha(bool renderIfZeroAlpha);

	virtual std::string GetDebugInfo() const;

	bool IsDescendant(WIBase *el);
	bool IsDescendantOf(WIBase *el);
	bool IsAncestor(WIBase *el);
	bool IsAncestorOf(WIBase *el);

	std::vector<std::string> &GetStyleClasses();
	void AddStyleClass(const std::string &className);

	void SetTooltip(const std::string &msg);
	const std::string &GetTooltip() const;
	bool HasTooltip() const;

	WIAttachment *GetAttachment(const std::string &name);
	const WIAttachment *GetAttachment(const std::string &name) const;
	WIAttachment *AddAttachment(const std::string &name,const Vector2 &position={});
	void SetAttachmentPos(const std::string &name,const Vector2 &position);
	const Vector2 *GetAttachmentPos(const std::string &name) const;
	const Vector2i *GetAbsoluteAttachmentPos(const std::string &name) const;
	const util::PVector2iProperty *GetAttachmentPosProperty(const std::string &name) const;

	void ClearAnchor();
	void SetAnchor(float left,float top,float right,float bottom);
	void SetAnchor(float left,float top,float right,float bottom,uint32_t refWidth,uint32_t refHeight);
	void SetAnchorLeft(float f);
	void SetAnchorRight(float f);
	void SetAnchorTop(float f);
	void SetAnchorBottom(float f);
	bool GetAnchor(float &outLeft,float &outTop,float &outRight,float &outBottom) const;
	bool HasAnchor() const;
	std::pair<Vector2,Vector2> GetAnchorBounds() const;
	std::pair<Vector2,Vector2> GetAnchorBounds(uint32_t refWidth,uint32_t refHeight) const;

	WIBase *Wrap(const std::string &wrapperClass);
	bool Wrap(WIBase &wrapper);

	void SetAutoSizeToContents(bool x,bool y);
	void SetAutoSizeToContents(bool autoSize);
	bool ShouldAutoSizeToContentsX() const;
	bool ShouldAutoSizeToContentsY() const;

	void EnableThinking();
	void DisableThinking();
	void SetThinkingEnabled(bool enabled);

	uint64_t GetIndex() const;

	// Handles
	WIHandle GetHandle() const;
protected:
	void SetIndex(uint64_t idx);
	void UpdateAutoSizeToContents();
	virtual void DoUpdate();
	void UpdateVisibility();
	void UpdateAnchorTransform();
	void UpdateAnchorTopLeftPixelOffsets();
	void UpdateAnchorBottomRightPixelOffsets();
	uint64_t m_index = std::numeric_limits<uint64_t>::max();
	StateFlags m_stateFlags = StateFlags::ShouldScissorBit;
	std::array<std::shared_ptr<void>,4> m_userData;
	std::shared_ptr<WIHandle> m_handle = nullptr;
	std::string m_class = "WIBase";
	std::string m_name;
	std::string m_toolTip;
	std::optional<WIAnchor> m_anchor = {};
	std::unordered_map<std::string,std::shared_ptr<WIAttachment>> m_attachments = {};
	std::unique_ptr<WIFadeInfo> m_fade = nullptr;
	GLFW::Cursor::Shape m_cursor = {};
	CallbackHandle m_callbackFocusGained = {};
	CallbackHandle m_callbackFocusKilled = {};
	Mat4 m_mvpLast = umat::identity();
	Vector4 m_colorLast = {0.f,0.f,0.f,1.f};
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
	virtual void Render(int w,int h,const Mat4 &mat,const Vector2i &origin,const Mat4 &matParent);
	void UpdateChildOrder(WIBase *child=NULL);
	template<class TElement>
		WIHandle CreateChild();
	virtual void InitializeHandle();
	template<class TWrapper>
		void InitializeWrapper();
	void InitializeHandle(std::shared_ptr<WIHandle> handle);
	template<class THandle>
		void InitializeHandle();
	void UpdateMouseInBounds();
	void UpdateChildrenMouseInBounds();
	virtual void OnVisibilityChanged(bool bVisible);
	void ApplySkin(WISkin *skin=nullptr);
	WISkin *GetSkin();
	void SetAutoAlignToParent(bool bX,bool bY,bool bReload);
	void SetAutoCenterToParentX(bool b,bool bReload);
	void SetAutoCenterToParentY(bool b,bool bReload);
	virtual void OnChildAdded(WIBase *child);
	virtual void OnChildRemoved(WIBase *child);
	virtual void OnRemove();

	bool ShouldThink() const;
private:
	void UpdateThink();
	WIBase *FindDeepestChild(const std::function<bool(const WIBase&)> &predInspect,const std::function<bool(const WIBase&)> &predValidCandidate);
	util::PVector2iProperty m_pos = nullptr;
	util::PVector2iProperty m_size = nullptr;
	util::PColorProperty m_color = nullptr;
	util::PBoolProperty m_bMouseInBounds = nullptr;
	util::PBoolProperty m_bVisible = nullptr;
	util::PBoolProperty m_bHasFocus = nullptr;
private:
	std::vector<std::string> m_styleClasses;
	WISkin *m_skin = nullptr;
	ChronoTimePoint m_clickStart;
	static bool __wiJoystickCallback(GLFW::Window &window,const GLFW::Joystick &joystick,uint32_t key,GLFW::KeyState state);
	static bool __wiKeyCallback(GLFW::Window &window,GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods);
	static bool __wiCharCallback(GLFW::Window &window,unsigned int c);
	static bool __wiMouseButtonCallback(GLFW::Window &window,GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods);
	static bool __wiScrollCallback(GLFW::Window &window,Vector2 offset);
};
REGISTER_BASIC_BITWISE_OPERATORS(WIBase::StateFlags)

#include "wgui.h"

template<class THandle>
	void WIBase::InitializeHandle()
{
	m_handle = std::make_shared<THandle>(new PtrWI(this));
}

template<class TElement>
	WIHandle WIBase::CreateChild()
{
	TElement *pGUI = WGUI::GetInstance().Create<TElement>(this);
	return pGUI->GetHandle();
}

#endif