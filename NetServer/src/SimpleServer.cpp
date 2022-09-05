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

class CustomServer : public Net::ServerInterface<CustomMsgTypes>
{
public:
	CustomServer(uint16_t port) : Net::ServerInterface<CustomMsgTypes>(port)
	{

	}

protected:
	virtual bool OnClientConnect(std::shared_ptr<Net::Connection<CustomMsgTypes>> client) override
	{
		Net::Message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerAccept;
		client->Send(msg);
		return true;
	}

	virtual void OnClientDisconnect(std::shared_ptr<Net::Connection<CustomMsgTypes>> client) override
	{
		std::cout << "Removing client [" << client->GetID() << "]\n";
	}

	virtual void OnMessage(std::shared_ptr<Net::Connection<CustomMsgTypes>> client, Net::Message<CustomMsgTypes>& msg) override
	{
		switch (msg.header.id)
		{
		case CustomMsgTypes::ServerAccept:
		{

		}
		break;

		case CustomMsgTypes::ServerDeny:
		{

		}
		break;

		case CustomMsgTypes::ServerPing:
		{
			std::cout << "[" << client->GetID() << "]: Server Ping\n";
			client->Send(msg);
		}
		break;

		case CustomMsgTypes::MessageAll:
		{
			std::cout << "[" << client->GetID() << "]: Message All\n";
			Net::Message<CustomMsgTypes> msg;
			msg.header.id = CustomMsgTypes::ServerMessage;
			msg << client->GetID();
			MessageAllClients(msg, client);
		}
		break;

		case CustomMsgTypes::ServerMessage:
		{

		}
		break;
		}
	}
};

int main()
{
	CustomServer server(60000);
	server.Start();

	while (1)
	{
		server.Update(-1, true);
	}

	return 0;
}