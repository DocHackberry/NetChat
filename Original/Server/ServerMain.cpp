#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <winbase.h>
#include <list>
using namespace std;

#pragma warning( disable : 4312 )
#pragma warning( disable : 4305 )
#pragma warning( disable : 4309 )

#define MAX_CLIENTS		10
#define MAX_LOADSTRING 100
#define CLIENT 12345
#define CNUMBER 123456
#define STOP 1213
#define RCVBUFSIZE 100   /* Size of receive buffer */
#define MAX_CONN 10
const int kBufferSize = 1024;

struct ClientData
{
	u_char IP[4];
	unsigned short Port;
	HANDLE threadID;
	SOCKET sckSocket;
};

typedef list<ClientData> listClientList;
listClientList ClientList;

#define ID_EDPORT		101
#define ID_EDOUTPUT		102
#define ID_EDINPUT		103
#define ID_BTNSEND		104
#define ID_STACLIENT	105
#define ID_LBXCLIENTS	106
#define ID_BTNSTSVR		107

//Network Code
SOCKET tcpSocket;
struct sockaddr_in SocketServ;
unsigned short echoServPort = 0;     /* Server port */
long NumClients = 0;
DWORD ThreadID;
HANDLE hThread, hUpdate;
TCHAR* bMessage = NULL;

//Window Handles
static HWND hInput;
static HWND hOutput;
static HWND hPort;
static HWND hSendBtn, hStartServer;
static HWND hStatClient, hListClients;

bool isStarted = false;

BOOL				InitTCP(int portNum);
DWORD	WINAPI		ListenForClients(PVOID pvParam);
DWORD	WINAPI		HandleTCPClient(PVOID pvParam);
DWORD	WINAPI		UpdateWindow(PVOID pvParam);
DWORD	WINAPI		BroadcastMessage(PVOID pvParam);
void				ErrorExit(char * ErrorMsg);




LRESULT CALLBACK WndProc	(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

HINSTANCE hInst;

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
					PSTR lpszCmdParam, int nCmdShow)
{
	static char szAppName[] = "Networked Chat Client";
	HWND			hwnd;
	MSG 			msg;
	WNDCLASSEX		wndclass; 
	
	
	wndclass.cbSize 		 = sizeof(wndclass);
	wndclass.style		   = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance	   = hInstance;
	wndclass.hIcon		   = LoadIcon (NULL, IDI_APPLICATION);
	wndclass.hCursor	   = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH); 
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppName;
	wndclass.hIconSm	   = NULL;
	
	RegisterClassEx (&wndclass);
	
	hwnd = CreateWindow (szAppName, 		// window class name
		"Network Chat Server",	 			// window caption
		WS_OVERLAPPEDWINDOW,				// window style
		CW_USEDEFAULT,						// initial x position
		CW_USEDEFAULT,						// initial y position
		640,								// initial x size
		480,								// initial y size
		NULL,								// parent window handle
		NULL,								// window menu handle
		hInstance,							// program instance handle
		NULL);								// creation parameters
	
	ShowWindow (hwnd, nCmdShow);
	UpdateWindow (hwnd);
	hInst = hInstance;

	while (GetMessage (&msg, NULL, 0, 0))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, 
						  WPARAM wParam, LPARAM lParam)
{
	static RECT rect;
	static int cxClient, cyClient;
	static HDC hdc;
	static PAINTSTRUCT ps;
	
	switch (message)
	{
	case WM_CREATE:
		hdc = GetDC(hwnd);
		
		SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));

		hPort = CreateWindow (TEXT("EDIT"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_NOHIDESEL, 
			10,
			10,
			250,
			20,
			hwnd, (HMENU) ID_EDPORT, hInst, NULL);

		hInput = CreateWindow (TEXT("EDIT"), NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_LEFT | ES_NOHIDESEL | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 
			10,
			40,
			450,
			80,
			hwnd, (HMENU) ID_EDINPUT, hInst, NULL);

		hOutput = CreateWindow ("LISTBOX", "ListBox", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_HASSTRINGS, 
			10,
			130,
			450,
			260,
			hwnd, (HMENU) ID_EDOUTPUT, hInst, NULL);

		hStartServer = CreateWindow (TEXT("BUTTON"), "Start Server", WS_CHILD | WS_VISIBLE, 
			270,
			10,
			100,
			20,
			hwnd, (HMENU) ID_BTNSTSVR, hInst, NULL);

		hSendBtn = CreateWindow (TEXT("BUTTON"), "Broadcast", WS_CHILD | WS_VISIBLE,
			470,
			100,
			100,
			20,
			hwnd, (HMENU) ID_BTNSEND, hInst, NULL);

		hStatClient = CreateWindow ("STATIC", "Clients:", WS_CHILD | WS_VISIBLE,
			470,
			130,
			100,
			20,
			hwnd, (HMENU) ID_STACLIENT, hInst, NULL);

		hListClients = CreateWindow ("LISTBOX", "ListBox", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_HASSTRINGS,
			470,
			160,
			100,
			230,
			hwnd, (HMENU) ID_LBXCLIENTS, hInst, NULL);
				
		ReleaseDC(hwnd, hdc);
		
		return 0;
		
	case WM_SIZE:
		rect.right	= LOWORD (lParam) ;
		rect.bottom = HIWORD (lParam) ;
		
		cxClient = LOWORD (lParam);
		cyClient = HIWORD (lParam);
		
		UpdateWindow (hwnd) ;
		return 0 ;
		
			
	case WM_PAINT:
		
		InvalidateRect (hwnd, NULL, TRUE);
		hdc = BeginPaint (hwnd, &ps);
		
		GetWindowRect(hwnd, &rect);
		
		SelectObject (hdc, GetStockObject (SYSTEM_FIXED_FONT));

		EndPaint (hwnd, &ps);
		return 0;
	
	case WM_COMMAND:
		switch (LOWORD (wParam))
		{
			case ID_BTNSEND:
				int length;
				length = (int)SendMessage(hInput, WM_GETTEXTLENGTH, 0, 0);
				if(bMessage)
					delete [] bMessage;
				bMessage = new TCHAR[length + 1];
				SendMessage(hInput, WM_GETTEXT, length + 1, (LPARAM)bMessage);
				BroadcastMessage(NULL);
				SetWindowText(hInput, "");
				
				break;

			case ID_BTNSTSVR:
				if(isStarted)
				{
					//Close Socket
					CloseHandle(hThread);
					CloseHandle(hUpdate);
					SetWindowText(hStartServer, "Start Server");
					isStarted = false;
				}
				else
				{
					int length;
					TCHAR* temp;
					length = (int)SendMessage(hPort, WM_GETTEXTLENGTH, 0, 0);
					temp = new TCHAR[length + 1];
					SendMessage(hPort, WM_GETTEXT, length + 1, (LPARAM)temp);
					int pNum = atoi(temp);
					if(pNum >= 0 && pNum <= 65565)
					{
						InitTCP(pNum);
						{
							hThread = CreateThread(NULL, 0, ListenForClients, NULL, 0, &ThreadID);
							hUpdate = CreateThread(NULL, 0, UpdateWindow, NULL, 0, &ThreadID);
							SetWindowText(hStartServer, "Stop Server");
							isStarted = true;
						}
					}
				}
		
				break;
		}
		return 0;

	case WM_LBUTTONDOWN:				
		return 0;
		
	case WM_RBUTTONDOWN:
		return 0;
		
	case WM_KEYDOWN:
		switch (wParam)
		{
			case VK_RETURN:
				SendMessage(hwnd, WM_COMMAND, ID_BTNSEND, 0);
				break;
				
			case VK_SPACE:
				break;
		}
		return 0;
			
	case WM_PARENTNOTIFY:
		return 0;
		
	case WM_MOVE:
		InvalidateRect (hwnd, NULL, TRUE);
		return 0;

		
	case WM_DESTROY:
		PostQuitMessage (0);
		return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

BOOL InitTCP(int portNum)
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int error = 0;

	wVersionRequested = MAKEWORD(2, 2);

	error = WSAStartup( wVersionRequested, &wsaData );
	if ( error != 0 ) 
	{
		//cout << "Winsock DLL version: " << wVersionRequested << " could not be loaded" << endl;
		//ParseError
		switch(error)
		{
		case WSASYSNOTREADY:
			MessageBox(0, "The underlying network subsystem is not ready for network communication", "Error", MB_SETFOREGROUND);
			break;
		case WSAVERNOTSUPPORTED:
			MessageBox(0, "The version of WinSock support requested is not provided by this WinSock implementation", "Error", MB_SETFOREGROUND);
			break;
		case WSAEINPROGRESS:
			MessageBox(0, "A blocking WinSock 1.1 operation is in progress", "Error", MB_SETFOREGROUND);
			break;
		case WSAEPROCLIM:
			MessageBox(0, "Task limit on this WinSock implementation reached", "Error", MB_SETFOREGROUND);
			break;
		case WSAEFAULT:
			MessageBox(0, "The lpWSAData is not a valid pointer", "Error", MB_SETFOREGROUND);
			break;
		default:
			MessageBox(0, "An Unspecified error has occured in WSAStartup", "Error", MB_SETFOREGROUND);
			break;
		}
		return FALSE;
	}

	/* Confirm that the WinSock DLL supports 2.2.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */

	if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 )
	{
		//cout << "Winsock Version: " << wVersionRequested << " is not supported by this system" << endl;
		//If can't allocate DLL then cleanup and shut down
		error = WSACleanup();
		//Parse Error
		switch(error)
		{
		case WSANOTINITIALISED:
			MessageBox(0, "A successful WSAStartup call must occur before using this function", "Error", MB_SETFOREGROUND);
			break;
		case WSAENETDOWN:
			MessageBox(0, "The network subsystem has failed", "Error", MB_SETFOREGROUND);
			break;
		case WSAEINPROGRESS:
			MessageBox(0, "A blocking 1.1 call in progress or service still processing a callback function", "Error", MB_SETFOREGROUND);
			break;
		default:
			MessageBox(0, "An Unspecified error has occured in WSACleanup", "Error", MB_SETFOREGROUND);
			break;
		}
		return FALSE;
	}
	//Create the socket used for TCP communications
	tcpSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	//Check for any errors
	if( tcpSocket == INVALID_SOCKET)
	{
		switch(WSAGetLastError())
		{
		case WSANOTINITIALISED:
			break;
		case WSAENETDOWN:
			break;
		case WSAEAFNOSUPPORT:
			break;
		case WSAEINPROGRESS:
			break;
		case WSAEMFILE:
			break;
		case WSAENOBUFS:
			break;
		case WSAEPROTONOSUPPORT:
			break;
		case WSAEPROTOTYPE:
			break;
		case WSAESOCKTNOSUPPORT:
			break;
		default:
			break;
		}
		return FALSE;
	}

	/* Construct local address structure */
	memset(&SocketServ, 0, sizeof(SocketServ));		/* Zero out structure */
	SocketServ.sin_family = AF_INET;                /* Internet address family */
	SocketServ.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	SocketServ.sin_port = htons(portNum);			/* Local port */

	error = bind(tcpSocket, (struct sockaddr *) &SocketServ, sizeof(SocketServ));
	if( error == SOCKET_ERROR )
	{
		switch(WSAGetLastError())
		{
		case WSANOTINITIALISED:
			break;
		case WSAENETDOWN:
			break;
		case WSAEACCES:
			break;
		case WSAEADDRINUSE:
			break;
		case WSAEFAULT:
			break;
		case WSAEADDRNOTAVAIL:
			break;
		case WSAEINPROGRESS:
			break;
		case WSAEINVAL:
			break;
		case WSAENOTSOCK:
			break;
		default:
			break;
		}
		return FALSE;
	}
	////Start Server listening for incoming client connections
	//error = listen(tcpSocket, MAX_CONN);

	////Check for error
	//if( error == INVALID_SOCKET )
	//{
	//	ExitProcess(0);
	//}
	return TRUE;
}

DWORD WINAPI ListenForClients(PVOID pvParam)
{
	SOCKET acceptedConnection;
	struct sockaddr_in ClientConn;
	int error = 0, size = 0;
	ClientData Clients;

	//Start Server listening for incoming client connections
	error = listen(tcpSocket, MAX_CONN);

	//Check for error
	if( error == INVALID_SOCKET )
	{
		ExitProcess(0);
	}
	//Loop while waiting for a client connection
	while(1)
	{
		size = sizeof(ClientConn);
		if ((acceptedConnection = accept(tcpSocket, (struct sockaddr *) &ClientConn, &size)) < 0)
			ErrorExit("accept() failed");
		Clients.threadID = CreateThread(NULL, 0, HandleTCPClient, (LPVOID)acceptedConnection, 0,&ThreadID);
		SuspendThread(hUpdate);
		Clients.sckSocket = acceptedConnection;
		Clients.Port = ClientConn.sin_port;
		Clients.IP[0] = ClientConn.sin_addr.S_un.S_un_b.s_b1;
		Clients.IP[1] = ClientConn.sin_addr.S_un.S_un_b.s_b2;
		Clients.IP[2] = ClientConn.sin_addr.S_un.S_un_b.s_b3;
		Clients.IP[3] = ClientConn.sin_addr.S_un.S_un_b.s_b4;
		ClientList.push_back(Clients);
		InterlockedIncrement(&NumClients);
		ResumeThread(hUpdate);
	}
	return 0;
}
DWORD WINAPI HandleTCPClient(PVOID pvParam)
{
    char echoBuffer[RCVBUFSIZE];        /* Buffer for echo string */
    int recvMsgSize;                    /* Size of received message */

	listClientList::iterator it = ClientList.begin();

	/* Clear Input Buffer before use */
	for (int i = 0; i < RCVBUFSIZE; i++)
		echoBuffer[i] = NULL;

    /* Receive message from client */
    if ((recvMsgSize = recv((SOCKET)pvParam, echoBuffer, RCVBUFSIZE, 0)) < 0)
        ErrorExit("recv() failed");

    /* Send received string and receive again until end of transmission */
    while (recvMsgSize > 0)      /* zero indicates end of transmission */
    {
		it = ClientList.begin();
		while( it != ClientList.end())
		{
			/* Echo message back to client */
			int i = 0;
			while (echoBuffer[i] > 0)
				i++;
			echoBuffer[i] = NULL;

			if ((int)send((*it).sckSocket, echoBuffer, strlen(echoBuffer), 0) != strlen(echoBuffer))
				ErrorExit("send() failed");
			it++;
		}
		SendMessage(hOutput, LB_INSERTSTRING, 0, (LPARAM)echoBuffer);

		/* Clear Input Buffer before another use */
		for (int i = 0; i < RCVBUFSIZE; i++)
			echoBuffer[i] = NULL;

        /* See if there is more data to receive */
        recvMsgSize = recv((SOCKET)pvParam, echoBuffer, RCVBUFSIZE, 0);
    }
	InterlockedDecrement(&NumClients);
	it = ClientList.begin();
	while( it != ClientList.end())
	{
		if( (SOCKET)pvParam == (*it).sckSocket)
		{
			ClientList.erase(it);
			it = ClientList.begin();
			if( ClientList.size() == 0)
				continue;
		}
		it++;
	}
    closesocket((int)pvParam);    /* Close client socket */

	return 0;
}
DWORD WINAPI UpdateWindow(PVOID pvParam)
{
	int i = 0;
	char cNumClients[2];
	char tIP[15];
	listClientList::iterator it;

	while(1)
	{
		it = ClientList.begin();
		SendMessage(hListClients, LB_RESETCONTENT, 0, 0);
		itoa(NumClients, cNumClients, 10);
		//SetWindowText(hwndNumber, cNumClients);
		if( NumClients > 0 )
		{
			while( it != ClientList.end())
			{
				sprintf( tIP, ("%i.%i.%i.%i     %i"), (*it).IP[0], (*it).IP[1], (*it).IP[2], (*it).IP[3], (*it).Port);
				SendMessage(hListClients, LB_ADDSTRING, 0, (LPARAM)tIP);
				it++;
			}
		}
		Sleep(200);
	}
	return 0;

}

DWORD WINAPI BroadcastMessage(PVOID pvParam)
{
	char echoBuffer[RCVBUFSIZE];
	strcpy(echoBuffer, bMessage);
	listClientList::iterator it;

 	it = ClientList.begin();
	while( it != ClientList.end())
	{
		if ((int)send((*it).sckSocket, echoBuffer, strlen(echoBuffer), 0) != strlen(echoBuffer))
			ErrorExit("send() failed");
		it++;
	}
	SendMessage(hOutput, LB_INSERTSTRING, 0, (LPARAM)echoBuffer);	
	return 0;
}

void ErrorExit(char * ErrorMsg)
{
	MessageBox(0, ErrorMsg, "Error", MB_SETFOREGROUND);
}
