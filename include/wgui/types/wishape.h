/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WISHAPE_H__
#define __WISHAPE_H__
#include "wgui/wibase.h"
#include <material_manager2.hpp>
#include "wgui/wibufferbase.h"
#include "wgui/shaders/wishader_textured.hpp"
#include "wiline.h"
#include <sharedutils/alpha_mode.hpp>
#include <texturemanager/texture.h>

class DLLWGUI WIShape
	: public WIBufferBase
{
public:
	enum class BasicShape : uint32_t
	{
		Rectangle = 0,
		Circle
	};
	static std::shared_ptr<prosper::IBuffer> CreateBuffer(const std::vector<Vector2> &verts);
	WIShape();
	~WIShape() override;
	virtual unsigned int AddVertex(Vector2 vert);
	void SetVertexPos(unsigned int vertID,Vector2 pos);
	void InvertVertexPositions(bool x=true,bool y=true);
	virtual void ClearVertices();
	virtual unsigned int GetVertexCount() override;
	void SetBuffer(prosper::IBuffer &buffer,uint32_t numVerts);
	void SetShape(BasicShape shape);
	void SetBoundsCheckFunction(const std::function<bool(const WIShape&,const Vector2i&)> &func);
	virtual void ClearBuffer() override;
protected:
	virtual void DoUpdate() override;
	virtual bool DoPosInBounds(const Vector2i &pos) const override;
	std::function<bool(const WIShape&,const Vector2i&)> m_checkInBounds = nullptr;
	std::optional<uint32_t> m_bufferVertexCount {};
	std::vector<Vector2> m_vertices;
	uint8_t m_vertexBufferUpdateRequired;
};

class DLLWGUI WIOutlinedShape
	: public WIShape,WILineBase
{
public:
	WIOutlinedShape();
	virtual ~WIOutlinedShape() override = default;
};

namespace prosper {class IDescriptorSetGroup;};
class WGUIShaderTextured;
class DLLWGUI WITexturedShape
	: public WIShape
{
public:
	enum class StateFlags : uint8_t
	{
		None = 0,
		AlphaOnly = 1u,
		ShaderOverride = AlphaOnly<<1u,
		ExpensiveShaderRequired = ShaderOverride<<1u
	};
	static std::shared_ptr<prosper::IBuffer> CreateUvBuffer(const std::vector<Vector2> &uvs);
	WITexturedShape();
	virtual ~WITexturedShape() override;
	virtual unsigned int AddVertex(Vector2 vert) override;
	virtual std::ostream &Print(std::ostream &stream) const override;
	unsigned int AddVertex(Vector2 vert,Vector2 uv);
	void SetVertexUVCoord(unsigned int vertID,Vector2 uv);
	void InvertVertexUVCoordinates(bool x=true,bool y=true);
	virtual void ClearVertices() override;
	prosper::IBuffer &GetUVBuffer() const;
	void SetUVBuffer(prosper::IBuffer &buffer);
	void SetAlphaOnly(bool b);
	bool GetAlphaOnly() const;
	float GetLOD() const;
	void SetLOD(float lod);
	void SetMaterial(Material *material);
	void SetMaterial(const std::string &material);
	Material *GetMaterial();
	void SetTexture(prosper::Texture &tex,std::optional<uint32_t> layerIndex={});
	void ClearTexture();
	const std::shared_ptr<prosper::Texture> &GetTexture() const;
	virtual void Render(const DrawInfo &drawInfo,wgui::DrawState &drawState,const Mat4 &matDraw,const Vector2 &scale,uint32_t testStencilLevel=0u,wgui::StencilPipeline stencilPipeline=wgui::StencilPipeline::Test) override;
	void SizeToTexture();
	void SetShader(wgui::ShaderTextured &shader);

	void SetAlphaMode(AlphaMode alphaMode);
	void SetAlphaCutoff(float alphaCutoff);
	AlphaMode GetAlphaMode() const;
	float GetAlphaCutoff() const;

	void SetChannelSwizzle(wgui::ShaderTextured::Channel dst,wgui::ShaderTextured::Channel src);
	wgui::ShaderTextured::Channel GetChannelSwizzle(wgui::ShaderTextured::Channel channel) const;

	virtual void ClearBuffer() override;
protected:
	virtual void UpdateTransparencyState() override;
	void UpdateShaderState();
	virtual void DoUpdate() override;
	msys::MaterialHandle m_hMaterial;
	std::shared_ptr<prosper::Texture> m_texture = nullptr;
	std::shared_ptr<bool> m_texLoadCallback;
	std::shared_ptr<prosper::IBuffer> m_uvBuffer = nullptr;
	std::vector<Vector2> m_uvs;
	AlphaMode m_alphaMode = AlphaMode::Blend;
	float m_alphaCutoff = 1.f;
	std::array<wgui::ShaderTextured::Channel,4> m_channels = {
		wgui::ShaderTextured::Channel::Red,
		wgui::ShaderTextured::Channel::Green,
		wgui::ShaderTextured::Channel::Blue,
		wgui::ShaderTextured::Channel::Alpha
	};
	float m_lod = -1.f;

	void ReloadDescriptorSet();
	virtual void SetShader(prosper::Shader &shader,prosper::Shader *shaderCheap=nullptr) override;
private:
	StateFlags m_stateFlags = StateFlags::None;
	util::WeakHandle<prosper::Shader> m_shader = {};
	std::shared_ptr<prosper::IDescriptorSetGroup> m_descSetTextureGroup = nullptr;
	uint32_t m_matUpdateCountRef = std::numeric_limits<uint32_t>::max();
	uint32_t m_texUpdateCountRef = std::numeric_limits<uint32_t>::max();
	void UpdateMaterialDescriptorSetTexture();
	void ClearTextureLoadCallback();
	void InitializeTextureLoadCallback(const std::shared_ptr<Texture> &texture);
};
REGISTER_BASIC_BITWISE_OPERATORS(WITexturedShape::StateFlags)

#endif
