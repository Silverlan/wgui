/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIDROPDOWNMENU_H__
#define __WIDROPDOWNMENU_H__

#include "wgui/wibase.h"
#include "witextentry.h"
#include "wgui/wihandle.h"

class DLLWGUI WIDropDownMenu;
class DLLWGUI WIDropDownMenuOption
	: public WIBase
{
public:
	friend WIDropDownMenu;
protected:
	int m_index;

	WIHandle m_hBackground;
	WIHandle m_hText;
	WIHandle m_dropDownMenu;
	std::string m_value;

	void UpdateTextPos();
	void SetDropDownMenu(WIDropDownMenu *menu);
public:
	WIDropDownMenuOption();
	virtual ~WIDropDownMenuOption() override;
	virtual void Initialize() override;
	void SetValue(const std::string &val);
	const std::string &GetValue();
	void SetIndex(int idx);
	int GetIndex();
	void SetText(const std::string_view &text);
	std::string_view GetText() const;
	virtual void SetSize(int x,int y) override;
	virtual void OnCursorEntered() override;
	virtual void OnCursorExited() override;
	WIDropDownMenu *GetDropDownMenu();
};

class DLLWGUI WIDropDownMenu
	: public WITextEntry
{
protected:
	unsigned int m_numListItems;
	unsigned int m_listOffset;
	int m_selected;

	WIHandle m_hOutline;
	WIHandle m_hArrow;
	WIHandle m_hList;
	WIHandle m_hScrollBar;
	std::vector<WIHandle> m_options;

	void UpdateTextPos();
	void UpdateText();
	virtual void OnTextChanged(const std::string &text,bool changedByUser) override;
public:
	WIDropDownMenu();
	virtual ~WIDropDownMenu() override;
	virtual void Initialize() override;
	void SelectOption(unsigned int idx);
	void SelectOption(const std::string &value);
	void SelectOptionByText(const std::string &name);
	void OnOptionSelected(WIDropDownMenuOption *option);
	std::string_view GetText() const;
	std::string GetValue();
	int32_t GetSelectedOption() const;
	std::string_view GetOptionText(uint32_t idx);
	std::string GetOptionValue(uint32_t idx);
	void SetOptionText(uint32_t idx,const std::string &text);
	void SetOptionValue(uint32_t idx,const std::string &val);
	void SetText(const std::string_view &text);
	unsigned int GetOptionCount();
	void AddOption(const std::string &option,const std::string &value);
	void AddOption(const std::string &option);
	void ClearOptions();
	void SetOptions(const std::vector<std::string> &options);
	void SetOptions(const std::unordered_map<std::string,std::string> &options);
	void SetOptionOffset(unsigned int offset);
	void OpenMenu();
	void CloseMenu();
	bool IsMenuOpen();
	void ToggleMenu();
	virtual util::EventReply MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods) override;
	virtual util::EventReply ScrollCallback(Vector2 offset) override;
	virtual void SetSize(int x,int y) override;
};

#endif
