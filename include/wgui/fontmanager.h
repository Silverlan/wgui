/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __FONTLOADER_H__
#define __FONTLOADER_H__
#include <ft2build.h>
#include FT_FREETYPE_H
#include <unordered_map>
#include <string>
#include <vector>
#include <image/prosper_texture.hpp>
#include "wguidefinitions.h"

class FontInfo;
class FontManager;
class DLLWGUI GlyphInfo
{
private:
	bool m_bInitialized = false;
	int32_t m_left = 0;
	int32_t m_top = 0;
	int32_t m_width = 0;
	int32_t m_height = 0;
	int32_t m_advanceX = 0;
	int32_t m_advanceY = 0;
	std::array<int32_t,4> m_bbox;
	friend FontManager;
protected:
	GlyphInfo();
	friend FontInfo;
public:
	void Initialize(FT_GlyphSlot glyph);
	void GetAdvance(int32_t &advanceX,int32_t &advanceY) const;
	void GetDimensions(int32_t &left,int32_t &top,int32_t &width,int32_t &height) const;
	void GetBounds(int32_t *xMin,int32_t *yMin,int32_t *xMax,int32_t *yMax) const;
	int32_t GetTop() const;
	int32_t GetLeft() const;
	int32_t GetWidth() const;
	int32_t GetHeight() const;
};

namespace prosper {class DescriptorSetGroup;};
class DLLWGUI FontInfo
	: public std::enable_shared_from_this<FontInfo>
{
public:
	static uint32_t CharToGlyphMapIndex(char c);
	~FontInfo();
	void Clear();
	bool Initialize(const std::string &cpath,uint32_t size);
	const FT_Face GetFace() const;
	const GlyphInfo *GetGlyphInfo(char c) const;
	const std::vector<std::shared_ptr<GlyphInfo>> &GetGlyphs() const;
	uint32_t GetSize() const;
	uint32_t GetMaxGlyphSize() const;
	uint32_t GetMaxGlyphHeight() const;
	uint32_t GetMaxGlyphBitmapWidth() const;
	uint32_t GetMaxGlyphBitmapHeight() const;
	int32_t GetMaxGlyphTop() const;
	std::shared_ptr<prosper::Texture> GetGlyphMap() const;
	Anvil::DescriptorSet *GetGlyphMapDescriptorSet() const;
	std::shared_ptr<prosper::Buffer> GetGlyphBoundsBuffer() const;
	Anvil::DescriptorSet *GetGlyphBoundsDescriptorSet() const;
protected:
	FontInfo()=default;
	friend FontManager;
private:
	struct DLLWGUI Face
	{
	private:
		FT_Face m_ftFace = nullptr;
	public:
		Face()=default;
		~Face();
		const FT_Face &GetFtFace() const;
	} m_face;
	std::vector<uint8_t> m_data;
	std::vector<std::shared_ptr<GlyphInfo>> m_glyphs;
	bool m_bInitialized = false;
	uint32_t m_size = 0;
	uint32_t m_maxGlyphHeight = 0;
	uint32_t m_maxGlyphSize = 0;
	int32_t m_glyphTopMax = 0;
	uint32_t m_maxBitmapWidth = 0;
	uint32_t m_maxBitmapHeight = 0;
	std::shared_ptr<prosper::Texture> m_glyphMap = nullptr;
	std::shared_ptr<prosper::DescriptorSetGroup> m_glyphMapDescSetGroup = nullptr;
	std::shared_ptr<prosper::Buffer> m_glyphBoundsBuffer = nullptr;
	std::shared_ptr<prosper::DescriptorSetGroup> m_glyphBoundsDsg = nullptr;
};

class DLLWGUI FontManager
{
public:
	static bool Initialize();
	static std::shared_ptr<const FontInfo> GetDefaultFont();
	static const std::unordered_map<std::string,std::shared_ptr<FontInfo>> &GetFonts();
	static void SetDefaultFont(const FontInfo &font);
	static const FT_Library GetFontLibrary();
	static std::shared_ptr<const FontInfo> LoadFont(const std::string &cidentifier,const std::string &cpath,uint32_t fontSize,bool bForceReload=false);
	static std::shared_ptr<const FontInfo> GetFont(const std::string &cfontName);
	static void Close();
	static void GetTextSize(const std::string_view &text,const FontInfo *font,int32_t *width,int32_t *height=nullptr);
	static void GetTextSize(const std::string_view &text,const std::string &font,int32_t *width,int32_t *height=nullptr);
	static void GetTextSize(char c,const FontInfo *font,int32_t *width,int32_t *height=nullptr);
	static void GetTextSize(char c,const std::string &font,int32_t *width,int32_t *height=nullptr);
private:
	struct Library
	{
	private:
		FT_Library m_ftLibrary = nullptr;
	public:
		Library()=default;
		~Library();
		const FT_Library &GetFtLibrary() const;
	};
	static Library m_lib;
	static std::unordered_map<std::string,std::shared_ptr<FontInfo>> m_fonts;
	static std::shared_ptr<const FontInfo> m_fontDefault;
};

#endif
