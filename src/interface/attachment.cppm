// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.gui:attachment;

import :handle;
export import pragma.util;

export class DLLWGUI WIAttachment {
  public:
	WIAttachment(WIBase &owner, const Vector2 &pos = {});
	void SetRelativePosition(const Vector2 &pos);
	const Vector2 &GetRelativePosition() const;

	void UpdateAbsolutePosition();
	const util::PVector2iProperty &GetAbsPosProperty() const;
  private:
	WIHandle m_hOwner = {};
	Vector2 m_relativePosition = {};
	util::PVector2iProperty m_absPosProperty = nullptr;
};
