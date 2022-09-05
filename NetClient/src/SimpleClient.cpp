#include <iostream>
#include <Networking.h>

enum class CustomMsgTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage
};

class CustomClient : public Net::ClientInterface<CustomMsgTypes>
{
public:
	void PingServer()
	{
		Net::Message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerPing;

		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

		msg << timeNow;
		Send(msg);
	}

	void MessageAll()
	{
		Net::Message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::MessageAll;
		Send(msg);
	}
};

int main()
{
	CustomClient c;
	c.Connect("127.0.0.1", 60000);

	bool key[3] = { false, false, false };
	bool oldKey[3] = { false, false, false };
	HWND thisHwnd = GetForegroundWindow();

	bool isQuit = false;
	while (!isQuit)
	{
		
		if (GetForegroundWindow() == thisHwnd)
		{
			key[0] = GetAsyncKeyState('1') & 0x8000;
			key[1] = GetAsyncKeyState('2') & 0x8000;
			key[2] = GetAsyncKeyState('3') & 0x8000;
		}

		if (key[0] && !oldKey[0]) c.PingServer();
		if (key[1] && !oldKey[1]) c.MessageAll();
		if (key[2] && !oldKey[2]) isQuit = true;

		for (int i = 0; i < 3; i++) oldKey[i] = key[i];

		if (!c.IsConnected())
		{
			std::cout << "Server Down\n";
			isQuit = true;
			continue;
		}

		if (!c.Incoming().IsEmpty())
		{
			auto msg = c.Incoming().PopFront().msg;

			switch (msg.header.id)
			{
			case CustomMsgTypes::ServerAccept:
			{
				std::cout << "Server Accepted Connection\n";
			}
			break;

			case CustomMsgTypes::ServerDeny:
			{

			}
			break;

			case CustomMsgTypes::ServerPing:
			{
				std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
				std::chrono::system_clock::time_point timeThen;
				msg >> timeThen;
				std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
			}
			break;

			case CustomMsgTypes::MessageAll:
			{

			}
			break;

			case CustomMsgTypes::ServerMessage:
			{
				uint32_t clientID;
				msg >> clientID;
				std::cout << "Hello from [" << clientID << "]\n";
			}
			break;
			}
		}
	}

	return 0;
}