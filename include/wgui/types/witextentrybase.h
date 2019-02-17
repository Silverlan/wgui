/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WITEXTENTRYBASE_H__
#define __WITEXTENTRYBASE_H__
#include "wgui/wibase.h"
#include "wgui/wihandle.h"

class WIText;
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLWGUI WITextEntryBase
	: public WIBase
{
protected:
	virtual void OnTextChanged(const std::string &oldText,const std::string &text);
	std::vector<WIHandle> m_selection;
	WIHandle m_hText;
	WIHandle m_hCaret;
	std::string m_text;
	int m_maxLength;
	int m_posCaret;
	bool m_bNumeric;
	bool m_bMultiLine;
	bool m_bEditable;
	double m_tBlink;
	int m_selectStart;
	int m_selectEnd;
	bool m_bHiddenInput;
	int GetCharPos(int x,int y);
	int GetCharPos();
	int GetLineFromPos(int pos);
	void SetSelectionStart(int pos);
	void SetSelectionEnd(int pos);
	void UpdateSelection();
	int GetLineInfo(int pos,std::string **line,int *lpos);
	void UpdateTextPosition();
	void UpdateText(const std::string &text);
public:
	WITextEntryBase();
	virtual ~WITextEntryBase() override;
	virtual void Initialize() override;
	virtual void Think() override;
	virtual void RequestFocus() override;
	virtual void OnFocusGained() override;
	virtual void OnFocusKilled() override;
	void SetInputHidden(bool b);
	std::string GetText();
	WIText *GetTextElement();
	void SetText(std::string text);
	void InsertText(std::string instext,int pos);
	void InsertText(std::string text);
	virtual void MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods) override;
	virtual void KeyboardCallback(GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods) override;
	virtual void CharCallback(unsigned int c,GLFW::Modifier mods=GLFW::Modifier::None) override;
	virtual void SetSize(int x,int y) override;
	int GetCaretPos();
	void SetCaretPos(int pos);
	bool IsNumeric();
	void SetNumeric(bool bNumeric);
	bool IsMultiLine();
	void SetMultiLine(bool bMultiLine);
	bool IsEditable();
	void SetEditable(bool bEditable);
	void GetSelectionBounds(int *start,int *end);
	void SetSelectionBounds(int start,int end);
	void ClearSelection();
	void RemoveSelection();
	void OnEnter();
	void SetMaxLength(int length);
	int GetMaxLength();
};
#pragma warning(pop)

#endif
