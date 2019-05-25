/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WITEXTENTRYBASE_H__
#define __WITEXTENTRYBASE_H__
#include "wgui/wibase.h"
#include "wgui/wihandle.h"

class WIText;
class WIRect;
class DLLWGUI WITextEntryBase
	: public WIBase
{
public:
	enum class StateFlags : uint32_t
	{
		None = 0u,
		Numeric = 1u,
		MultiLine = Numeric<<1u,
		Editable = MultiLine<<1u,
		Selectable = Editable<<1u,
		HideInput = Selectable<<1u
	};

	WITextEntryBase();
	virtual ~WITextEntryBase() override;
	virtual void Initialize() override;
	virtual void Think() override;
	virtual void RequestFocus() override;
	virtual void OnFocusGained() override;
	virtual void OnFocusKilled() override;
	virtual void SizeToContents() override;
	void SetInputHidden(bool b);
	std::string GetText();
	WIText *GetTextElement();
	void SetText(std::string text);
	void InsertText(std::string instext,int pos);
	void InsertText(std::string text);
	virtual util::EventReply MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods) override;
	virtual util::EventReply KeyboardCallback(GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods) override;
	virtual util::EventReply CharCallback(unsigned int c,GLFW::Modifier mods=GLFW::Modifier::None) override;
	virtual util::EventReply OnDoubleClick() override;
	virtual void SetSize(int x,int y) override;
	int GetCaretPos() const;
	void SetCaretPos(int pos);
	WIRect *GetCaretElement();

	virtual void SetColor(float r,float g,float b,float a=1.f) override;
	using WIBase::SetColor;

	bool IsNumeric() const;
	void SetNumeric(bool bNumeric);
	bool IsMultiLine() const;
	void SetMultiLine(bool bMultiLine);
	bool IsEditable() const;
	void SetEditable(bool bEditable);
	bool IsSelectable() const;
	void SetSelectable(bool bSelectable);

	void GetSelectionBounds(int *start,int *end);
	void SetSelectionBounds(int start,int end);
	void ClearSelection();
	void RemoveSelection();
	bool RemoveSelectedText();
	void OnEnter();
	void SetMaxLength(int length);
	int GetMaxLength();
protected:
	virtual void OnTextChanged(const std::string &oldText,const std::string &text);
	std::vector<WIHandle> m_selection;
	WIHandle m_hText;
	WIHandle m_hCaret;
	std::string m_text;
	int m_maxLength;
	int m_posCaret;
	StateFlags m_stateFlags = static_cast<StateFlags>(umath::to_integral(StateFlags::Editable) | umath::to_integral(StateFlags::Selectable));
	double m_tBlink;
	int m_selectStart;
	int m_selectEnd;
	bool m_bWasDoubleClick = false;
	int GetCharPos(int x,int y);
	int GetCharPos();
	int GetLineFromPos(int pos);
	void SetSelectionStart(int pos);
	void SetSelectionEnd(int pos);
	void UpdateSelection();
	int GetLineInfo(int pos,const std::string_view **line,int *lpos);
	void UpdateTextPosition();
	void UpdateText(const std::string &text);
};
REGISTER_BASIC_BITWISE_OPERATORS(WITextEntryBase::StateFlags)

#endif
