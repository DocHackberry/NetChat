#include <winsock2.h>
#include <windows.h>


#define ID_EDSERVER		101
#define ID_EDPORT		102
#define ID_EDOUTPUT		103
#define ID_EDINPUT		104
#define ID_BTNCONNECT	105
#define ID_BTNSEND		106

#define ID_LIST			1
#define CONNECT			11111
#define CLOSE			22222
#define SEND			33333
#define RCVBUFSIZE		100   /* Size of receive buffer */
#define MAX_LOADSTRING	100


// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
//Network Code
SOCKET tcpSocket;
unsigned short echoServPort;     /* Server port */
struct sockaddr_in SocketServ;

HANDLE hListen;
DWORD ThreadID = 0;

char ServerName[50], TextMessage[100];
int  PortNumber, TextLength;

LRESULT CALLBACK WndProc	(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL				InitTCP();
DWORD	WINAPI		GetData( PVOID pvParam );


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
		"Network Chat Client", 			// window caption
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
	static HWND hInput;
	static HWND hOutput;
	static HWND hServer, hPort;
	static HWND hSendBtn, hConnectBtn;
	static RECT rect;
	static int cxClient, cyClient;
	static HDC hdc;
	static PAINTSTRUCT ps;
	static bool	isConnected;
	char inputText[50];
	int len = 0;

	switch (message)
	{
	case WM_CREATE:
		hdc = GetDC(hwnd);
		
		SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));

		hServer = CreateWindow (TEXT("EDIT"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_NOHIDESEL, 
			10,
			10,
			250,
			20,
			hwnd, (HMENU) ID_EDSERVER, hInst, NULL);

		hPort = CreateWindow (TEXT("EDIT"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_NOHIDESEL, 
			270,
			10,
			180,
			20,
			hwnd, (HMENU) ID_EDPORT, hInst, NULL);

		hInput = CreateWindow (TEXT("EDIT"), NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_LEFT | ES_NOHIDESEL | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 
			10,
			40,
			450,
			180,
			hwnd, (HMENU) ID_EDINPUT, hInst, NULL);

		hOutput = CreateWindow ("LISTBOX", "ListBox", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_HASSTRINGS,
			10,
			230,
			450,
			180,
			hwnd, (HMENU) ID_EDOUTPUT, hInst, NULL);

		hConnectBtn = CreateWindow (TEXT("BUTTON"), "Connect", WS_CHILD | WS_VISIBLE, 
			470,
			10,
			100,
			20,
			hwnd, (HMENU) ID_BTNCONNECT, hInst, NULL);

		hSendBtn = CreateWindow (TEXT("BUTTON"), "Send", WS_CHILD | WS_VISIBLE,
			470,
			230,
			100,
			20,
			hwnd, (HMENU) ID_BTNSEND, hInst, NULL);
				
		ReleaseDC(hwnd, hdc);
		InitTCP();
		isConnected = false;
		
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
			case ID_BTNCONNECT:
				GetWindowText(hServer, ServerName, 50);
				GetWindowText(hPort, inputText, 5);
				PortNumber = atoi(inputText);
				if(!isConnected)
				{
					/* Construct the server address structure */
					memset(&SocketServ, 0, sizeof(SocketServ));				/* Zero out structure */
					SocketServ.sin_family      = AF_INET;					/* Internet address family */
					SocketServ.sin_addr.s_addr = inet_addr(ServerName);		/* Server IP address */
					SocketServ.sin_port        = htons(PortNumber);			/* Server port */

					/* Establish the connection to the echo server */
					if (connect(tcpSocket, (struct sockaddr *) &SocketServ, sizeof(SocketServ)) < 0)
						MessageBox(0, "Connection to server failed", "Error", MB_SETFOREGROUND);
					else
					{
						hListen = CreateThread(NULL, 0, GetData, (LPVOID)hOutput, 0, &ThreadID);
						MessageBox(0, "Connected to server", "Success", MB_SETFOREGROUND);
						isConnected = true;
						SetWindowText(hConnectBtn, "Disconnect");
					}
				}
				else
				{
					isConnected = false;
					SetWindowText(hConnectBtn, "Connect");
				}
				break;
			case ID_BTNSEND:
				if(isConnected)
				{
					len = GetWindowTextLength(hInput);
					TextLength = GetWindowText(hInput, TextMessage, len + 1);
					if (send(tcpSocket, TextMessage, TextLength, 0) != TextLength)
						MessageBox(0, "send() sent a different number of bytes than expected", "Error", MB_SETFOREGROUND);
					//Test Data
					Sleep(100);
					//GetData((PVOID)hOutput);
					SetWindowText(hInput, "");
				}
				else
					MessageBox(0, "Client Not Connected", "Error", MB_SETFOREGROUND);
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

BOOL InitTCP()
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

	return TRUE;
}
DWORD WINAPI GetData(PVOID pvParam)
{
	static TCHAR TextMessage[100];

	while(1)
	{
		memset((void*)TextMessage, '\0', RCVBUFSIZE);
		if ((recv(tcpSocket, TextMessage, RCVBUFSIZE, 0)) > 0)
		{
			SendMessage((HWND)pvParam, LB_INSERTSTRING, 0, (LPARAM)TextMessage);
		}
		Sleep(200);
	}
	return 0;
}

