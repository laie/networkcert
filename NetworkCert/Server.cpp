#include "stdafx.h"

DETECT_MEMORY_LEAK(-1);

class Server
{
	PROPERTY_PROVIDE(Server);
private:
	typedef struct
	{
		SOCKET hSock;
		SOCKADDR_IN Addr;
	} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

	typedef struct
	{
		static const DWORD BufferSize = 4096;
		OVERLAPPED Overlapped;
		char Buffer[BufferSize];
		WSABUF WsaBuf;
	} PER_IO_DATA, *LPPER_IO_DATA;

public:
	class ClientChunk
	{
	private:
		bool IsStarted;
		SOCKET hSockConnect;
		DWORD IP;
		WORD Port;

		PER_HANDLE_DATA	HandleData;
		PER_IO_DATA		IoBuffer;
		
	public:
		inline ClientChunk(SOCKET hSockToAssign, DWORD IP, WORD Port)
		{
			this->IsStarted = false;
			this->hSockConnect = HandleData.hSock;
			this->IP = IP;
			this->Port = Port;

			SOCKADDR_IN addr = { };
			addr.sin_family			= AF_INET;
			addr.sin_addr.s_addr	= htonl(this->IP);
			addr.sin_port			= htons(this->Port);

			PER_HANDLE_DATA handledata = { this->hSockConnect, addr };
			this->HandleData = handledata;

			memset(&IoBuffer, 0, sizeof(IoBuffer));
			IoBuffer.WsaBuf.len = PER_IO_DATA::BufferSize;
			IoBuffer.WsaBuf.buf = IoBuffer.Buffer;
		}

		inline ~ClientChunk()
		{
			closesocket(hSockConnect);
		}

		void Start(HANDLE hIoCompletionPort)
		{
			IsStarted = true;
			DWORD recvedbytes = 0,
				flags = 0;
			CreateIoCompletionPort((HANDLE)hSockConnect, hIoCompletionPort, (DWORD)&HandleData, 0);
			WSARecv(
				hSockConnect,
				&IoBuffer.WsaBuf,
				1,
				&recvedbytes,
				&flags,
				&IoBuffer.Overlapped,
				NULL
				);
		}

		void Handle(const std::vector<char>& Buffer)
		{
			ATHROW(IsStarted);
		}
	};

	enum STATUS_CODE
	{
		NotStarted,
		Started
	};

private:
	DECLARE_PROP_TYPE_R(Server, (std::map<DWORD, std::tr1::shared_ptr<ClientChunk>>), ListClient, { return ListClient.Value; }, { ListClient.Value = Value; });
	DECLARE_PROP_TYPE_R(Server, HANDLE, hIoCompletionPort, { return hIoCompletionPort.Value; }, { hIoCompletionPort.Value = Value; });
	DECLARE_PROP_TYPE_R(Server, STATUS_CODE, Status, { return Status.Value; }, { Status.Value = Value; });

	SOCKET hSockListen;
	DWORD ProcessorNumber;
	std::list<std::thread> ListThread;

	void Uninit()
	{
		ATHROW(Status >= Started);
		closesocket(hSockListen);
	}

public:
	DECLARE_PROPERTY(ListClient);
	DECLARE_PROPERTY(hIoCompletionPort);
	DECLARE_PROPERTY(Status);

	inline Server()
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		ProcessorNumber = si.dwNumberOfProcessors;
		
		hIoCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

		ListThread.resize(ProcessorNumber*2);
	}

	inline ~Server()
	{
		if ( Status >= Started )
			Uninit();
		CloseHandle(hIoCompletionPort);
	}

	void Start()
	{
		ATHROW(Status == NotStarted);

		Status = Started;
		std::wcout << L"starting server... ";

		hSockListen = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		ATHROW(hSockListen != INVALID_SOCKET);

		SOCKADDR_IN adrlisten;
		adrlisten.sin_family = AF_INET;
		adrlisten.sin_addr.s_addr = htonl(INADDR_ANY);
		adrlisten.sin_port = htons(48486);
		ATHROW( 0 == bind(hSockListen, (SOCKADDR*)&adrlisten, sizeof(adrlisten)) );
		ATHROW( 0 == listen(hSockListen, SOMAXCONN) );

		for ( auto i=ListThread.begin(); i != ListThread.end(); i++ )
		{
			*i = std::thread
				(
					[this]()
					{
						DWORD bytestransferred;
						PER_HANDLE_DATA targetinfo;
						PER_IO_DATA olbuffer;
						
						bool issucceeded =
							FALSE !=
							GetQueuedCompletionStatus
							(
								this->hIoCompletionPort,
								&bytestransferred,
								(PULONG_PTR)&targetinfo,
								(LPOVERLAPPED*)&olbuffer,
								INFINITE
							);



						auto pclientpair = ListClient().find((DWORD)targetinfo.hSock);
						ATHROW(pclientpair != ListClient().cend());
						auto pclient = pclientpair->second;
						pclient->Handle(std::vector<char>(olbuffer.WsaBuf.buf, olbuffer.WsaBuf.buf+olbuffer.WsaBuf.len));

						if ( bytestransferred )
						{

						}
						else
						{
							// disconnected, handle for it
							ListClient.Value.erase(pclientpair);
						}
					}
				);
		}
		
		for ( ;; )
		{
			SOCKADDR_IN adrclient;
			SOCKET hsockclient;
			int lenadr = sizeof(adrclient);

			hsockclient = accept(hSockListen, (SOCKADDR*)&adrclient, &lenadr);
			ATHROW(hsockclient != INVALID_SOCKET);
			
			std::tr1::shared_ptr<ClientChunk> pclient(
					new ClientChunk(
						hsockclient,
						ntohl(adrclient.sin_addr.s_addr),
						ntohs(adrclient.sin_port)
					)
				);
			ListClient.Value.insert(std::map<DWORD, std::tr1::shared_ptr<ClientChunk>>::value_type(
				(DWORD)hsockclient,
				pclient
				)
			);

			pclient->Start(hIoCompletionPort);
		}
	}

	
};

int _tmain(int argc, _TCHAR* argv[])
{
	std::locale::global(std::locale("kor"));
	WSADATA wsadata;
	if ( 0 != WSAStartup(MAKEWORD(2,2), &wsadata) )
	{
		// failed
		return -1;
	}

	Server server;
	server.Start();

	WSACleanup();
	return 0;
}

