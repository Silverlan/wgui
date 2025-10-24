// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"
#include <array>

export module pragma.gui:types.rect;

export import :handle;
export import :types.base;
export import :types.rounded_base;
export import :types.shape;

export {
	class DLLWGUI WIRect : public WIShape {
	public:
		WIRect();
		~WIRect() override;
	};

	class DLLWGUI WIOutlinedRect : public wgui::WIBase {
	private:
		std::array<wgui::WIHandle, 4> m_lines;
		unsigned int m_lineWidth;
		void UpdateLines();
	public:
		WIOutlinedRect();
		virtual void Initialize() override;
		unsigned int GetOutlineWidth();
		void SetOutlineWidth(unsigned int width);
		virtual void SetSize(int x, int y) override;
		using WIBase::SetColor;
	};

	class DLLWGUI WIRoundedRect : public WIShape, public WIRoundedBase {
	public:
		WIRoundedRect();
		virtual ~WIRoundedRect() override = default;
		virtual void DoUpdate() override;
		virtual void Initialize() override;
	};

	class DLLWGUI WITexturedRect : public WITexturedShape {
	public:
		WITexturedRect();
		virtual ~WITexturedRect() override = default;
	};

	class DLLWGUI WIRoundedTexturedRect : public WITexturedShape, public WIRoundedBase {
	public:
		WIRoundedTexturedRect();
		virtual ~WIRoundedTexturedRect() override = default;
		virtual void DoUpdate() override;
		virtual void Initialize() override;
	};
};
