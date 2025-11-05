// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.gui:types.rounded_base;

export class WIShape;
export class DLLWGUI WIRoundedBase {
  public:
	friend WIShape;
  protected:
	WIRoundedBase();
	char m_roundness;
	float m_cornerSize;
	bool m_bRoundUpperLeft;
	bool m_bRoundUpperRight;
	bool m_bRoundLowerLeft;
	bool m_bRoundLowerRight;
	virtual void DoUpdate();
	virtual void Initialize();
  public:
	virtual ~WIRoundedBase() = default;
	char GetRoundness();
	void SetRoundness(char roundness);
	void SetCornerSize(float size);
	float GetCornerSize();
	void SetRoundTopRight(bool b);
	void SetRoundTopLeft(bool b);
	void SetRoundBottomLeft(bool b);
	void SetRoundBottomRight(bool b);
	bool IsTopRightRound();
	bool IsTopLeftRound();
	bool IsBottomLeftRound();
	bool IsBottomRightRound();
};
