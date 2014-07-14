/*
* Copyright (C) GTA-Network Team
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*
*     * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following disclaimer
* in the documentation and/or other materials provided with the
* distribution.
*     * Neither the name of GTA-Network nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* INCLUDING NEGLIGENCE OR OTHERWISE ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "CInput.h"
#include <CLogFile.h>
#include <Scripting/CEvents.h>
#include "CServer.h"

extern bool g_bClose;

void CInput::InputThread(CThread* pThread)
{
	CString strInput = GetInput();
	while(pThread->IsRunning())
	{
		ProcessInput(strInput);

		strInput = GetInput();
	}
}

CScriptVehicle * pScriptVehicle = nullptr;
void CInput::ProcessInput(CString strInput)
{
	// Get the command and parameters
	size_t sSplit = strInput.Find(' ', 0);
	CString strCommand = strInput.Substring(0, sSplit++);
	CString strParameters = strInput.Substring(sSplit, (strInput.GetLength() - sSplit));

	if(strCommand.IsEmpty())
		return;
	else if(strCommand == "quit" || strCommand == "Quit" || strCommand == "exit")
	{
		CLogFile::Print("[Server] Server is going to shutdown NOW ....");
		g_bClose = true;
		return;
	} 
	else if(strCommand == "help" || strCommand == "?" || strCommand == "--usage")
	{
		printf("========== Available console commands: ==========\n");
		printf("say <text>\n");
		printf("uptime\n");
		printf("resources\n");
		printf("players\n");
		printf("loadresource <name>\n");
		printf("reloadresource <name>\n");
		printf("unloadresource <name>\n");
		printf("setSyncRate <rate>\n");
		printf("setMaxFPS <fps>\n");
		printf("exit\n");
		return;
	}
	else if (strCommand == "setSyncRate")
	{
		int rate = atoi(strParameters.Get());
		CServer::GetInstance()->SetSyncRate(rate);
	}
	else if (strCommand == "setMaxFPS")
	{
		int fps = atoi(strParameters.Get());
		CServer::GetInstance()->SetMaximumFPS(fps);
	}
	else if (strCommand == "say")
	{
		RakNet::BitStream bitStream;
		bitStream.Write(RakNet::RakString(strParameters.Get()));
		bitStream.Write(0);
		bitStream.Write(true);
		CServer::GetInstance()->GetNetworkModule()->Call(GET_RPC_CODEX(RPC_PLAYER_MESSAGE_TO_ALL), &bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, -1, true);		
	}
	else if (strCommand == "resources")
	{
		for (auto resource : CServer::GetInstance()->GetResourceManager()->GetResources())
		{
			CLogFile::Printf("%s : %s", resource->GetName().C_String(), resource->GetResourceDirectoryPath().C_String());
		}
	}
	else if (strCommand == "loadresource")
	{	
		if (strParameters.IsEmpty()) return;
		if (CServer::GetInstance()->GetResourceManager()->GetResource(strParameters)) return;

		if (CResource* pResource = CServer::GetInstance()->GetResourceManager()->Load(SharedUtility::GetAbsolutePath(CServer::GetInstance()->GetResourceManager()->GetResourceDirectory()), strParameters))
		{
			if (!CServer::GetInstance()->GetResourceManager()->StartResource(pResource))
				CLogFile::Printf("Warning: Failed to load resource %s.", strParameters.Get());
		}
		else
			CLogFile::Printf("Warning: Failed to load resource %s.", strParameters.Get());
	}
	else if (strCommand == "reloadresource")
	{
		if (strParameters.IsEmpty()) return;
		
		CResource * pResource = CServer::GetInstance()->GetResourceManager()->GetResource(strParameters);

		if (pResource == nullptr)
			pResource = CServer::GetInstance()->GetResourceManager()->Load(SharedUtility::GetAbsolutePath(CServer::GetInstance()->GetResourceManager()->GetResourceDirectory()), strParameters);
		else
			CServer::GetInstance()->GetResourceManager()->Reload(pResource);

		if (!CServer::GetInstance()->GetResourceManager()->StartResource(pResource))
				CLogFile::Printf("Warning: Failed to load resource %s.", strParameters.Get());
	}
	else if (strCommand == "unloadresource")
	{
		if (strParameters.IsEmpty()) return;
		if (!CServer::GetInstance()->GetResourceManager()->GetResource(strParameters)) return;

		CServer::GetInstance()->GetResourceManager()->Unload(CServer::GetInstance()->GetResourceManager()->GetResource(strParameters));
	}
	else if (strCommand == "players")
	{
		for (EntityId i = 0; i < CServer::GetInstance()->GetPlayerManager()->GetMax(); ++i)
		{
			if (CServer::GetInstance()->GetPlayerManager()->DoesExists(i))
			{
				CLogFile::Printf("#%i: %s, %s", i, CServer::GetInstance()->GetPlayerManager()->GetAt(i)->GetName().Get(), CServer::GetInstance()->GetPlayerManager()->GetAt(i)->GetSerial().Get());
			}
		}
	}
}


CString CInput::GetInput()
{
	char szInputBuffer[512]; // Buffer MUST be 3 or more (input char, \n, \0)
	CString strInputString;

	// Wait for input from the console
	fgets(szInputBuffer, sizeof(szInputBuffer), stdin);

	// Do we have anything in the input?
	if(szInputBuffer[0] != '\n')
	{
		// Append the input to the input string
		strInputString += szInputBuffer;

		// Get the input string length
		size_t sLength = strInputString.GetLength();

		// Is this the last of the input?
		if(strInputString[sLength - 1] == '\n')
		{
#ifdef _WIN32
			// Replace '\n' with '\0'
			strInputString.Truncate(sLength - 1);
#else
			// Replace '\r\n' with '\0'
			strInputString.Truncate(sLength - 2);
#endif

			// Return the input
			return strInputString;
		}
	}
	return strInputString;
}