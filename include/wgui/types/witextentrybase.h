/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WITEXTENTRYBASE_H__
#define __WITEXTENTRYBASE_H__
#include "wgui/wibase.h"
#include "wgui/wihandle.h"
#include <util_formatted_text_types.hpp>
#include <string_view>

class WIText;
class WIRect;
class WITextDecorator;
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
	std::string_view GetText() const;
	WIText *GetTextElement();
	void SetText(std::string_view text);
	void InsertText(const std::string_view &instext,int pos);
	void InsertText(const std::string_view &text);
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
	bool RemoveSelectedText();
	void OnEnter();
	void SetMaxLength(int length);
	int GetMaxLength();
protected:
	virtual void OnTextContentsChanged();
	void UpdateHiddenText();
	std::shared_ptr<WITextDecorator> m_selectionDecorator = nullptr;
	WIHandle m_hText;
	WIHandle m_hCaret;
	std::string m_realText; // Only used if the 'HideInput' flag is set
	int m_maxLength;
	int m_posCaret;
	StateFlags m_stateFlags = static_cast<StateFlags>(umath::to_integral(StateFlags::Editable) | umath::to_integral(StateFlags::Selectable));
	double m_tBlink;
	int m_selectStart;
	int m_selectEnd;
	bool m_bWasDoubleClick = false;
	int GetCharPos(int x,int y) const;
	int GetCharPos() const;
	int GetLineFromPos(int pos);
	void SetSelectionStart(int pos);
	void SetSelectionEnd(int pos);
	void UpdateSelection();
	std::pair<util::text::LineIndex,util::text::LineIndex> GetLineInfo(int pos,std::string_view &outLine,int *lpos) const;
	void UpdateTextPosition();
	virtual void OnTextChanged(const std::string &text,bool changedByUser);
	void OnTextChanged(bool changedByUser);
};
REGISTER_BASIC_BITWISE_OPERATORS(WITextEntryBase::StateFlags)

#endif
