// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :types.rounded_base;

pragma::gui::types::WIRoundedBase::WIRoundedBase() : m_roundness(3), m_cornerSize(0.2f), m_bRoundUpperLeft(true), m_bRoundUpperRight(true), m_bRoundLowerLeft(true), m_bRoundLowerRight(true) {}

void pragma::gui::types::WIRoundedBase::Initialize() { DoUpdate(); }

void pragma::gui::types::WIRoundedBase::DoUpdate()
{
	WIShape *shape = dynamic_cast<WIShape *>(this);
	shape->ClearVertices();
	float cornerSize = GetCornerSize();
	float offset = 1.f - cornerSize;
	float roundness = GetRoundness();
	float step = 90.f / powf(2.f, roundness);
	float radPos = 0.f;
	float radNeg = -0.f;
	float sinPos = sinf(radPos);
	float cosPos = cosf(radPos);
	float sinNeg = sinf(radNeg);
	float cosNeg = cosf(radNeg);
	for(float f = 0.f; f <= (90.f - step); f += step) {
		float radPosNext = ((f + step) / 180.f) * float(math::pi);
		float radNegNext = ((-f - step) / 180.f) * float(math::pi);
		float sinPosNext = sinf(radPosNext);
		float cosPosNext = cosf(radPosNext);
		float sinNegNext = sinf(radNegNext);
		float cosNegNext = cosf(radNegNext);

		// Top Left
		if(m_bRoundUpperLeft == true) {
			shape->AddVertex(Vector2(sinNegNext, -cosNegNext) * cornerSize + Vector2(-offset, -offset));
			shape->AddVertex(Vector2(-0.f, -0.f));
			shape->AddVertex(Vector2(sinNeg, -cosNeg) * cornerSize + Vector2(-offset, -offset));
		}

		// Bottom Left
		if(m_bRoundLowerLeft == true) {
			shape->AddVertex(Vector2(0.f, 0.f));
			shape->AddVertex(Vector2(sinNegNext, cosNegNext) * cornerSize + Vector2(-offset, offset));
			shape->AddVertex(Vector2(sinNeg, cosNeg) * cornerSize + Vector2(-offset, offset));
		}

		// Bottom Right
		if(m_bRoundLowerRight == true) {
			shape->AddVertex(Vector2(sinPosNext, cosPosNext) * cornerSize + Vector2(offset, offset));
			shape->AddVertex(Vector2(0.f, 0.f));
			shape->AddVertex(Vector2(sinPos, cosPos) * cornerSize + Vector2(offset, offset));
		}

		// Top Right
		if(m_bRoundUpperRight == true) {
			shape->AddVertex(Vector2(0.f, -0.f));
			shape->AddVertex(Vector2(-sinNegNext, -cosNegNext) * cornerSize + Vector2(offset, -offset));
			shape->AddVertex(Vector2(-sinNeg, -cosNeg) * cornerSize + Vector2(offset, -offset));
		}

		radPos = radPosNext;
		radNeg = radNegNext;
		sinPos = sinPosNext;
		cosPos = cosPosNext;
		sinNeg = sinNegNext;
		cosNeg = cosNegNext;
	}
	// Bottom
	shape->AddVertex(Vector2((m_bRoundLowerRight == false) ? 1.f : offset, 1.f));
	shape->AddVertex(Vector2(0.f, 0.f));
	shape->AddVertex(Vector2((m_bRoundLowerLeft == false) ? -1.f : -offset, 1));

	// Right
	shape->AddVertex(Vector2(1.f, (m_bRoundUpperRight == false) ? -1.f : -offset));
	shape->AddVertex(Vector2(0.f, 0.f));
	shape->AddVertex(Vector2(1.f, (m_bRoundLowerRight == false) ? 1.f : offset));

	// Top
	shape->AddVertex(Vector2((m_bRoundUpperLeft == false) ? -1.f : -offset, -1.f));
	shape->AddVertex(Vector2(0.f, 0.f));
	shape->AddVertex(Vector2((m_bRoundUpperRight == false) ? 1.f : offset, -1.f));

	// Left
	shape->AddVertex(Vector2(-1.f, (m_bRoundLowerLeft == false) ? 1.f : offset));
	shape->AddVertex(Vector2(0.f, 0.f));
	shape->AddVertex(Vector2(-1.f, (m_bRoundUpperLeft == false) ? -1.f : -offset));
}

char pragma::gui::types::WIRoundedBase::GetRoundness() { return m_roundness; }

void pragma::gui::types::WIRoundedBase::SetRoundness(char roundness)
{
	if(roundness < 0)
		roundness = 0;
	if(roundness > 6)
		roundness = 6;
	m_roundness = roundness;
}

void pragma::gui::types::WIRoundedBase::SetCornerSize(float size)
{
	if(size > 1.f)
		size = 1.f;
	if(size < 0.f)
		size = 0.f;
	m_cornerSize = size;
}

float pragma::gui::types::WIRoundedBase::GetCornerSize() { return m_cornerSize; }

void pragma::gui::types::WIRoundedBase::SetRoundTopRight(bool b) { m_bRoundUpperRight = b; }
void pragma::gui::types::WIRoundedBase::SetRoundTopLeft(bool b) { m_bRoundUpperLeft = b; }
void pragma::gui::types::WIRoundedBase::SetRoundBottomLeft(bool b) { m_bRoundLowerLeft = b; }
void pragma::gui::types::WIRoundedBase::SetRoundBottomRight(bool b) { m_bRoundLowerRight = b; }
bool pragma::gui::types::WIRoundedBase::IsTopRightRound() { return m_bRoundUpperRight; }
bool pragma::gui::types::WIRoundedBase::IsTopLeftRound() { return m_bRoundUpperLeft; }
bool pragma::gui::types::WIRoundedBase::IsBottomLeftRound() { return m_bRoundLowerLeft; }
bool pragma::gui::types::WIRoundedBase::IsBottomRightRound() { return m_bRoundLowerRight; }
