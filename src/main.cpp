/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/fontmanager.h"
#include <fsys/filesystem.h>
#include "wgui/wibase.h"
#include "wgui/types/wirect.h"
#include "wgui/types/wishape.h"
#include "wgui/types/witext.h"
#include "wgui/types/witextentry.h"
#include "wgui/types/wiline.h"
#include "wgui/types/wibutton.h"
#include "wgui/wgui.h"
#include <cmaterialmanager.h>
#include <iostream>
#include <vector>
#include <algorithm>
/*
class GL2DBox
	: public GL2DShape
{
public:
	GL2DBox()
	{

	}
};*/

#ifdef WGUI_DLL
/*
BOOLEAN WINAPI DllMain(IN HINSTANCE hDllHandle,IN DWORD nReason,IN LPVOID reserved)
{
	switch(nReason)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
*/
#else
#ifdef WGUI_EXE
int main(int argc,char *argv[])
{
	std::cout<<"GUI Test"<<std::endl;
	if(OpenGL::Initialize() == false)
		exit(-1);
	int err;
	OpenGL::SetWindowHint(GLFW_REFRESH_RATE,0);
	OpenGL::SetWindowHint(GLFW_SAMPLES,4);
	OpenGL::SetWindowHint(GLFW_DECORATED,GL_FALSE);
	OpenGL::SetWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
	if(OpenGL::OpenContext("WGUI Test",800,600,&err) == NULL)
	{
		switch(err)
		{
		case OPENGL_ERR_GLFW_FAIL:
			std::cout<<"ERROR: Failed to initialize glfw"<<std::endl;
			exit(-1);
		case OPENGL_ERR_GLFW_CONTEXT_FAIL:
			std::cout<<"ERROR: Unable to open context"<<std::endl;
			exit(-1);
		case OPENGL_ERR_GLEW_FAIL:
			std::cout<<"ERROR: Failed to initialize GLEW"<<std::endl;
			exit(-1);
		}
	}
	if(WGUI::Initialize() == NULL)
	{
		std::cout<<"ERROR: Failed to initialize GUI system"<<std::endl;
		exit(-1);
	}
	CMaterialSystem::Initialize();

	OpenGL::SetInputMode(GLFW_CURSOR,GLFW_CURSOR_HIDDEN);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CW); // Clockwise

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	GLenum glerr = glewInit();

	WIRect *pRect = WGUI::Create<WIRect>();
	pRect->SetSize(800,600);
	pRect->SetColor(1,0,0,0.3);
	glClearColor(0.0f,0.0f,0.0f,0.0f);

	while(true)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		WGUI::Draw();
		OpenGL::SwapBuffers();
		OpenGL::PollEvents();
	}
	return 0;
}
/*int main(int argc,char *argv[])
{
	std::cout<<"GUI Test"<<std::endl;
	if(OpenGL::Initialize() == false)
		exit(-1);
	int err;
	if(OpenGL::OpenContext("WGUI Test",800,600,&err) == NULL)
	{
		switch(err)
		{
		case OPENGL_ERR_GLFW_FAIL:
			std::cout<<"ERROR: Failed to initialize glfw"<<std::endl;
			exit(-1);
		case OPENGL_ERR_GLFW_CONTEXT_FAIL:
			std::cout<<"ERROR: Unable to open context"<<std::endl;
			exit(-1);
		case OPENGL_ERR_GLEW_FAIL:
			std::cout<<"ERROR: Failed to initialize GLEW"<<std::endl;
			exit(-1);
		}
	}
	if(WGUI::Initialize() == NULL)
	{
		std::cout<<"ERROR: Failed to initialize GUI system"<<std::endl;
		exit(-1);
	}
	CMaterialSystem::Initialize();
	GLuint shaderIDB = OpenGL::LoadShader("WGUIColored","vs_wgui_colored.gls","fs_wgui_colored.gls");
	GLuint shaderID = OpenGL::LoadShader("WGUITextured","vs_wgui_textured.gls","fs_wgui_textured.gls");
	GLuint shaderIDText = OpenGL::LoadShader("WUIText","vs_wgui_textured.gls","fs_wgui_text.gls");
	GLuint shaderTest = OpenGL::LoadShader("WGUITest","vs_simpletest.gls","fs_simpletest.gls");

	OpenGL::Enable(GL_BLEND);
	OpenGL::SetBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	int width,height;
	OpenGL::GetWindowSize(&width,&height);
	std::cout<<"Resolution: "<<width<<","<<height<<std::endl;
	Material *mat = CMaterialSystem::Load("hud_essense");
	ShaderInfo *shader = mat->GetShaderInfo();

	WIRect *hud = WGUI::Create<WIRect>();
	hud->SetColor(Vector4(1,0,0,0.5));
	hud->SetSize(width,height);

	WITexturedRect *pEssence = WGUI::Create<WITexturedRect>();
	pEssence->SetMaterial("hud_essense");
	std::cout<<"Shader: "<<shader->GetIdentifier()<<"("<<shader->GetShader()<<")"<<std::endl;
	pEssence->SetSize(256,256);
	pEssence->SetPos(0,height -256);

	WITexturedRect *pHp = WGUI::Create<WITexturedRect>();
	pHp->SetMaterial("hud_hpback");
	//pHp->SetColor(Vector4(1,0,0,1));
	pHp->SetSize(256,256);
	pHp->SetPos(-64,height -256);

	WIRect *pRect = WGUI::Create<WIRect>();
	pRect->SetSize(250,250);
	pRect->SetPos(400,400);
	pRect->SetColor(Vector4(1,0,0,1));

	WIRect *pRectSub = WGUI::Create<WIRect>(pRect);
	pRectSub->SetSize(125,125);
	pRectSub->SetPos(50,50);
	pRectSub->SetColor(Vector4(0,1,0,1));
	pRectSub->SetMouseInputEnabled(true);
	pRectSub->SetKeyboardInputEnabled(true);

	WIText *pText = WGUI::Create<WIText>();
	pText->SetText("The Quick Brown Fox Jumps Over!gg");
	pText->SetPos(0,0);
	pText->SetColor(0,0,1,1);
	pText->SizeToContents();
	int wText,hText;
	pText->GetSize(&wText,&hText);
	std::cout<<"Text Size: "<<wText<<","<<hText<<std::endl;

	WIRect *textFG = WGUI::Create<WIRect>();
	textFG->SetColor(Vector4(0,1,0,0.2));
	textFG->SetSize(*pText->GetSize());

	WITextEntry *pTe = WGUI::Create<WITextEntry>();
	pTe->SetSize(450,160);
	pTe->SetPos(100,100);
	pTe->Update();
	pTe->SetText("p[2qw4");
	pTe->SetMultiLine(true);

	WIButton *pButton = WGUI::Create<WIButton>();
	pButton->SetText("Button");
	pButton->Update();
	pButton->SetSize(80,50);
	pButton->SetPos(150,400);

	WIRect *wt = WGUI::Create<WIRect>();
	wt->SetColor(0,0,1,1);
	wt->Update();

	WIText *tt = WGUI::Create<WIText>();
	tt->SetText("This is a\ntest text wit\nh a lot of breaks\n 'n shitg");
	tt->Update();
	tt->SizeToContents();
	tt->SetPos(300,300);
	//tt->SetSize(200,500);

	wt->SetPos(*tt->GetPos());
	wt->SetSize(*tt->GetSize());

	FontInfo *font = FontManager::GetDefaultFont();
	for(unsigned int i=32;i<=126;i++)
	{
		GlyphInfo *glyph = font->GetGlyphInfo(i);
		int xMin,yMin,xMax,yMax;
		glyph->GetBounds(&xMin,&yMin,&xMax,&yMax);
		std::cout<<char(i)<<":"<<std::endl;
		std::cout<<"\t"<<xMin<<","<<yMin<<","<<xMax<<","<<yMax<<std::endl;
	}



	static const GLfloat g_vertex_buffer_data[] = { 
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 0.0f,  1.0f, 0.0f,
	};

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
	std::cout<<"Supported version: "<<glGetString(GL_VERSION)<<std::endl;

	WIRect *pBgTest = WGUI::Create<WIRect>();
	pBgTest->SetSize(200,300);
	pBgTest->SetPos(100,100);
	pBgTest->SetColor(0.5,0.5,0,1);

	WIOutlinedRect *pOBgTest = WGUI::Create<WIOutlinedRect>(pBgTest);
	pOBgTest->SetSize(200,300);
	pOBgTest->SetPos(0,0);
	pOBgTest->SetColor(0,1,0,1);
	pOBgTest->SetOutlineWidth(4);
	
	{
		WIBase *pBgBase = WGUI::Create("WIRect");
		WIRect *pBg = dynamic_cast<WIRect*>(pBgBase);
		pBg->SetColor(0,0,0,1);
		pBg->SetSize(800,600);

		WIRect *ttt = WGUI::Create<WIRect>(pBg);
		ttt->SetSize(200,300);
		ttt->SetPos(100,100);
		ttt->SetColor(0.5,0,0,1);

		WILine *pLine = WGUI::Create<WILine>(ttt);
		pLine->SetLineWidth(10);
		pLine->SetColor(1,0,0,1);
		pLine->SetPos(0,0);
		pLine->SetEndPos(100,150);
		pLine->SizeToContents();

		WIOutlinedRect *pO = WGUI::Create<WIOutlinedRect>(ttt);
		pO->SetColor(0,0.5,1,1);
		pO->SetOutlineWidth(6);
		pO->SetPos(0,0);
		pO->SetSize(200,300);

		//WIRect *pLineR = WGUI::Create<WIRect>();
		//pLineR->Initialize();
		//pLineR->SetSize(100,300);
		//pLineR->SetColor(1,0,0,1);
		//pLineR->SetPos(200,100);

		WILine *pLineB = WGUI::Create<WILine>(ttt);
		pLineB->SetLineWidth(10);
		pLineB->SetColor(0,1,0,1);
		pLineB->SetStartPos(100,0);
		pLineB->SetEndPos(200,300);
		pLineB->SizeToContents();
		Vector2i pos,size;
		pLineB->GetPos(&pos.x,&pos.y);
		pLineB->GetSize(&size.x,&size.y);
		//std::cout<<"Bounds: ("<<pos.x<<","<<pos.y<<") ("<<size.x<<","<<size.y<<")"<<std::endl;

		WILine *pLineC = WGUI::Create<WILine>();
		pLineC->SetLineWidth(10);
		pLineC->SetColor(0,0,1,1);
		pLineC->SetPos(200,100);
		pLineC->SetEndPos(400,400);
		pLineC->SizeToContents();
	}
	
	//GRID
	int w = 800;
	int h = 600;
	for(unsigned int i=0;i<=w;i+=100)
	{
		WILine *pLine = WGUI::Create<WILine>();
		pLine->SetLineWidth((i == 400) ? 3 : 1);
		pLine->SetColor(1,1,1,1);
		pLine->SetStartPos(i,0);
		pLine->SetEndPos(i,h);
		pLine->SizeToContents();
		for(unsigned int j=0;j<=h;j+=100)
		{
			WILine *pLine = WGUI::Create<WILine>();
			pLine->SetLineWidth((j == 300) ? 3 : 1);
			pLine->SetColor(1,1,1,1);
			pLine->SetStartPos(0,j);
			pLine->SetEndPos(w,j);
			pLine->SizeToContents();
		}
	}
	//
	//{
	//	WIRect *pRect = WGUI::Create<WIRect>();
	//	pRect->Initialize();
	//	pRect->SetSize(800,600);
	//	pRect->SetColor(0,0,0,1);
	//
	//	WIRect *pBg = WGUI::Create<WIRect>();
	//	pBg->Initialize();
	//	pBg->SetColor(0.5,0,0,1);
	//
	//	WILine *pLine = WGUI::Create<WILine>();
	//	pLine->Initialize();
	//	pLine->SetStartPos(400,300);
	//	pLine->SetEndPos(200,500);
	//	pLine->SizeToContents();
	//	pLine->SetColor(1,1,1,1);
	//
	//	pBg->SetPos(*pLine->GetPos());
	//	pBg->SetSize(*pLine->GetSize());
	//
	//	Vector2i pos,size;
	//	pLine->GetPos(&pos.x,&pos.y);
	//	pLine->GetSize(&size.x,&size.y);
	//	std::cout<<"Bounds: ("<<pos.x<<","<<pos.y<<") ("<<size.x<<","<<size.y<<")"<<std::endl;
	//}

	WIBase *base = WGUI::GetBaseElement();
	while(true)
	{
		OpenGL::GetWindowSize(&width,&height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_NEVER);
		glDisable(GL_DEPTH_TEST);
		//glScalef(0.5f,0.5f,0.5f);
		//gluOrtho2D(0,0,128,128);
		//glOrtho(0,0,0,0,0,0);
		//glViewport(0,0,100,100);
		glEnable(GL_SCISSOR_TEST);
		glScissor(0,0,width,height);
		//c.Draw(width,height);
		/////////////////////////
		GLint shaderID = shaderTest;
		glUseProgram(shaderID);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// Draw the triangle !
	//	glDrawArrays(GL_TRIANGLES, 0, 3); // 3 indices starting at 0 -> 1 triangle

		glDisableVertexAttribArray(0);
		//////////////////////////
		//glScissor(100,0,200,500);
		base->Think();
		if(base->IsVisible())
			base->Draw(width,height);
		//sh.Draw(width,height);
		glDisable(GL_SCISSOR_TEST);
		//c.Draw(data.Width,data.Height);
		//b.Draw(shaderIDB,data.Width,data.Height);
		//glScalef(1.f,1.f,1.f);
		OpenGL::SwapBuffers();
		OpenGL::PollEvents();
		//while(true);
	}
	WGUI::Close();
	return 0;
}*/
#endif
#endif