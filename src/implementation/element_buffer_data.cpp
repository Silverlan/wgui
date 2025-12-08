// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :element_buffer_data;

pragma::gui::WIElementVertexBufferData::~WIElementVertexBufferData()
{
	if(m_buffer != nullptr)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_buffer);
}
void pragma::gui::WIElementVertexBufferData::SetBuffer(const std::shared_ptr<prosper::IBuffer> &buffer) { m_buffer = buffer; }
void pragma::gui::WIElementVertexBufferData::ClearBuffer() { m_buffer = nullptr; }
std::shared_ptr<prosper::IBuffer> pragma::gui::WIElementVertexBufferData::GetBuffer() const { return m_buffer; }
