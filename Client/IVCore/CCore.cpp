//========== IV:Network - https://github.com/GTA-Network/IV-Network ======================
//
// File: CCore.cpp
// Project: Client.Core
// Author: FRi<FRi.developing@gmail.com>
// License: See LICENSE in root directory
//
//==========================================================================================

#include	"CCore.h"
#include    <IV/CIVScript.h>

extern	CCore			* g_pCore;
bool					g_bDeviceLost = false;

CCore::CCore(void) :
	m_bInitialised(false), m_bGameLoaded(false), m_pGame(NULL),
	m_pGraphics(NULL), m_pChat(NULL), m_pFPSCounter(NULL),
	m_pNetworkManager(NULL), m_pGUI(NULL), m_bLoadingVisibility(0),
	m_byteLoadingStyle(0), m_uiGameInitializeTime(0)//, m_pAudioManager(NULL)
{

}

bool CCore::Initialise()
{
	// Are we already initialsed?
	if(m_bInitialised)
		return false;
	
	PRINT_FUNCTION
	
	// Get the application base address
	m_uiBaseAddress = reinterpret_cast<unsigned int>(GetModuleHandle(NULL));
	
	CLogFile::Printf("Game Base: 0x%p (0x%p)", m_uiBaseAddress, (m_uiBaseAddress - 0x400000));
	
	// Subtract the image size from the base address
	m_uiBaseAddress -= 0x400000;
	
	// Open the settings file
	CSettings::Open(SharedUtility::GetAbsolutePath(CLIENT_SETTINGS_FILE), true, true, true);
	
	// Parse the command line
	CSettings::ParseCommandLine(GetCommandLine());
	
	// Load RAGE Engine
	m_pRAGELibrary = new CLibrary();

	// Did the module fail to load?
	if(m_pRAGELibrary->Load(SharedUtility::GetAbsolutePath("IVEngine.dll")))
	{
		// Get the module function pointers
		m_pEngine = (GetInterface_t)m_pRAGELibrary->GetProcedureAddress("GetInterface");

		if(m_pEngine) {
			RAGEEngineInterface *pTemp = new RAGEEngineInterface;

			// Get the interface
			m_pEngine(pTemp);

			RAGEEngineInterface *m_pInterface = static_cast<RAGEEngineInterface *>(pTemp);

			// Initialise the interface
			m_pInterface->Initialise();
		}
	}
	else {
		// Delete the library instance
		SAFE_DELETE(m_pRAGELibrary);
	}

	// Create the game instance
	m_pGame = new CGame;

	// Create the time instance
	m_pTimeManagement = new CTime;
	
	// Create the graphics instance
	m_pGraphics = new CGraphics;
	
	// Create the fps counter instance
	m_pFPSCounter = new CFPSCounter;
	
	// Create the network manager instance
	m_pNetworkManager = new CNetworkManager;
	
	// Create the development instance
	m_pDevelopment = new CDevelopment;
	
	// Create the chat instance
	m_pChat = new CChat(30, 80);

	m_pIVStartupScript = new CIVStartupScript;
	
	// Unprotect memory before starting addressing
	m_pGame->UnprotectMemory();
	
	// Intialize the offsets instance
	COffsets::Initialize(m_uiBaseAddress);
	
	// Install the XLive hook
	CXLiveHook::Install();
	
	// Install the cursor hook
	//CCursorHook::Install();
	
	// Setup the game instance
	m_pGame->Setup();
	
	// Apply hook's to game
	CHooks::Intialize();
	
	// Patch game addresses
	CPatches::Initialize();

	// Install crash fixes
	CCrashFixes::Initialize();
	
	// Setup the development instance
	m_pDevelopment->SetDebugView(true);
	
	// Get loaded modules from our process
	GetLoadedModulesList();
	
	// Register module manager
	CModuleManager::FetchModules();
	
	EpisodeManager::Initialize();

	// Initialize our HTTP Connection
	m_pHttpClient = new CHttpClient();
	m_pHttpClient->SetRequestTimeout(10000);
	m_pHttpClient->SetHost(MASTERLIST_URL);

	CLogFile::Printf("Done!");
	return true;
}

void CCore::OnGameLoad()
{
	// Is the game already loaded?
	if(IsGameLoaded())
		return;

	CLogFile::Print(__FUNCTION__);

	// Initialize game instance & respawn local player
	GetGame()->Initialise();

	// Mark the game as loaded
	SetGameLoaded(true);

	// Startup the network module
	m_pNetworkManager->Startup();
	
	// Prepare the client in game elements
	g_pCore->GetGame()->PrepareWorld();

	// Finalize the client in game elements
	g_pCore->GetGame()->OnClientReadyToGamePlay();

	// Set the main menu visible
	m_pMainMenu->SetVisible(true);

	// Set the initialize time
	m_uiGameInitializeTime = timeGetTime(); 
}

void CCore::OnGameUpdate()
{
	if (g_pCore->GetNetworkManager())
		g_pCore->GetNetworkManager()->Pulse();

	g_pCore->GetTimeManagementInstance()->Pulse();

	g_pCore->GetGame()->ProcessEnvironment();

	if (g_pCore->GetFPSCounter())
		g_pCore->GetFPSCounter()->Pulse();

	if (g_pCore->GetGame()->GetLocalPlayer())
		g_pCore->GetGame()->GetLocalPlayer()->Pulse();
}

void CCore::ConnectToServer(CString strHost, unsigned short usPort, CString strPass)
{
	SetHost(strHost);
	SetClientPort(usPort);
	SetPass(strPass);

	// Connect to the network
	m_pNetworkManager->Connect(GetHost(), (unsigned short) GetPort(), GetPass());
}

void CCore::OnDeviceCreate(IDirect3DDevice9 * pDevice, D3DPRESENT_PARAMETERS * pPresentationParameters)
{
	PRINT_FUNCTION

	// Setup the chat
	if(m_pChat)
		m_pChat->Setup(pPresentationParameters);

	// Setup audio manager
	m_pAudioManager = new CAudioManager();

	if (!m_pAudioManager->Initialize())
		CLogFile::Printf("CAudioManager::Initialize failed");

	m_pGUI = new CGUI(pDevice);
	m_pGUI->Initialize();

	// Initialize the main menu elements
	m_pMainMenu = new CMainMenu(m_pGUI);
	m_pMainMenu->Initialize();
}

void CCore::OnDeviceLost(IDirect3DDevice9 * pDevice)
{
	PRINT_FUNCTION

	// Mark as lost device
	g_bDeviceLost = true;
}

void CCore::OnDeviceReset(IDirect3DDevice9 * pDevice, D3DPRESENT_PARAMETERS * pPresentationParameters)
{
	PRINT_FUNCTION

	// Mark as not lost device
	g_bDeviceLost = false;
}

void CCore::OnDeviceRender(IDirect3DDevice9 * pDevice)
{
	// Has the device been lost?
	if(g_bDeviceLost || !m_pGraphics)
		return;

	// Render our chat instance
	if (m_pChat && m_pChat->IsVisible())
		m_pChat->Render();

	// Render our gui instance
	if (m_pGUI)
		m_pGUI->Render();

	// Print our IVNetwork "Identifier" in the left upper corner
	unsigned short usPing = m_pNetworkManager != NULL ? (m_pNetworkManager->IsConnected() ? (g_pCore->GetGame()->GetLocalPlayer() ? g_pCore->GetGame()->GetLocalPlayer()->GetPing() : -1) : -1) : -1;

	CString strConnection;
	int iConnectTime = GetGameLoadInitializeTime() != 0 ? (int)((timeGetTime() - GetGameLoadInitializeTime()) / 1000) : 0;

	CString strSeconds;
	if (iConnectTime > 0)
	{
		strSeconds.AppendF(" | Running since ");

		int iSeconds = iConnectTime % 60;
		int iMinutes = (iConnectTime / 60) % 60;
		int iHours = (iConnectTime / 60 / 60) % 24;
		int iDays = (iConnectTime / 60 / 60 / 24);

		if(iDays > 0)
		{
			if(iDays > 9)
				strSeconds.AppendF("%d Days, ", iDays);
			else
				strSeconds.AppendF("%d Day%s, ", iDays, iDays > 1 ? "s" : "");
		}
		if(iHours > 0)
		{
			if(iHours > 9)
				strSeconds.AppendF("%d Hours, ",iHours);
			else
				strSeconds.AppendF("0%d Hour%s, ",iHours, iHours > 1 ? "s" : "");
		}
		if(iMinutes > 0)
		{
			if(iMinutes > 9)
				strSeconds.AppendF("%d Minutes, ",iMinutes);
			else
				strSeconds.AppendF("0%d Minute%s, ",iMinutes, iMinutes > 1 ? "s" : "");
		}

		if (iSeconds > 9)
			strSeconds.AppendF("%d Seconds", iSeconds);
		else
			strSeconds.AppendF("0%d Second%s", iSeconds, iSeconds > 1 ? "s" : "");

	}

	// Simulate temporary loading symbol
	m_byteLoadingStyle++;
	CString strLoadingInformation;
	if(m_byteLoadingStyle >= 0 && m_byteLoadingStyle < 10)
		strLoadingInformation = CString(MOD_NAME " " VERSION_IDENTIFIER " - Loading.. Hold on .").Get();
	else if(m_byteLoadingStyle >= 10 && m_byteLoadingStyle < 20)
		strLoadingInformation = CString(MOD_NAME " " VERSION_IDENTIFIER " - Loading.. Hold on ..").Get();
	else if(m_byteLoadingStyle >= 20 && m_byteLoadingStyle < 30)
		strLoadingInformation = CString(MOD_NAME " " VERSION_IDENTIFIER " - Loading.. Hold on ...").Get();
	else if(m_byteLoadingStyle >= 30 && m_byteLoadingStyle < 40)
		strLoadingInformation = CString(MOD_NAME " " VERSION_IDENTIFIER " - Loading.. Hold on ....").Get();
	else if (m_byteLoadingStyle >= 40 && m_byteLoadingStyle < 50)
	{
		strLoadingInformation = CString(MOD_NAME " " VERSION_IDENTIFIER " - Loading.. Hold on ....").Get();
		if (m_byteLoadingStyle == 49)
			m_byteLoadingStyle = 0;
	}

	CString strInformation = usPing == 0xFFFF ? CString("%s%s", MOD_NAME " " VERSION_IDENTIFIER, strSeconds.Get()) : CString("%s%s | Ping %hu", MOD_NAME " " VERSION_IDENTIFIER, strSeconds.Get(), usPing);
	
	if(!g_pCore->GetGame()->GetLocalPlayer())
		m_pGraphics->DrawText(60.0f, 5.0f, D3DCOLOR_ARGB(255, 0, 195, 255), 1.0f, 5, DT_NOCLIP, (bool)true, strLoadingInformation.Get());
	else
		m_pGraphics->DrawText(60.0f, 5.0f, D3DCOLOR_ARGB(255, 0, 195, 255), 1.0f, 5, DT_NOCLIP, (bool)true, strInformation.Get());
	
	strSeconds.Clear();
	strLoadingInformation.Clear();
	strInformation.Clear();

	// Before rendering FPS-Counter instance, update FPS display
	m_pGraphics->DrawText(5.0f, 5.0f, D3DCOLOR_ARGB((unsigned char)255, 255, 255, 255), 1.0f, 5, DT_NOCLIP, (bool)true, CString("FPS: %d", m_pFPSCounter->GetFPS()).Get());

#ifdef _DEBUG
	// Render our development instance
	if(m_pDevelopment)
		m_pDevelopment->Process();
#endif

	// Check if our snap shot write failed
	if(CSnapShot::IsDone())
	{
		if(CSnapShot::HasSucceeded())
			m_pChat->Outputf(false, "Screen shot written (%s).", CSnapShot::GetWriteName().Get());
		else
			m_pChat->Outputf(false, "Screen shot write failed (%s).", CSnapShot::GetError().Get());

		CSnapShot::Reset();
	}
}

void CCore::GetLoadedModulesList()
{
	DWORD aProcesses[1024]; 
    DWORD cbNeeded; 
    DWORD cProcesses;
    unsigned int i;

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
        return;

    cProcesses = cbNeeded / sizeof(DWORD);

    for ( i = 0; i < cProcesses; i++ )
    {
		if(aProcesses[i] == GetProcessId(GetCurrentProcess()))
			GetLoadedModule( aProcesses[i] );
    }
}

void CCore::GetLoadedModule(DWORD dwProcessId)
{
	HMODULE hMods[1024];
    HANDLE hProcess;
    DWORD cbNeeded;
    unsigned int i;

    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                            PROCESS_VM_READ,
                            FALSE, dwProcessId );
    if (NULL == hProcess)
        return;

    if( EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for ( i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ ) {
            TCHAR szModName[MAX_PATH];
            if ( GetModuleFileNameExA(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
			  CString strModule;

			  std::string strModulee;
			  strModulee.append(szModName);
			  std::size_t found = strModulee.find("SYSTEM32",10);
			  std::size_t found2 = strModulee.find("system32",10);
			  std::size_t found3 = strModulee.find("AppPatch",10);
			  std::size_t found4 = strModulee.find("WinSxS", 10);

			  if(found!=std::string::npos || found2!=std::string::npos || found3!=std::string::npos || found4!=std::string::npos)
			  {
				//strModule.AppendF("--> %s", szModName);
				//CLogFile::Printf("  %s (0x%08X)",strModule.Get(), hMods[i] ); 
			  }//;//strModule.AppendF("%i, %s",found, szModName);
			  else {
				strModule.AppendF("--> IVModules: %s", szModName);
				CLogFile::Printf("  %s (0x%08X)",strModule.Get(), hMods[i] );
			  }
            }
        }
    }
    CloseHandle(hProcess);
    return;
}

void CCore::OnNetworkShutDown()
{
	// Call destructor of cgame
	g_pCore->GetGame()->~CGame();
}

void CCore::OnNetworkTimeout()
{
	// Call reinitialise cgame
	g_pCore->GetGame()->Reset();
}

void CCore::DumpVFTable(DWORD dwAddress, int iFunctionCount)
{
	CLogFile::Printf("Dumping Virtual Function Table at 0x%p...",dwAddress);

	for(int i = 0; i < iFunctionCount; i++)
		CLogFile::Printf("VFTable Offset: %d, Function: 0x%p (At Address: 0x%p)", (i * 4), *(PDWORD)(dwAddress + (i * 4)), (dwAddress + (i * 4)));
}