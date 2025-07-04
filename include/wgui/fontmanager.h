// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __FONTLOADER_H__
#define __FONTLOADER_H__

#include <ft2build.h>
#include FT_FREETYPE_H
#include <unordered_map>
#include <string>
#include <vector>
#include <image/prosper_texture.hpp>
#include "wguidefinitions.h"

import pragma.string.unicode;

namespace prosper {
	class IDescriptorSet;
};

class FontInfo;
class FontManager;
class DLLWGUI GlyphInfo {
  private:
	bool m_bInitialized = false;
	int32_t m_left = 0;
	int32_t m_top = 0;
	int32_t m_width = 0;
	int32_t m_height = 0;
	int32_t m_advanceX = 0;
	int32_t m_advanceY = 0;
	std::array<int32_t, 4> m_bbox;
  public:
	GlyphInfo();
	void Initialize(FT_GlyphSlot glyph);
	void GetAdvance(int32_t &advanceX, int32_t &advanceY) const;
	void GetDimensions(int32_t &left, int32_t &top, int32_t &width, int32_t &height) const;
	void GetBounds(int32_t *xMin, int32_t *yMin, int32_t *xMax, int32_t *yMax) const;
	int32_t GetTop() const;
	int32_t GetLeft() const;
	int32_t GetWidth() const;
	int32_t GetHeight() const;
};

namespace prosper {
	class DescriptorSetGroup;
};
struct DLLWGUI FontSettings {
	FontSettings();
	~FontSettings();
	uint32_t fontSize;
};
struct DLLWGUI FontFace {
  private:
	FT_Face m_ftFace = nullptr;
  public:
	FontFace() = default;
	~FontFace();
	const FT_Face &GetFtFace() const;
};
class DynamicFontMap;
class DLLWGUI FontInfo : public std::enable_shared_from_this<FontInfo> {
  public:
	~FontInfo();
	void Clear();
	bool Initialize(const std::string &cpath, const std::string &name, const FontSettings &fontSettings);
	const std::string &GetName() const { return m_name; }
	const FT_Face GetFace() const;
	const GlyphInfo *GetGlyphInfo(int32_t c) const;
	const GlyphInfo *InitializeGlyph(int32_t c) const;
	uint32_t CharToGlyphMapIndex(int32_t c) const;
	const std::vector<std::shared_ptr<GlyphInfo>> &GetGlyphs() const;
	uint32_t GetSize() const;
	uint32_t GetMaxGlyphSize() const;
	uint32_t GetMaxGlyphHeight() const;
	uint32_t GetMaxGlyphBitmapWidth() const;
	uint32_t GetMaxGlyphBitmapHeight() const;
	int32_t GetMaxGlyphTop() const;
	uint32_t GetGlyphCountPerRow() const;
	std::shared_ptr<prosper::Texture> GetGlyphMap() const;
	prosper::IDescriptorSet *GetGlyphMapDescriptorSet() const;
	std::shared_ptr<prosper::IBuffer> GetGlyphBoundsBuffer() const;
	prosper::IDescriptorSet *GetGlyphBoundsDescriptorSet() const;
	void AddFallbackFont(FontInfo &font);
	const std::vector<std::shared_ptr<FontInfo>> &GetFallbackFonts() const { return m_fallbackFonts; }
  protected:
	FontInfo();
	friend FontManager;
  private:
	std::vector<std::shared_ptr<FontInfo>> m_fallbackFonts;
	std::unordered_map<uint32_t, uint32_t> m_glyphIndexMap;
	bool m_bInitialized = false;
	std::string m_name;
	uint32_t m_size = 0;
	uint32_t m_maxGlyphHeight = 0;
	uint32_t m_maxGlyphSize = 0;
	std::unique_ptr<DynamicFontMap> m_dynamicFontMap;
};

class DLLWGUI FontManager {
  public:
	static const auto TAB_WIDTH_SPACE_COUNT = 4u;
	static bool Initialize();
	static std::shared_ptr<const FontInfo> GetDefaultFont();
	static const std::unordered_map<std::string, std::shared_ptr<FontInfo>> &GetFonts();
	static void SetDefaultFont(const FontInfo &font);
	static const FT_Library GetFontLibrary();
	static std::shared_ptr<const FontInfo> LoadFont(const std::string &cidentifier, const std::string &cpath, const FontSettings &fontSettings, bool bForceReload = false);
	static std::shared_ptr<const FontInfo> GetFont(const std::string &cfontName);
	static void Close();
	static void UpdateDirtyFonts();
	static void SetFontsDirty();
	static void InitializeFontGlyphs(const pragma::string::Utf8StringArg &text, const FontInfo &font);
	static void AddFallbackFont(const std::string &fallbackFont);
	static const std::vector<std::string> &GetFallbackFonts();
	// Char offset (relative to a line) is required to calculate the correct tab size
	static uint32_t GetTextSize(const pragma::string::Utf8StringArg &text, uint32_t charOffset, const FontInfo *font, int32_t *width, int32_t *height = nullptr);
	static uint32_t GetTextSize(const pragma::string::Utf8StringArg &text, uint32_t charOffset, const std::string &font, int32_t *width, int32_t *height = nullptr);
	static uint32_t GetTextSize(int32_t c, uint32_t charOffset, const FontInfo *font, int32_t *width, int32_t *height = nullptr);
	static uint32_t GetTextSize(int32_t c, uint32_t charOffset, const std::string &font, int32_t *width, int32_t *height = nullptr);
  private:
	struct Library {
	  private:
		FT_Library m_ftLibrary = nullptr;
	  public:
		Library() = default;
		~Library();
		const FT_Library &GetFtLibrary() const;
	};
	static Library m_lib;
	static std::unordered_map<std::string, std::shared_ptr<FontInfo>> m_fonts;
	static std::shared_ptr<const FontInfo> m_fontDefault;
	static std::vector<std::string> m_fallbackFonts;
};

#endif
