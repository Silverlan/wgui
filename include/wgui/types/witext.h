/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WITEXT_H__
#define __WITEXT_H__

#define WITEXT_VALIDATION_ENABLED

#include "wgui/wibase.h"
#include "wgui/fontmanager.h"
#include "wgui/wihandle.h"
#include <image/prosper_render_target.hpp>
#include <sharedutils/property/util_property.hpp>
#include <string_view>
#include <queue>
#ifdef WITEXT_VALIDATION_ENABLED
#include <sstream>
#endif

namespace prosper
{
	class Shader;
	class BlurSet;
	class UniformResizableBuffer;
};

class WGUIShaderText;
class DLLWGUI WIText
	: public WIBase
{
public:
	struct DLLWGUI RangeInfo
	{
		int32_t offset = 0;
		int32_t length = 0;
		int32_t GetEndOffset() const {return offset +length -1;} // Note: May be negative if length is 0
		void SetEndOffset(int32_t endOffset) {length = endOffset -offset +1;}
	};
private:
	struct SubBufferInfo
	{
		std::shared_ptr<prosper::Buffer> buffer;
		std::shared_ptr<prosper::Buffer> colorBuffer;
		size_t subStringHash;
		RangeInfo range = {}; // start offset and length into RAW text (including tags)
		uint32_t numChars = 0u;
		uint32_t width = 0u;
		uint32_t height = 0u;
		float sx = 0.f;
		float sy = 0.f;
	};
public:
	enum class AutoBreak : int
	{
		NONE,
		ANY,
		WHITESPACE
	};
	enum class Flags : uint8_t
	{
		None = 0u,
		RenderTextScheduled = 1u,
		FullUpdateScheduled = RenderTextScheduled<<1u,

		Cache = FullUpdateScheduled<<1u,
		TextDirty = Cache<<1u,
		ApplySubTextTags = TextDirty<<1u
	};
	enum class TagType : uint32_t
	{
		None = 0u,
		Color,
		Link,
		Underline,
		Tooltip,
		Template
	};
	struct DLLWGUI LineInfo
	{
		RangeInfo textRange = {}; // start offset and size into RAW text (including tags, but excluding new-line characters)
		RangeInfo visTextRange = {};
		std::string_view line;
		int32_t widthInPixels = 0u;
		bool newLine = false; // Indicates whether this line is terminated by a new-line character
		bool bufferUpdateRequired = true;
		std::vector<SubBufferInfo> buffers = {};
		uint32_t GetNumberOfVisibleCharacters() const {return line.size();}
		uint32_t GetNumberOfCharacters() const {return line.size() +(newLine ? 1 : 0);}
		bool IsEmpty() const {return visTextRange.length == 0;}
	};
	struct DLLWGUI TagArgument
	{
		enum class Type : uint32_t
		{
			String = 0u,
			Function,
			Color,
			Vector4
		};
		Type type;
		std::shared_ptr<void> value;
	};
	struct DLLWGUI TagInfo
	{
		~TagInfo()
		{
			ClearOverlays();
		}
		void ClearOverlays()
		{
			for(auto &hOverlay : overlays)
			{
				if(hOverlay.IsValid())
					hOverlay->Remove();
			}
		}
		TagType tagType = TagType::None;
		std::vector<TagArgument> args = {};
		RangeInfo rangeOpenTag = {}; // start offset and length into RAW text (including tags themselves)
		RangeInfo rangeClosingTag = {};
		std::vector<WIHandle> overlays = {};
		std::string label = {};
		uint32_t GetLength() const
		{
			return rangeClosingTag.GetEndOffset() -rangeOpenTag.offset +1;
		}
		std::string GetOpenTag(const std::string &baseText) const
		{
			return baseText.substr(rangeOpenTag.offset,rangeOpenTag.length);
		}
		std::string GetClosingTag(const std::string &baseText) const
		{
			return baseText.substr(rangeClosingTag.offset,rangeClosingTag.length);
		}
		std::string GetTagString(const std::string &baseText) const
		{
			return baseText.substr(rangeOpenTag.offset,GetLength());
		}
	};
	static void set_link_handler(const std::function<void(const std::string&)> &linkHandler);

	WIText();
	virtual ~WIText() override;
	WIText(const WIText&)=delete;
	void operator=(const WIText&)=delete;
	virtual void Initialize() override;
	const FontInfo *GetFont() const;
	uint32_t GetLineCount() const;
	// Number of lines including lines broken
	// up into multiple segments
	uint32_t GetTotalLineCount() const;
	const std::vector<LineInfo> &GetLines() const;
	std::string_view *GetLine(int line);
	int GetLineHeight() const;
	int GetBreakHeight();
	void SetBreakHeight(int breakHeight);
	const util::PStringProperty &GetTextProperty() const;
	const std::string &GetText() const;
	const std::string &GetVisibleText() const;
	const std::vector<uint32_t> &GetRawTextIndicesToVisibleIndices() const;
	const std::vector<uint32_t> &GetVisibleTextIndicesToRawIndices() const;
	void SetText(const std::string &text);
	void SetFont(const std::string &font);
	void SetFont(const FontInfo *font);
	void SetAutoBreakMode(AutoBreak b);
	AutoBreak GetAutoBreakMode() const;
	using WIBase::SetSize;
	virtual void SetSize(int x,int y) override;
	int GetTextHeight();
	std::shared_ptr<prosper::Texture> GetTexture() const;
	virtual std::string GetDebugInfo() const override;
	std::pair<Vector2i,Vector2i> GetCharacterPixelBounds(uint32_t charOffset) const;
	void SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const std::string &arg);
	void SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const Vector4 &arg);
	void SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const CallbackHandle &arg);
	void SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const Color &arg);
	void SetTagsEnabled(bool bEnabled);
	bool AreTagsEnabled() const;

	void SetTabSpaceCount(uint32_t numberOfSpaces);
	uint32_t GetTabSpaceCount() const;

	void SetAutoSizeToText(bool bAutoSize);
	bool ShouldAutoSizeToText() const;
	
	void RemoveLine(uint32_t lineIdx);
	void PopFrontLine();
	void PopBackLine();
	void InsertLine(uint32_t lineIdx,const std::string_view &line);
	void AppendLine(const std::string_view &line);
	void PrependLine(const std::string_view &line);
	void SetDirty();
	bool IsDirty() const;

	// Shadow
	void EnableShadow(bool b);
	bool IsShadowEnabled();
	void SetShadowOffset(Vector2i offset);
	virtual void SetShadowOffset(int x,int y);
	Vector2i *GetShadowOffset();
	void SetShadowColor(Vector4 col);
	void SetShadowColor(const Color &col);
	virtual void SetShadowAlpha(float alpha);
	float GetShadowAlpha();
	Vector4 *GetShadowColor();
	virtual void SetShadowColor(float r,float g,float b,float a=1.f);
	virtual void SetShadowBlurSize(float size);
	float GetShadowBlurSize();
	//

	virtual void SelectShader();
	virtual void Render(int w,int h,const Mat4 &mat,const Vector2i &origin,const Mat4 &matParent) override;
	virtual void Think() override;
	virtual void Update() override;
	virtual void SizeToContents() override;
	virtual Mat4 GetTransformedMatrix(const Vector2i &origin,int w,int h,Mat4 mat) override;

	void SetCacheEnabled(bool bEnabled);
	bool IsCacheEnabled() const;

	static void InitializeTextBuffer(prosper::Context &context);
	static void ClearTextBuffer();

#ifdef WITEXT_VALIDATION_ENABLED
	bool Validate(std::stringstream &outMsg) const;
	bool UnitTest();
#endif
private:
	struct WITextShadowInfo
	{
		WITextShadowInfo()
			: enabled(false),blurSize(0.f)
		{}
		Vector2i offset;
		bool enabled;
		Vector4 color;
		float blurSize;
	};
private:
	static const auto MAX_CHARS_PER_BUFFER = 32u;
	static std::shared_ptr<prosper::UniformResizableBuffer> s_textBuffer;
	static std::shared_ptr<prosper::UniformResizableBuffer> s_colorBuffer;
	static std::function<void(const std::string&)> s_linkHandler;
	util::WeakHandle<prosper::Shader> m_shader = {};
	util::PStringProperty m_text;
	std::string m_visibleText;
	std::vector<uint32_t> m_rawTextIndexToVisibleTextIndex = {}; // Visible text = excluding tags
	std::vector<uint32_t> m_visibleTextIndexToRawTextIndex = {};
	std::vector<LineInfo> m_lines = {}; // Excludes tags
	uint32_t m_lineCount = 0u; // Number of actual lines
	std::vector<TagInfo> m_subTextTags = {};
	std::unordered_map<std::string,std::unordered_map<uint32_t,TagArgument>> m_tagArgumentOverrides = {};
	WIHandle m_baseText;
	std::shared_ptr<const FontInfo> m_font;
	int m_breakHeight;
	AutoBreak m_autoBreak;
	bool m_bTagsEnabled = false;
	bool m_bAutoSizeToText = false;
	uint32_t m_tabSpaceCount = 4u;

	// Shadow
	WIHandle m_baseTextShadow;
	WITextShadowInfo m_shadow;
	//

	// Render Text
	std::shared_ptr<prosper::RenderTarget> m_renderTarget = nullptr;
	std::shared_ptr<prosper::RenderTarget> m_shadowRenderTarget = nullptr;
	std::shared_ptr<prosper::BlurSet> m_shadowBlurSet = nullptr;
	//

	Flags m_flags = Flags::FullUpdateScheduled;
	unsigned int m_wTexture;
	unsigned int m_hTexture;

	// Takes the index for an actual line (separated by a new-line character) and returns
	// the associated index for the m_lines array
	std::optional<uint32_t> GetInternalLineIndex(uint32_t lineIndex) const;
	std::pair<int32_t,int32_t> GetTagRange(const TagInfo &tag) const;
	bool IsTextUpdateRequired(const std::string &newText,bool compareText=false) const;
	bool ParseLines(const std::string_view &text);
	uint32_t ParseLine(const std::string_view &text);
	void BreakLineByWidth(uint32_t lineIndex);
	
	void UpdateLineStringViewObjects(uint32_t offset=0u);
	void PerformTextPostProcessing();
	void AutoSizeToText();
	int32_t FindLine(int32_t charOffset);
	uint32_t GetStringLength(const std::string_view &sv) const;
	std::optional<uint32_t> ParseCodeTag(const std::string_view &sv,uint32_t offset=0u,TagInfo *outTagInfo=nullptr,bool *outIsClosingTag=nullptr) const;
	void SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const TagArgument &arg);
	void ApplySubTextTags();
	void ApplySubTextTag(const std::string &label);
	void ApplySubTextTag(TagInfo &tag);
	void ApplySubTextColor(TagInfo &tag);
	void ApplySubTextLink(TagInfo &tag);
	void ApplySubTextUnderline(TagInfo &tag);
	const std::vector<WIHandle> *CreateSubTextOverlayElement(TagInfo &tag);
	void InitializeTextBuffers();
	void InitializeTextBuffers(uint32_t lineIndex);
	void UpdateRenderTexture();
	void GetTextSize(int *w,int *h,const std::string_view *inText=nullptr);
	void RenderText();
	void RenderText(Mat4 &mat);
	void InitializeShadow(bool bReload=false);
	void DestroyShadow();
	void InitializeBlur(bool bReload=false);
	void DestroyBlur();
	void ScheduleRenderUpdate(bool bFull=false);
};
REGISTER_BASIC_BITWISE_OPERATORS(WIText::Flags)

#endif
