// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

class ClientSession
{
	PROPERTY_PROVIDE(ClientSession);
public:
	enum STATUS_CODE
	{
		NotConnected,
		Connected,
		Identified
	};

private:
	DECLARE_PROP_TYPE_R(ClientSession, STATUS_CODE, Status, { return Status.Value; }, { Status.Value = Value; });

	SOCKET hSock;

	void Send(const std::string& Buffer)
	{
		std::vector<char> sendbuf;
		sendbuf.resize(sendbuf.size()+4);
		(DWORD&)sendbuf[sendbuf.size()-4] = Buffer.size();
		sendbuf.insert(sendbuf.end(), Buffer.begin(), Buffer.end());

		send(hSock, sendbuf.data(), sendbuf.size(), 0);
	}

	void Loop()
	{
		for ( ;; )
		{
			switch ( Status )
			{
			case Connected:
				{
					std::wstring usernameinput;
					std::wstring passwordinput;
		
					std::wcout << L"input your username and p/w." << std::endl;
					std::getline(std::wcin, usernameinput);
					std::getline(std::wcin, passwordinput);


					BYTE packetcode = 0;
					DWORD strsize;
					std::stringstream ss;

					ss.write((char*)&packetcode, 1);

					strsize = usernameinput.size();
					ss.write((char*)&strsize, sizeof(strsize));
					ss.write((char*)usernameinput.data(), strsize*2);

					strsize = passwordinput.size();
					ss.write((char*)&strsize, sizeof(strsize));
					ss.write((char*)passwordinput.data(), strsize*2);

					Send(ss.str());

					BYTE answercode = 0;
					recv(hSock, (char*)&answercode, 1, 0);

					if ( answercode )
					{
						Status = Identified;
						std::wcout << L"login succeeded." << std::endl;
					} else
					{
						std::wcout << L"login failed. try again." << std::endl;
					}
				}
				break;

			case Identified:
				{

				}
				break;
			default: MTHROW(InvalidOperation, "unknown status", );
			}


		}
	}

public:
	DECLARE_PROPERTY(Status);

	inline ClientSession()
	{
		hSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	}

	inline ~ClientSession()
	{
		closesocket(hSock);
	}

	void Start()
	{
		ATHROW(Status == NotConnected);
		Status = Connected;

		SOCKADDR_IN addr = { };
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = 0x0100007F;
		addr.sin_port = htons(48486);
		
		connect(hSock, (sockaddr*)&addr, sizeof(addr));

		Loop();
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	std::locale::global(std::locale("kor"));

	WSADATA wsadata;
	WSAStartup(MAKEWORD(2,2), &wsadata);

	ClientSession session;
	session.Start();

	WSACleanup();
	return 0;
}

