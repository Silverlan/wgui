// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :types.line;

#undef DrawState

unsigned int pragma::gui::WILineBase::GetLineWidth() { return m_lineWidth; }
void pragma::gui::WILineBase::SetLineWidth(unsigned int width) { m_lineWidth = width; }
pragma::gui::WILineBase::WILineBase() : m_lineWidth(1) {}

////////////////////

static std::shared_ptr<prosper::IBuffer> s_lineBuffer = nullptr;
static uint32_t s_lineCount = 0u;

pragma::gui::types::WILine::WILine() : WIBufferBase(), WILineBase(), m_posStart {util::Vector2iProperty::Create({})}, m_posEnd {util::Vector2iProperty::Create({})}, m_dot(0.f), m_bufColor(nullptr)
{
	++s_lineCount;
	SetShouldScissor(false);
	const std::array<Vector2, 2> verts = {Vector2(0.f, 0.f), Vector2(1.f, 1.f)};
	auto &instance = WGUI::GetInstance();
	auto &context = instance.GetContext();
	auto *pShader = instance.GetColoredLineShader();
	if(pShader != nullptr)
		SetShader(*pShader);

	if(s_lineBuffer == nullptr) {
		prosper::util::BufferCreateInfo bufCreateInfo {};
		bufCreateInfo.usageFlags = prosper::BufferUsageFlags::VertexBufferBit;
		bufCreateInfo.size = util::size_of_container(verts);
		bufCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
		s_lineBuffer = context.CreateBuffer(bufCreateInfo, verts.data());
		s_lineBuffer->SetDebugName("gui_line_vertex_buf");
	}
	InitializeBufferData(*s_lineBuffer);

	const std::array<Vector4, 2> colors = {Vector4 {1.f, 1.f, 1.f, 1.f}, Vector4 {1.f, 1.f, 1.f, 1.f}};
	prosper::util::BufferCreateInfo bufCreateInfo {};
	bufCreateInfo.usageFlags = prosper::BufferUsageFlags::VertexBufferBit | prosper::BufferUsageFlags::TransferDstBit;
	bufCreateInfo.size = util::size_of_container(colors);
	bufCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
	m_bufColor = context.CreateBuffer(bufCreateInfo, colors.data());
	m_bufColor->SetDebugName("gui_line_color_buf");

	auto col = colors::White.ToVector4();
	SetColor(col.x, col.y, col.z, col.w);
}

pragma::gui::types::WILine::~WILine()
{
	if(m_bufColor != nullptr)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_bufColor);
	if(--s_lineCount == 0u)
		s_lineBuffer = nullptr;
}

void pragma::gui::types::WILine::UpdateColorBuffer()
{
	auto &colStart = GetStartColor();
	auto &colEnd = GetEndColor();
	std::array<Vector4, 2> colors = {colStart.ToVector4(), colEnd.ToVector4()};
	/*prosper::util::record_update_buffer(
		*WGUI::GetInstance().GetContext().GetDrawCommandBuffer(),*m_bufColor,0ull,colors,
		vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eVertexInput | vk::PipelineStageFlagBits::eGeometryShader,
		vk::AccessFlagBits::eShaderRead
	);*/
	m_bufColor->Write(0u, colors.size() * sizeof(colors.front()), colors.data());
}

void pragma::gui::types::WILine::SetColor(float r, float g, float b, float a)
{
	WIBase::SetColor(r, g, b, a);
	Color col {static_cast<int16_t>(umath::round(r * 255.f)), static_cast<int16_t>(umath::round(g * 255.f)), static_cast<int16_t>(umath::round(b * 255.f)), static_cast<int16_t>(umath::round(a * 255.f))};
	m_colStart = col;
	m_colEnd = col;
	UpdateColorBuffer();
}

void pragma::gui::types::WILine::SetLineWidth(unsigned int width) { pragma::gui::WILineBase::SetLineWidth(width); }
unsigned int pragma::gui::types::WILine::GetLineWidth() { return pragma::gui::WILineBase::GetLineWidth(); }

void pragma::gui::types::WILine::SetStartColor(const Color &col)
{
	m_colStart = col;
	UpdateColorBuffer();
}
void pragma::gui::types::WILine::SetEndColor(const Color &col)
{
	m_colEnd = col;
	UpdateColorBuffer();
}
const Color &pragma::gui::types::WILine::GetStartColor() const { return m_colStart; }
const Color &pragma::gui::types::WILine::GetEndColor() const { return m_colEnd; }

unsigned int pragma::gui::types::WILine::GetVertexCount() { return 2; }
Mat4 pragma::gui::types::WILine::GetTransformPose(const Vector2i &origin, int w, int h, const Mat4 &poseParent, const Vector2 &scale) const
{
	auto bounds = GetNormalizedLineBounds();
	const auto &posStart = bounds.first;
	const auto &posEnd = bounds.second;
	Vector3 size {(posEnd.x - posStart.x), (posEnd.y - posStart.y), 0};
	Vector2 offset {origin.x, origin.y};
	if(posStart.x > posEnd.x)
		offset.x += posStart.x - posEnd.x;
	if(posStart.y > posEnd.y)
		offset.y += posStart.y - posEnd.y;
	Vector3 normOrigin {offset * 2.f, 0.f};
	Vector3 normScale {size.x * scale.x, size.y * scale.y, 0};

	if(!m_rotationMatrix)
		return poseParent * glm::gtc::scale(glm::gtc::translate(normOrigin), normScale);

	auto t = glm::gtc::translate(normOrigin);
	auto s = glm::gtc::scale(normScale);
	auto &r = *m_rotationMatrix;
	auto m = t * r * s;
	return poseParent * m;
}

void pragma::gui::types::WILine::Render(const DrawInfo &drawInfo, DrawState &drawState, const Mat4 &matDraw, const Vector2 &scale, uint32_t testStencilLevel, StencilPipeline stencilPipeline)
{
	auto *pShader = GetShader();
	auto col = drawInfo.GetColor(*this, drawState);
	if(col.a <= 0.f)
		return;

	if(s_lineBuffer == nullptr || m_bufColor == nullptr)
		return;
	auto &shader = static_cast<shaders::ShaderColoredLine &>(*pShader);
	auto &context = WGUI::GetInstance().GetContext();
	prosper::ShaderBindState bindState {*drawInfo.commandBuffer};
	if(shader.RecordBeginDraw(bindState, drawState, drawInfo.size.x, drawInfo.size.y, stencilPipeline, umath::is_flag_set(drawInfo.flags, DrawInfo::Flags::Msaa)) == true) {
		ElementData pushConstants {matDraw, col, ElementData::ToViewportSize(drawInfo.size)};
		shader.RecordDraw(bindState, s_lineBuffer, m_bufColor, GetVertexCount(), GetLineWidth(), pushConstants, testStencilLevel);
		shader.RecordEndDraw(bindState);
	}
}

void pragma::gui::types::WILine::SizeToContents(bool x, bool y)
{
	auto bounds = GetNormalizedLineBounds();
	const Vector2i &pos = bounds.first;
	const Vector2i &posEnd = bounds.second;
	int xL, xR, yU, yB;
	if(pos.x < posEnd.x) {
		xL = pos.x;
		xR = posEnd.x;
	}
	else {
		xL = posEnd.x;
		xR = pos.x;
	}
	if(pos.y < posEnd.y) {
		yU = pos.y;
		yB = posEnd.y;
	}
	else {
		yU = posEnd.y;
		yB = pos.y;
	}
	SetPos(xL, yU);
	auto w = xR - xL;
	auto h = yB - yU + 1;
	if(x && y)
		SetSize(w, h);
	else if(x)
		SetWidth(w);
	else if(y)
		SetHeight(h);
	Vector2 dir = Vector2(posEnd.x, posEnd.y) - Vector2(posEnd.x, posEnd.y);
	dir = glm::normalize(dir);
	m_dot = glm::dot(Vector2(1.f, 0.f), dir);
}

std::pair<Vector2i, Vector2i> pragma::gui::types::WILine::GetNormalizedLineBounds() const { return {*m_posStart, *m_posEnd}; }

const util::PVector2iProperty &pragma::gui::types::WILine::GetStartPosProperty() const { return m_posStart; }
const util::PVector2iProperty &pragma::gui::types::WILine::GetEndPosProperty() const { return m_posEnd; }
Vector2i &pragma::gui::types::WILine::GetStartPos() const { return *m_posStart; }
void pragma::gui::types::WILine::SetStartPos(Vector2i pos) { SetStartPos(pos.x, pos.y); }
void pragma::gui::types::WILine::SetStartPos(int x, int y) { *m_posStart = Vector2i {x, y}; }

Vector2i &pragma::gui::types::WILine::GetEndPos() const { return *m_posEnd; }
void pragma::gui::types::WILine::SetEndPos(Vector2i pos) { SetEndPos(pos.x, pos.y); }
void pragma::gui::types::WILine::SetEndPos(int x, int y) { *m_posEnd = Vector2i {x, y}; }
