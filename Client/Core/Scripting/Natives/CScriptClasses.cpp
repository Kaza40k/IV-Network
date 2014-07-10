//========= IV: Network - https://github.com/GTA-Network/IV-Network ============
//
// File: CScriptClasses.cpp
// Project: Client.Core
// Authors: xForce <xf0rc3.11@gmail.com>, g3o0or
// License: See LICENSE in root directory
//
//==============================================================================

#include "CScriptClasses.h"
#include <Game/EFLC/CScript.h>
#include <CCore.h>
#include <Scripting/CLuaVM.h>
#include <Scripting/CSquirrelVM.h>
#include <CLogFile.h>
#include <Scripting/ResourceSystem/CResourceManager.h>
#include "Scripting/Natives/Natives_Client.h"

extern CCore * g_pCore;

int SendMessage(int * VM)
{
	GET_SCRIPT_VM_SAFE;	

	CString sMessage;
	DWORD dwColor;
	bool bAllowFormatting;
	
	pVM->Pop(sMessage);
	pVM->Pop(dwColor);
	pVM->Pop(bAllowFormatting);
	
	g_pCore->GetGraphics()->GetChat()->Print(CString("#%x%s", dwColor, sMessage.C_String()));

	pVM->ResetStackIndex();

	return 1;
}

int GetLocalPlayer(int * VM)
{
	GET_SCRIPT_VM_SAFE;

	pVM->PushInstance("CPlayerEntity", g_pCore->GetGame()->GetLocalPlayer());

	pVM->ResetStackIndex();

	return 1;
}

int TriggerServerEvent(int * VM)
{
	GET_SCRIPT_VM_SAFE;

	CString eventName;

	pVM->Pop(eventName);

	RakNet::BitStream bitStream;
	bitStream.Write(eventName);
	g_pCore->GetNetworkManager()->Call(GET_RPC_CODEX(RPC_PLAYER_TRIGGER_EVENT), &bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, true);

	pVM->ResetStackIndex();

	return 1;
}

int IsChatVisible(int * VM)
{
	GET_SCRIPT_VM_SAFE;

	pVM->Push(g_pCore->GetGraphics()->GetChat()->IsVisible());

	pVM->ResetStackIndex();

	return 1;
}

int SetChatVisible(int * VM)
{
	GET_SCRIPT_VM_SAFE;

	bool bVisible;

	pVM->Pop(bVisible);

	g_pCore->GetGraphics()->GetChat()->SetVisible(bVisible);

	pVM->ResetStackIndex();

	return 1;
}

int IsChatInputVisible(int * VM)
{
	GET_SCRIPT_VM_SAFE;

	bool bVisible;

	
	pVM->Push(g_pCore->GetGraphics()->GetChat()->IsInputVisible());

	pVM->ResetStackIndex();

	return 1;
}

int IsMainMenuVisible(int * VM)
{
	GET_SCRIPT_VM_SAFE;

	pVM->Push(g_pCore->GetGraphics()->GetMainMenu()->IsMainMenuVisible());

	pVM->ResetStackIndex();

	return 1;
}

void CScriptClasses::Register(IScriptVM * pVM)
{
	pVM->RegisterFunction("getLocalPlayer", GetLocalPlayer);
	pVM->RegisterFunction("sendMessage", SendMessage);
	pVM->RegisterFunction("triggerServerEvent", TriggerServerEvent);
	pVM->RegisterFunction("isChatVisible", IsChatVisible);
	pVM->RegisterFunction("setChatVisible", SetChatVisible);
	pVM->RegisterFunction("isChatInputVisible", IsChatInputVisible);
	pVM->RegisterFunction("isMainMenuVisible", IsMainMenuVisible);
}
