#include "stdafx.h"

DETECT_MEMORY_LEAK(-1);

// i see there should be synchronization issues...
// ¹Ì·ï¾ßÁö

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
	class AccountManager;
	class ChannelChunk;
	class ClientChunk;

	class AccountManager
	{
		PROPERTY_PROVIDE(AccountManager);

	public:
		class AccountChunk
		{
			PROPERTY_PROVIDE(AccountChunk);

		private:
			AccountChunk(const AccountChunk&){ static_assert(true, "no copy"); }

			DECLARE_PROP_TYPE_R(AccountChunk, std::wstring, UserName, { return UserName.Value; }, { UserName.Value = Value; });
			DECLARE_PROP_TYPE_R(AccountChunk, std::wstring, Password, { return Password.Value; }, { Password.Value = Value; });

		public:
			DECLARE_PROPERTY(UserName);
			DECLARE_PROPERTY(Password);


			inline AccountChunk(const std::wstring& UserName, const std::wstring& Password)
			{
				this->UserName = UserName;
				this->Password = Password;
			}
		};

	private:
		DECLARE_PROP_TYPE_R(AccountManager, (std::map<std::wstring, std::tr1::shared_ptr<AccountChunk>>), ListAccount, { return ListAccount.Value; }, { ListAccount.Value = Value; });

		void SaveToFile()
		{
			auto hf = CreateFile(L"users.txt", GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			ATHROW(hf != INVALID_HANDLE_VALUE);

			for ( auto i=ListAccount.Value.begin(); i != ListAccount.Value.end(); i++ )
			{
				auto& account = i->second;

				DWORD sizestr, dwread;

				sizestr = account->UserName().size();
				WriteFile(hf, &sizestr, sizeof(sizestr), &dwread, NULL);
				WriteFile(hf, account->UserName().data(), sizestr*2, &dwread, NULL);

				sizestr = account->Password().size();
				WriteFile(hf, &sizestr, sizeof(sizestr), &dwread, NULL);
				WriteFile(hf, account->Password().data(), sizestr*2, &dwread, NULL);
			}

			CloseHandle(hf);
		}

	public:
		DECLARE_PROPERTY(ListAccount);

		inline AccountManager()
		{
			auto hf = CreateFile(L"users.txt", GENERIC_READ, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			ATHROW(hf != INVALID_HANDLE_VALUE);

			try
			{
				for ( ;; )
				{
					std::wstring username, password;

					std::vector<wchar_t> buf;
					DWORD sizestr = 0;
					DWORD dwread = 0;
					bool issucceeded = false;
				
					issucceeded = 0 != ReadFile(hf, &sizestr, sizeof(sizestr), &dwread, NULL);
					ATHROW(issucceeded && dwread == sizeof(sizestr));
					buf.resize(sizestr);
					issucceeded = 0 != ReadFile(hf, buf.data(), buf.size()*2, &dwread, NULL);
					ATHROW(issucceeded && dwread == buf.size()*2);
					username = std::wstring(buf.begin(), buf.end());

					issucceeded = 0 != ReadFile(hf, &sizestr, sizeof(sizestr), &dwread, NULL);
					ATHROW(issucceeded && dwread == sizeof(sizestr));
					buf.resize(sizestr);
					issucceeded = 0 != ReadFile(hf, buf.data(), buf.size()*2, &dwread, NULL);
					ATHROW(issucceeded && dwread == buf.size()*2);
					password = std::wstring(buf.begin(), buf.end());

					ListAccount.Value.insert(
						std::pair<std::wstring, std::tr1::shared_ptr<AccountChunk>>(
							username,
							std::tr1::shared_ptr<AccountChunk>(
								new AccountChunk(username,password)
							)
						)
					);
				}
			}
			catch ( std::exception& )
			{

			}

			auto padmin = ListAccount().find(L"admin");
			if ( padmin == ListAccount().end() )
					ListAccount.Value.insert(
						std::pair<std::wstring, std::tr1::shared_ptr<AccountChunk>>(
							L"admin",
							std::tr1::shared_ptr<AccountChunk>(
								new AccountChunk(L"admin",L"passwordofadmin")
							)
						)
					);
			CloseHandle(hf);

			std::wcout << L"total " << ListAccount().size() << L" accounts found." << std::endl;
			for ( auto i=ListAccount().begin(); i != ListAccount().end(); i++ )
				std::wcout << L"\t" << i->second->UserName() << L"\t" << i->second->Password() << std::endl;

			SaveToFile();
		}

		inline ~AccountManager()
		{
			SaveToFile();
		}
	} Account;

	class ChannelChunk
	{
		PROPERTY_PROVIDE(ChannelChunk);
	private:
		std::wstring Name;
		DECLARE_PROP_TYPE_R(ChannelChunk, (std::list<std::tr1::shared_ptr<ClientChunk>>), ListClient, { return ListClient.Value; }, { ListClient.Value = Value; });
		
	public:
		DECLARE_PROPERTY(ListClient);

		inline ChannelChunk(const std::wstring& ChannelName)
		{
			this->Name = ChannelName;
		}

		void AddClient(const std::tr1::shared_ptr<ClientChunk>& Client)
		{
			ListClient.Value.push_back(Client);
		}

		void SubClient(const std::tr1::shared_ptr<ClientChunk>& Client)
		{
			ListClient.Value.remove_if([Client](const std::tr1::shared_ptr<ClientChunk>& A){ return Client == A; });
		}
	};

	class ClientChunk
	{
	public:
		enum STATUS_CODE
		{
			NotStarted,
			Started,
			Identified
		};

	private:
		const Server& Owner;

		SOCKET hSockConnect;
		DWORD IP;
		WORD Port;

		PER_HANDLE_DATA	HandleData;
		PER_IO_DATA		IoBuffer;

		std::queue<char> PacketQueue;

		STATUS_CODE Status;
		
		std::wstring UserName;

		class PacketDivider
		{
			enum PACKET_READ_STATUS_CODE
			{
				WaitForPacketSizeArrive,
				WaitForPacketBufferArrive,
				PacketArrived
			};

			PACKET_READ_STATUS_CODE Status;
			DWORD LastPacketSize;

		public:
			inline PacketDivider():
				Status(WaitForPacketSizeArrive)
			{
			}

			std::vector<char> Process(std::queue<char>& PacketQueue)
			{
				switch ( Status )
				{
				case WaitForPacketSizeArrive:
					if ( PacketQueue.size() >= 4 )
					{
						Status = WaitForPacketBufferArrive;

						DWORD size;
						char* psizeds = (char*)&size;

						for ( DWORD i=0; i < sizeof(size); i++ )
						{
							psizeds[i] = PacketQueue.front();
							PacketQueue.pop();
						}
						LastPacketSize = size;
						
						goto processbuffer;
					}
					break;

				case WaitForPacketBufferArrive:
processbuffer:
					if ( PacketQueue.size() >= LastPacketSize )
					{
						Status = WaitForPacketSizeArrive;

						std::vector<char> retvector(LastPacketSize);
						for ( DWORD i=0; i < LastPacketSize; i++ )
						{
							retvector[i] = PacketQueue.front();
							PacketQueue.pop();
						}
						return retvector;
					}
					break;

				default: NRTHROW();
				}

				return std::vector<char>();
			}
		} PacketProcessor;
	private:
		void Send(const std::string& Buffer)
		{
			std::vector<char> sendbuf;
			sendbuf.resize(sendbuf.size()+4);
			(DWORD&)sendbuf[sendbuf.size()-4] = Buffer.size();
			sendbuf.insert(sendbuf.end(), Buffer.begin(), Buffer.end());

			send(hSockConnect, sendbuf.data(), sendbuf.size(), 0);
		}

		void ProcessPacket()
		{
			auto& packetbuffer = PacketProcessor.Process(PacketQueue);
			if ( !packetbuffer.size() ) return;

			std::strstream sstream(packetbuffer.data(), packetbuffer.size());

			BYTE packetcode;
			ATHROW(sstream.read((char*)&packetcode, 1));

			switch ( Status )
			{
			case Started:
				switch ( packetcode )
				{
					case 0:
						{
							std::wstring username;
							std::wstring password;
							std::vector<wchar_t> buf;
							DWORD strsize;

							ATHROW(sstream.read((char*)&strsize, 4));
							buf.resize(strsize);
							ATHROW(sstream.read((char*)buf.data(), buf.size()*2));
							username = std::wstring(buf.begin(), buf.end());

							ATHROW(sstream.read((char*)&strsize, 4));
							buf.resize(strsize);
							ATHROW(sstream.read((char*)buf.data(), buf.size()*2));
							password = std::wstring(buf.begin(), buf.end());

							auto paccount = Owner.Account.ListAccount().find(username);
							if ( paccount != Owner.Account.ListAccount().end()
								&& paccount->second->Password() == password
								)
							{
								// succeeded
								Status = Identified;
								UserName = username;

								BYTE answercode = 1;
								std::stringstream ss;
								ss.write((char*)&answercode, 1);
								send(hSockConnect, ss.str().data(), ss.str().size(), 0);
							} else
							{
								// failed
								BYTE answercode = 0;
								std::stringstream ss;
								ss.write((char*)&answercode, 1);
								send(hSockConnect, ss.str().data(), ss.str().size(), 0);
							}

						}
						break;
					default: MTHROW(InvalidOperation, "invalid packet on not identified status", );
				}
				break;

			case Identified:
				switch ( packetcode )
				{
					case 0:
						ATHROW(Status == Started);
						Status = Identified;
						break;
				}
				break;
			}

		}

	public:
		inline ClientChunk(const Server& ServerInstance, SOCKET hSockToAssign, DWORD IP, WORD Port):
			Owner(ServerInstance)
		{
			this->Status = NotStarted;
			this->hSockConnect = hSockToAssign;
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
			while ( WSAEWOULDBLOCK == closesocket(hSockConnect) );
		}

		void Start()
		{
			ATHROW(Status == NotStarted);
			Status = Started;

			CreateIoCompletionPort((HANDLE)hSockConnect, Owner.hIoCompletionPort, (DWORD)&HandleData, 0);
			// number of concurrent thread would defaultly be cpu's cores number
			// when work thread got wait status(like Sleep), iocp reports completion to the other threads,
			// so making *2 of threads is fine
			// reference: http://ssb777.blogspot.kr/2009/07/socket-iocp-2.html

			// starts with recv,
			// tcp socket must do one work at once.
			// like recv, send, wait
			DWORD recvedbytes = 0,
				flags = 0;
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
		
		void HandleAsync(const std::vector<char>& Buffer)
		{
			ATHROW(Status >= Started);
			
			std::for_each(Buffer.begin(), Buffer.end(), [this](const char& ch){ this->PacketQueue.push(ch); });

			ProcessPacket();

			DWORD recvedbytes = 0,
				flags = 0;
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

		SOCKADDR_IN adrlisten = { };
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
						DWORD bytestransferred = 0;
						PER_HANDLE_DATA *targetinfo = 0;
						PER_IO_DATA *olbuffer = 0;
						
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

						ATHROW(targetinfo);
						ATHROW(olbuffer);

						try
						{
							auto pclientpair = ListClient().find((DWORD)targetinfo->hSock);
							ATHROW(pclientpair != ListClient().cend());
							auto pclient = pclientpair->second;

							if ( bytestransferred )
							{
								pclient->HandleAsync(std::vector<char>(olbuffer->WsaBuf.buf, olbuffer->WsaBuf.buf+bytestransferred));
							}
							else
							{
								// disconnected, handle for it
								ListClient.Value.erase(pclientpair);
							}
						}
						catch ( std::exception& exception )
						{
							MessageBoxA(0, exception.what(), 0, 0);
							exit(-1);
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
						*this,
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

			pclient->Start();
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

