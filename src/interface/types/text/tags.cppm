// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:text_tags;

import :handle;
import :types.base;
export import pragma.string.formatted_text;

export namespace pragma::gui {
	struct DLLWGUI WITextTagArgument {
		enum class Type : uint32_t { String = 0u, Function, Color, Vector4 };
		Type type;
		std::shared_ptr<void> value;
	};

	namespace types {
		class WIText;
	}
	class DLLWGUI WITextDecorator {
	  public:
		WITextDecorator(types::WIText &text);
		virtual ~WITextDecorator();
		virtual void Clear();
		virtual bool IsValid() const;

		void SetDirty(bool dirty = true);
		bool IsDirty() const;

		virtual bool IsTag() const;

		virtual void Initialize();
		virtual void Apply();
		virtual string::AnchorPoint *GetStartAnchorPoint() = 0;
		virtual string::AnchorPoint *GetEndAnchorPoint() = 0;
		const string::AnchorPoint *GetStartAnchorPoint() const;
		const string::AnchorPoint *GetEndAnchorPoint() const;

		virtual string::TextOffset GetStartTextCharOffset() const;
		virtual string::TextOffset GetEndTextCharOffset() const;

		void GetTagRange(string::LineIndex &startLine, string::LineIndex &endLine, string::TextOffset &startOffset, string::TextOffset &endOffset) const;
		int32_t GetTagRange(const string::FormattedTextLine &line, string::CharOffset &startOffset, string::CharOffset &endOffset) const;
		int32_t GetTagRange(const string::FormattedTextLine &line, string::CharOffset minOffsetInLine, string::CharOffset maxOffsetInLine, string::CharOffset &outTagStartOffsetInLine, string::CharOffset &outTagEndOffsetInLine) const;
	  protected:
		void CreateOverlayElements();
		void CreateOverlayElement(string::LineIndex lineIndex, string::TextOffset startOffset, string::TextOffset endOffset, std::vector<WIHandle> &cachedOverlays);
		virtual void CalcBounds(Vector2i &inOutPos, Vector2i &inOutSize);
		virtual void InitializeOverlay(types::WIBase &overlay);

		std::vector<WIHandle> m_overlays = {};
		bool m_bDirty = false;
		types::WIText &m_text;
	};

	class DLLWGUI WITextTag : public WITextDecorator {
	  public:
		WITextTag(types::WIText &text, string::TextTag &tag);
		virtual bool IsTag() const override;
		virtual bool IsValid() const override;
		virtual string::AnchorPoint *GetStartAnchorPoint() override;
		virtual string::AnchorPoint *GetEndAnchorPoint() override;
		virtual string::TextOffset GetStartTextCharOffset() const override;
		virtual string::TextOffset GetEndTextCharOffset() const override;

		const string::TextTag &GetTag() const;
		string::TextTag &GetTag();
		const std::vector<WITextTagArgument> &GetArguments() const;
		std::vector<WITextTagArgument> &GetArguments();
	  protected:
		std::vector<WITextTagArgument> m_args = {};
		string::TextTag &m_tag;
	};

	class DLLWGUI WITextTagUnderline : public WITextTag {
	  public:
		using WITextTag::WITextTag;
		virtual void Apply() override;
	  protected:
		virtual void InitializeOverlay(types::WIBase &overlay) override;
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
		virtual void InitializeOverlay(types::WIBase &overlay) override;
	};

	class DLLWGUI WITextTagLink : public WITextTagUnderline {
	  public:
		static void set_link_handler(const std::function<void(const std::string &)> &linkHandler);

		using WITextTagUnderline::WITextTagUnderline;
		virtual void Initialize() override;
		virtual void Apply() override;
	  protected:
		virtual void InitializeOverlay(types::WIBase &overlay) override;
		CallbackHandle m_cbFunction = {};
		std::vector<std::string> m_strArgs = {};
	  private:
		static std::function<void(const std::string &)> s_linkHandler;
	};

	class DLLWGUI WITextTagSelection : public WITextDecorator {
	  public:
		WITextTagSelection(types::WIText &text, string::TextOffset startOffset, string::TextOffset endOffset);
		virtual void Apply() override;
		virtual bool IsValid() const override;
		virtual string::AnchorPoint *GetStartAnchorPoint() override;
		virtual string::AnchorPoint *GetEndAnchorPoint() override;

		void SetStartOffset(string::TextOffset offset);
		void SetEndOffset(string::TextOffset offset);
	  protected:
		virtual void InitializeOverlay(types::WIBase &overlay) override;
		virtual void CalcBounds(Vector2i &inOutPos, Vector2i &inOutSize) override;
		std::optional<std::pair<string::LineIndex, string::CharOffset>> GetAbsOffset(string::TextOffset offset) const;
		util::TSharedHandle<string::AnchorPoint> m_startAnchorPoint = nullptr;
		util::TSharedHandle<string::AnchorPoint> m_endAnchorPoint = nullptr;
	};
};
