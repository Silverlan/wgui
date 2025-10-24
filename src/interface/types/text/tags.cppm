// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"
#include <functional>
#include <optional>
#include <string>

export module pragma.gui:text_tags;

import :handle;
import :types.base;
export import pragma.string.formatted_text;

export {
	struct DLLWGUI WITextTagArgument {
		enum class Type : uint32_t { String = 0u, Function, Color, Vector4 };
		Type type;
		std::shared_ptr<void> value;
	};

	class WIText;
	class DLLWGUI WITextDecorator {
	public:
		WITextDecorator(WIText &text);
		virtual ~WITextDecorator();
		virtual void Clear();
		virtual bool IsValid() const;

		void SetDirty(bool dirty = true);
		bool IsDirty() const;

		virtual bool IsTag() const;

		virtual void Initialize();
		virtual void Apply();
		virtual util::text::AnchorPoint *GetStartAnchorPoint() = 0;
		virtual util::text::AnchorPoint *GetEndAnchorPoint() = 0;
		const util::text::AnchorPoint *GetStartAnchorPoint() const;
		const util::text::AnchorPoint *GetEndAnchorPoint() const;

		virtual util::text::TextOffset GetStartTextCharOffset() const;
		virtual util::text::TextOffset GetEndTextCharOffset() const;

		void GetTagRange(util::text::LineIndex &startLine, util::text::LineIndex &endLine, util::text::TextOffset &startOffset, util::text::TextOffset &endOffset) const;
		int32_t GetTagRange(const util::text::FormattedTextLine &line, util::text::CharOffset &startOffset, util::text::CharOffset &endOffset) const;
		int32_t GetTagRange(const util::text::FormattedTextLine &line, util::text::CharOffset minOffsetInLine, util::text::CharOffset maxOffsetInLine, util::text::CharOffset &outTagStartOffsetInLine, util::text::CharOffset &outTagEndOffsetInLine) const;
	protected:
		void CreateOverlayElements();
		void CreateOverlayElement(util::text::LineIndex lineIndex, util::text::TextOffset startOffset, util::text::TextOffset endOffset, std::vector<wgui::WIHandle> &cachedOverlays);
		virtual void CalcBounds(Vector2i &inOutPos, Vector2i &inOutSize);
		virtual void InitializeOverlay(wgui::WIBase &overlay);

		std::vector<wgui::WIHandle> m_overlays = {};
		bool m_bDirty = false;
		WIText &m_text;
	};

	class DLLWGUI WITextTag : public WITextDecorator {
	public:
		WITextTag(WIText &text, util::text::TextTag &tag);
		virtual bool IsTag() const override;
		virtual bool IsValid() const override;
		virtual util::text::AnchorPoint *GetStartAnchorPoint() override;
		virtual util::text::AnchorPoint *GetEndAnchorPoint() override;
		virtual util::text::TextOffset GetStartTextCharOffset() const override;
		virtual util::text::TextOffset GetEndTextCharOffset() const override;

		const util::text::TextTag &GetTag() const;
		util::text::TextTag &GetTag();
		const std::vector<WITextTagArgument> &GetArguments() const;
		std::vector<WITextTagArgument> &GetArguments();
	protected:
		std::vector<WITextTagArgument> m_args = {};
		util::text::TextTag &m_tag;
	};

	class DLLWGUI WITextTagUnderline : public WITextTag {
	public:
		using WITextTag::WITextTag;
		virtual void Apply() override;
	protected:
		virtual void InitializeOverlay(wgui::WIBase &overlay) override;
		virtual void CalcBounds(Vector2i &inOutPos, Vector2i &inOutSize) override;
		Color m_underlineColor;
	};

	class DLLWGUI WITextTagColor : public WITextTag {
	public:
		static void ClearColorBuffer();

		using WITextTag::WITextTag;
		virtual void Apply() override;

		std::optional<Vector4> GetColor() const;
	private:
		static std::shared_ptr<prosper::IUniformResizableBuffer> s_colorBuffer;
	};

	class DLLWGUI WITextTagTooltip : public WITextTag {
	public:
		using WITextTag::WITextTag;
		virtual void Apply() override;
	protected:
		virtual void InitializeOverlay(wgui::WIBase &overlay) override;
	};

	class DLLWGUI WITextTagLink : public WITextTagUnderline {
	public:
		static void set_link_handler(const std::function<void(const std::string &)> &linkHandler);

		using WITextTagUnderline::WITextTagUnderline;
		virtual void Initialize() override;
		virtual void Apply() override;
	protected:
		virtual void InitializeOverlay(wgui::WIBase &overlay) override;
		CallbackHandle m_cbFunction = {};
		std::vector<std::string> m_strArgs = {};
	private:
		static std::function<void(const std::string &)> s_linkHandler;
	};

	class DLLWGUI WITextTagSelection : public WITextDecorator {
	public:
		WITextTagSelection(WIText &text, util::text::TextOffset startOffset, util::text::TextOffset endOffset);
		virtual void Apply() override;
		virtual bool IsValid() const override;
		virtual util::text::AnchorPoint *GetStartAnchorPoint() override;
		virtual util::text::AnchorPoint *GetEndAnchorPoint() override;

		void SetStartOffset(util::text::TextOffset offset);
		void SetEndOffset(util::text::TextOffset offset);
	protected:
		virtual void InitializeOverlay(wgui::WIBase &overlay) override;
		virtual void CalcBounds(Vector2i &inOutPos, Vector2i &inOutSize) override;
		std::optional<std::pair<util::text::LineIndex, util::text::CharOffset>> GetAbsOffset(util::text::TextOffset offset) const;
		util::TSharedHandle<util::text::AnchorPoint> m_startAnchorPoint = nullptr;
		util::TSharedHandle<util::text::AnchorPoint> m_endAnchorPoint = nullptr;
	};
};
