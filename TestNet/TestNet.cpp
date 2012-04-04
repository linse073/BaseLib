// TestNet.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <signal.h>
#include "Echo.h"
#include "WorkQueue.h"
#include "Packet.h"
#include "Manager.h"
#include "Log.h"
#include <vector>

using std::tr1::bind;
using namespace std::tr1::placeholders;

bool run = true;

BOOL WINAPI HandlerRoutine(DWORD ctrlTpye)
{
	run = false;
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	CNetInit net;
	CPacket packet;
	CEcho echo(bind(&CPacket::Send, &packet, _1, _2, _3));
	CWorkQueue workQueu(bind(&CEcho::Process, &echo, _1, _2, _3), bind(&CEcho::End, &echo, _1));
	CManager manager(bind(&CPacket::Process, &packet, _1, _2, _3), bind(&CWorkQueue::End, &workQueu, _1));
	packet.Init(bind(&CWorkQueue::Post, &workQueu, _1, _2, _3), bind(&CManager::Send, &manager, _1, _2, _3));
	workQueu.Start(1);
	manager.Start();

	if (SetConsoleCtrlHandler(HandlerRoutine, TRUE) == 0)
		LOG("Fail to call SetConsoleCtrlHandler.");

	typedef std::vector<int> Clients;
	typedef Clients::iterator ClientsIter;
	manager.StartServer("192.168.2.102", 9999);
	Sleep(1000); //Server must wait for a moment to accept connection.
	Clients clients;
	for (int i=0; i<3000; ++i)
	{
		int clientID = manager.AddClient("192.168.2.102", 9999);
		if (clientID == 0)
		{
			LOG("Fail to add client.");
			continue;
		}
		clients.push_back(clientID);
		echo.AddAsynClient(clientID);
	}
	int count = 0;
	char message[512];
	while (run)
	{
		++count;
		for (ClientsIter iter=clients.begin(); iter!=clients.end(); ++iter)
		{
			sprintf_s(message, sizeof(message), "Client[%d] send message[%d].", *iter, count);
			echo.Send(*iter, message, strlen(message)+1);
		}
		Sleep(1000);
	}

	manager.Stop();
	workQueu.Stop();

	return 0;
}

