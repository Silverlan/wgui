// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"
#include <cinttypes>
#include <string>

export module pragma.gui:types.content_wrapper;

import :types.base;

export {
	struct DLLWGUI WIPadding {
		int32_t left = 0;
		int32_t right = 0;
		int32_t top = 0;
		int32_t bottom = 0;
	};

	class DLLWGUI WIContentWrapper : public WIBase {
	public:
		WIContentWrapper();

		void SetPadding(const WIPadding &border);
		void ClearPadding();
		const WIPadding &GetPadding() const;
		void SetPadding(int32_t left, int32_t right, int32_t top, int32_t bottom);
		void SetPadding(int32_t border);
		void SetPaddingLeft(int32_t border);
		void SetPaddingRight(int32_t border);
		void SetPaddingTop(int32_t border);
		void SetPaddingBottom(int32_t border);
		void SetPaddingLeftRight(int32_t border);
		void SetPaddingTopBottom(int32_t border);

		virtual void SetSize(int x, int y) override;
	private:
		void UpdateChildElement();
		WIPadding m_padding;
		CallbackHandle m_onChildSizeChanged;
		bool m_skipCallback = false;
		bool m_skipChildResize = false;
	};
};
