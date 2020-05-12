/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "texturemanager/texturemanager.h"
#include "wgui/types/wishape.h"
#include "cmaterialmanager.h"
#include "textureinfo.h"
#include "wgui/shaders/wishader_textured.hpp"
#include <prosper_context.hpp>
#include <buffers/prosper_buffer.hpp>
#include <prosper_util.hpp>
#include <prosper_util_square_shape.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <buffers/prosper_uniform_resizable_buffer.hpp>

LINK_WGUI_TO_CLASS(WIShape,WIShape);
LINK_WGUI_TO_CLASS(WITexturedShape,WITexturedShape);

WIShape::WIShape()
	: WIBufferBase(),m_vertexBufferUpdateRequired(false)
{}
unsigned int WIShape::AddVertex(Vector2 vert)
{
	m_vertices.push_back(vert);
	m_vertexBufferUpdateRequired |= 1;
	return static_cast<unsigned int>(m_vertices.size());
}
void WIShape::SetVertexPos(unsigned int vertID,Vector2 pos)
{
	if(m_vertices.size() <= vertID)
		return;
	m_vertices[vertID] = pos;
	m_vertexBufferUpdateRequired |= 1;
}
void WIShape::ClearVertices() {m_vertices.clear(); m_vertexBufferUpdateRequired |= 1;}
void WIShape::DoUpdate()
{
	WIBase::DoUpdate();
	if(!(m_vertexBufferUpdateRequired &1) || m_vertices.size() == 0)
		return;
	m_vertexBufferUpdateRequired &= ~1;
	
	auto &context = WGUI::GetInstance().GetContext();
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.size = m_vertices.size() *sizeof(Vector2);
	createInfo.usageFlags = prosper::BufferUsageFlags::VertexBufferBit;
	createInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
	auto buf = context.CreateBuffer(createInfo,m_vertices.data());
	buf->SetDebugName("gui_shape_vertex_buf");
	InitializeBufferData(*buf);
}
unsigned int WIShape::GetVertexCount() {return static_cast<unsigned int>(m_vertices.size());}
void WIShape::InvertVertexPositions(bool x,bool y)
{
	for(auto &v : m_vertices)
	{
		if(x == true)
			v.x = 2.f -(v.x +1.f) -1.f;
		if(y == true)
			v.y = 2.f -(v.y +1.f) -1.f;
	}
	m_vertexBufferUpdateRequired |= 1;
	Update();
}

///////////////////

WIOutlinedShape::WIOutlinedShape()
	: WIShape(),WILineBase()
{}

///////////////////

WITexturedShape::WITexturedShape()
	: WIShape(),m_hMaterial(),m_texture(),
	m_uvBuffer(nullptr),m_shader(),m_descSetTextureGroup(nullptr),m_texLoadCallback(nullptr)
{
	auto &instance = WGUI::GetInstance();
	auto *pShader = instance.GetTexturedShader();
	auto *pShaderCheap = instance.GetTexturedRectShader();
	if(pShader != nullptr)
		SetShader(*pShader,pShaderCheap);
	ReloadDescriptorSet();
}
void WITexturedShape::SetShader(prosper::Shader &shader,prosper::Shader *shaderCheap)
{
	if(dynamic_cast<wgui::ShaderTextured*>(&shader) != nullptr)
		m_shader = shader.GetHandle();
	else
		m_shader = {};
	
	if(dynamic_cast<wgui::ShaderTexturedRect*>(shaderCheap) != nullptr)
		m_shaderCheap = shaderCheap->GetHandle();
	else
		m_shaderCheap = {};
}
void WITexturedShape::ReloadDescriptorSet()
{
	m_descSetTextureGroup = nullptr;
	if(wgui::ShaderTextured::DESCRIPTOR_SET_TEXTURE.IsValid() == false)
		return;
	m_descSetTextureGroup = WGUI::GetInstance().GetContext().CreateDescriptorSetGroup(wgui::ShaderTextured::DESCRIPTOR_SET_TEXTURE);
	//m_descSetTextureGroup = prosper::util::create_descriptor_set_group(WGUI::GetInstance().GetContext().GetDevice(),m_shader.get()->GetDescriptorSetGroup(),false); // TODO: FIXME
}
prosper::IBuffer &WITexturedShape::GetUVBuffer() const {return *m_uvBuffer;}
void WITexturedShape::SetUVBuffer(prosper::IBuffer &buffer) {m_uvBuffer = buffer.shared_from_this();}
void WITexturedShape::SetAlphaOnly(bool b) {umath::set_flag(m_stateFlags,StateFlags::AlphaOnly,b);}
bool WITexturedShape::GetAlphaOnly() const {return umath::is_flag_set(m_stateFlags,StateFlags::AlphaOnly);}
float WITexturedShape::GetLOD() const {return m_lod;}
void WITexturedShape::SetLOD(float lod) {m_lod = lod;}
void WITexturedShape::ClearTextureLoadCallback()
{
	if(m_texLoadCallback != nullptr)
		*m_texLoadCallback = false;
	m_texLoadCallback = nullptr;
}
void WITexturedShape::InvertVertexUVCoordinates(bool x,bool y)
{
	for(auto &v : m_uvs)
	{
		if(x == true)
			v.x = 1.f -v.x;
		if(y == true)
			v.y = 1.f -v.y;
	}
	m_vertexBufferUpdateRequired |= 2;
	Update();
}
void WITexturedShape::InitializeTextureLoadCallback(const std::shared_ptr<Texture> &texture)
{
	auto hThis = GetHandle();
	m_texLoadCallback = std::make_shared<bool>(true);
	auto bLoadCallback = m_texLoadCallback;
	texture->CallOnLoaded([hThis,bLoadCallback](std::shared_ptr<Texture> texture) {
		if((bLoadCallback != nullptr && *bLoadCallback == false) || !hThis.IsValid() || texture->HasValidVkTexture() == false)
			return;
		if(hThis.get<WITexturedShape>()->m_descSetTextureGroup == nullptr)
			return;
		auto &descSet = *hThis.get<WITexturedShape>()->m_descSetTextureGroup->GetDescriptorSet();
		descSet.SetBindingTexture(*texture->GetVkTexture(),0u);
	});
}
void WITexturedShape::SetMaterial(Material *material)
{
	ClearTexture();
	m_hMaterial = material->GetHandle();
	if(!m_hMaterial.IsValid())
		return;
	auto *diffuseMap = m_hMaterial->GetDiffuseMap();
	if(diffuseMap == nullptr || diffuseMap->texture == nullptr)
		return;
	auto diffuseTexture = std::static_pointer_cast<Texture>(diffuseMap->texture);
	if(diffuseTexture == nullptr)
		return;
	InitializeTextureLoadCallback(diffuseTexture);
}
void WITexturedShape::SetMaterial(const std::string &material)
{
	auto *mat = WGUI::GetInstance().GetMaterialLoadHandler()(material);
	if(mat == nullptr)
		SetMaterial(nullptr);
	else
		SetMaterial(mat);
}
Material *WITexturedShape::GetMaterial()
{
	if(!m_hMaterial.IsValid())
		return nullptr;
	return m_hMaterial.get();
}
void WITexturedShape::ClearTexture()
{
	ClearTextureLoadCallback();
	m_hMaterial = MaterialHandle();
	m_texture = nullptr;
}
void WITexturedShape::SetTexture(prosper::Texture &tex,uint32_t layerIndex)
{
	ClearTexture();
	m_texture = tex.shared_from_this();
	
	ReloadDescriptorSet(); // Need to generate a new descriptor set and keep the old one alive, in case it was still in use
	m_descSetTextureGroup->GetDescriptorSet()->SetBindingTexture(tex,0u,layerIndex);
}
const std::shared_ptr<prosper::Texture> &WITexturedShape::GetTexture() const
{
	if(m_texture || m_hMaterial.IsValid() == false)
		return m_texture;
	auto *diffuseMap = m_hMaterial.get()->GetDiffuseMap();
	if(diffuseMap == nullptr || diffuseMap->texture == nullptr)
		return m_texture;
	auto diffuseTexture = std::static_pointer_cast<Texture>(diffuseMap->texture);
	if(diffuseTexture == nullptr)
		return m_texture;
	return diffuseTexture->GetVkTexture();
}

void WITexturedShape::SetVertexUVCoord(unsigned int vertID,Vector2 uv)
{
	if(m_uvs.size() <= vertID)
		return;
	m_uvs[vertID] = uv;
	m_vertexBufferUpdateRequired |= 2;
}
void WITexturedShape::ClearVertices()
{
	WIShape::ClearVertices();
	m_uvs.clear();
}
WITexturedShape::~WITexturedShape()
{
	if(m_uvBuffer != nullptr)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_uvBuffer);
	ClearTextureLoadCallback();
}
unsigned int WITexturedShape::AddVertex(Vector2 vert)
{
	Vector2 uv(vert.x +1.f,vert.y +1.f);
	if(uv.x != 0.f) uv.x /= 2.f;
	if(uv.y != 0.f) uv.y /= 2.f;
	return AddVertex(vert,uv);
}
unsigned int WITexturedShape::AddVertex(Vector2 vert,Vector2 uv)
{
	m_uvs.push_back(uv);
	m_vertexBufferUpdateRequired |= 2;
	return WIShape::AddVertex(vert);
}
void WITexturedShape::DoUpdate()
{
	WIShape::DoUpdate();
	if(!(m_vertexBufferUpdateRequired &2) || m_uvs.size() == 0)
		return;
	m_vertexBufferUpdateRequired &= ~2;

	auto &context = WGUI::GetInstance().GetContext();
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.usageFlags = prosper::BufferUsageFlags::VertexBufferBit;
	createInfo.size = m_uvs.size() *sizeof(Vector2);
	createInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
	m_uvBuffer = context.CreateBuffer(createInfo,m_uvs.data());
}
void WITexturedShape::SetChannelSwizzle(wgui::ShaderTextured::Channel dst,wgui::ShaderTextured::Channel src)
{
	m_channels.at(umath::to_integral(src)) = dst;
}
wgui::ShaderTextured::Channel WITexturedShape::GetChannelSwizzle(wgui::ShaderTextured::Channel channel) const
{
	return m_channels.at(umath::to_integral(channel));
}
void WITexturedShape::SetShader(wgui::ShaderTextured &shader)
{
	m_shader = shader.GetHandle();
	m_stateFlags |= StateFlags::ShaderOverride;
}
void WITexturedShape::SizeToTexture()
{
	if(m_texture || m_hMaterial.IsValid() == false)
		return;
	uint32_t width,height;
	if(m_texture)
	{
		auto extents = m_texture->GetImage().GetExtents();
		width = extents.width;
		height = extents.height;
	}
	else
	{
		auto *diffuseMap = m_hMaterial.get()->GetDiffuseMap();
		if(diffuseMap == nullptr || diffuseMap->texture == nullptr)
			return;
		width = diffuseMap->width;
		height = diffuseMap->height;
	}
	SetSize(width,height);
}
void WITexturedShape::Render(const DrawInfo &drawInfo,const Mat4 &matDraw)
{
	if(m_hMaterial.IsValid() == false && m_texture == nullptr)
		return;
	auto col = drawInfo.GetColor(*this);
	if(col.a <= 0.f)
		return;
	// Try to use cheap shader if no custom vertex buffer was used
	if(umath::is_flag_set(m_stateFlags,StateFlags::ShaderOverride) == false && ((m_vertexBufferData == nullptr && m_uvBuffer == nullptr) || m_shader.expired()))
	{
		auto *pShaderCheap = static_cast<wgui::ShaderTexturedRect*>(GetCheapShader());
		if(pShaderCheap == nullptr)
			return;
		auto &context = WGUI::GetInstance().GetContext();
		if(pShaderCheap->BeginDraw(context.GetDrawCommandBuffer(),drawInfo.size.x,drawInfo.size.y) == true)
		{
			pShaderCheap->Draw({
				matDraw,col,umath::is_flag_set(m_stateFlags,StateFlags::AlphaOnly) ? 1 : 0,m_lod,
				m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Red)),
				m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Green)),
				m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Blue)),
				m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Alpha))
			},*m_descSetTextureGroup->GetDescriptorSet(0u));
			pShaderCheap->EndDraw();
		}
		return;
	}
	//

	if(m_shader.expired())
		return;
	auto &shader = static_cast<wgui::ShaderTextured&>(*m_shader.get());
	auto &context = WGUI::GetInstance().GetContext();
	auto vbuf = (m_vertexBufferData != nullptr) ? m_vertexBufferData->GetBuffer() : prosper::util::get_square_vertex_buffer(context);
	auto uvBuf = (m_uvBuffer != nullptr) ? m_uvBuffer : prosper::util::get_square_uv_buffer(context);
	if(vbuf == nullptr || uvBuf == nullptr)
		return;
	if(shader.BeginDraw(context.GetDrawCommandBuffer(),drawInfo.size.x,drawInfo.size.y) == true)
	{
		wgui::ShaderTextured::PushConstants pushConstants {};
		pushConstants.elementData.modelMatrix = matDraw;
		pushConstants.elementData.color = col;
		pushConstants.lod = m_lod;
		pushConstants.red = m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Red));
		pushConstants.green = m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Green));
		pushConstants.blue = m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Blue));
		pushConstants.alpha = m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Alpha));
		shader.Draw(
			vbuf,
			uvBuf,
			GetVertexCount(),*m_descSetTextureGroup->GetDescriptorSet(0u),pushConstants
		);
		shader.EndDraw();
	}
}
