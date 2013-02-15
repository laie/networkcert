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
	std::list<std::tr1::function<void ()>> ListInvokeProcedures;
	std::thread commandthread;

	Util::CriticalSection CsInvoke;

	void Send(const std::string& Buffer)
	{
		std::stringstream ss;
		DWORD sizepacket = Buffer.size();
		ss.write((char*)&sizepacket, sizeof(sizepacket));
		ss.write(Buffer.data(), Buffer.size());

		send(hSock, ss.str().data(), ss.str().size(), 0);
	}

	std::vector<char> Recv()
	{
		DWORD packetsize = 0;
		recv(hSock, (char*)&packetsize, sizeof(packetsize), 0);
		if ( packetsize )
		{
			std::vector<char> buf(packetsize);
			recv(hSock, buf.data(), buf.size(), 0);
			return buf;
		} else return std::vector<char>();
	}

	void Loop()
	{
		for ( ;; )
		{
			if ( !ListInvokeProcedures.empty() )
			{
				std::list<std::tr1::function<void ()>> listtmpinvoke;

				{
					Util::LockBlock lblk(CsInvoke);
					for ( auto i=ListInvokeProcedures.begin(); i != ListInvokeProcedures.end(); )
					{
						listtmpinvoke.push_back(*i);
						ListInvokeProcedures.erase(i++);
					}
				}

				for ( auto i=listtmpinvoke.begin(); i != listtmpinvoke.end(); i++ )
					(*i)();

			} else Sleep(1);

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

	void Say(const std::wstring& Message)
	{
		std::ostringstream ss;
		
		BYTE packetcode = 1;
		DWORD sizestr = Message.size();

		ss.write((char*)&packetcode, sizeof(packetcode));
		ss.write((char*)&sizestr, sizeof(sizestr));
		ss.write((char*)Message.data(), Message.size()*2);

		Send(ss.str());
	}

	void MoveChannel(const std::wstring& ChannelName)
	{
		std::ostringstream ss;
		
		BYTE packetcode = 2;
		DWORD sizestr = ChannelName.size();


		ss.write((char*)&packetcode, sizeof(packetcode));
		ss.write((char*)&sizestr, sizeof(sizestr));
		ss.write((char*)ChannelName.data(), ChannelName.size()*2);

		Send(ss.str());
	}

	void DestroyChannel()
	{
		std::ostringstream ss;

		BYTE packetcode = 3;
		
		ss.write((char*)&packetcode, sizeof(packetcode));
		Send(ss.str());
	}

	void ShowList()
	{
		std::ostringstream ss;

		BYTE packetcode = 4;
		
		ss.write((char*)&packetcode, sizeof(packetcode));
		Send(ss.str());
	}

	template<typename T>
	void Invoke(const T& Functor)
	{
		Util::LockBlock lbk(CsInvoke);
		ListInvokeProcedures.push_back(std::tr1::function<void ()>(Functor));
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

		// i know that this looks nasty and should not be done like,
		// but you must know that this is very dirty and hard work without boost :(
		std::thread threadcommunicate(
			[this]()
			{
				for ( ;; )
				{
					auto& recvbuf = this->Recv();
					this->Invoke(
						[this, recvbuf]() mutable
						{
							std::strstream rs(recvbuf.data(), recvbuf.size());
							
							BYTE packetcode;
							rs.read((char*)&packetcode, sizeof(packetcode));

							switch ( this->Status() )
							{
							case Connected:
								switch ( packetcode )
								{
								case 0:
									{
										BYTE code=0xff;
										rs.read((char*)&code, sizeof(code));
										switch ( code )
										{
										case 0:
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
											}
											break;
										case 1:
											{
												bool isloginsucceeded=false;
												rs.read((char*)&isloginsucceeded, sizeof(isloginsucceeded));
												if ( isloginsucceeded )
												{
													commandthread =
														std::thread
															(
															[this]()
															{
																for ( ;; )
																{
																	std::wstring commandstr;
																	std::getline(std::wcin, commandstr);

																	std::wistringstream iss(commandstr);

																	std::wstring commandtype;
																	iss >> commandtype;
																	if ( commandtype == L"chat" )
																	{
																		std::wstring chatmessage;
																		chatmessage.resize(iss.str().size() - (DWORD)iss.tellg());
																		iss.read((wchar_t*)chatmessage.data(), chatmessage.size());

																		Say(chatmessage);
																	} else if ( commandtype == L"move" )
																	{
																		std::wstring channelname;
																		iss >> channelname;
																		MoveChannel(channelname);
																	} else if ( commandtype == L"destroychannel" )
																	{
																		DestroyChannel();
																	} else if ( commandtype == L"showlist" )
																	{
																		ShowList();
																	} else
																	{
																		std::wcout << L"invalid command" << std::endl;
																	}

																	//std::copy(std::istream_iterator<std::string>(iss),
																			// std::istream_iterator<std::string>(),
																			 //std::ostream_iterator<std::string>(std::cout, "\n"));
																}
				
															}
															);
														
													Status = Identified;
													std::wcout << L"login succeeded." << std::endl;
												}
												else std::wcout << L"login failed. try again." << std::endl;
											}
											break;
										default: NRTHROW();
										}
									}

									break;
								}
								break;

							case Identified:
								{
									switch ( packetcode )
									{
									case 1:
										{
											// channel message
											DWORD sizestr;
											std::wstring channel,
												message;
										
											sizestr = 0;
											rs.read((char*)&sizestr, sizeof(sizestr));
											channel.resize(sizestr);
											rs.read((char*)channel.data(), channel.size()*2);
										
											sizestr = 0;
											rs.read((char*)&sizestr, sizeof(sizestr));
											message.resize(sizestr);
											rs.read((char*)message.data(), message.size()*2);
											
											std::wcout << L"[" + channel + L"] " + message << std::endl;
										}
										break;
									case 2:
										{
											// system message
											DWORD sizestr;
											std::wstring message;
										
											sizestr = 0;
											rs.read((char*)&sizestr, sizeof(sizestr));
											message.resize(sizestr);
											rs.read((char*)message.data(), message.size()*2);
											
											std::wcout << message << std::endl;
										}
										break;
									case 3:

										break;
									case 4:
										{
											// show list
											DWORD countbuf = 0;
											rs.read((char*)&countbuf, sizeof(countbuf));

											std::wcout << L"-- users list --" << std::endl;
											for ( DWORD i=0; i < countbuf; i++ )
											{
												DWORD sizestr = 0;
												std::wstring username;
												rs.read((char*)&sizestr, sizeof(sizestr));
												username.resize(sizestr);
												rs.read((char*)username.data(), username.size()*2);
												std::wcout << L"\t" << username << std::endl;
											}
											std::wcout << L"-- end list --" << std::endl;
										}
										break;
									default: MTHROW(InvalidOperation, "Invalid packet code", );

									}
								}
								break;
							default: MTHROW(InvalidOperation, "unknown status", );
							}
						});
				}
			}
		);
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

