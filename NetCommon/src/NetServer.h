#pragma once

#include "NetCommon.h"
#include "NetTSQueue.h"
#include "NetMessage.h"
#include "NetConnection.h"

namespace Net
{
	template<typename T>
	class ServerInterface
	{
	public:
		ServerInterface(uint16_t port) : m_ASIOAcceptor(m_ASIOContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
		{

		}

		virtual ~ServerInterface()
		{
			Stop();
		}

		bool Start()
		{
			try
			{
				WaitForClientConnection();

				m_ThrContext = std::thread([this]() { m_ASIOContext.run(); });
			}
			catch (const std::exception& e)
			{
				std::cerr << "[SERVER] Exception: " << e.what() << "\n";
				return false;
			}

			std::cout << "[SERVER] Started!\n";
			return true;
		}

		void Stop()
		{
			m_ASIOContext.stop();
			if (m_ThrContext.joinable()) m_ThrContext.join();

			std::cout << "[SERVER] Stopped!\n";
		}

		// ASYNC - Instruct asio to wait for connection.
		void WaitForClientConnection()
		{
			m_ASIOAcceptor.async_accept(
				[this](std::error_code ec, asio::ip::tcp::socket socket)
				{
					if (!ec)
					{
						std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;

						std::shared_ptr<Connection<T>> newConn = std::make_shared<Connection<T>>(Connection<T>::Owner::Server,
							m_ASIOContext, std::move(socket), m_QMessagesIn);

						if (OnClientConnect(newConn))
						{
							m_DeqConnections.push_back(std::move(newConn));
							m_DeqConnections.back()->ConnectToClient(this, m_IDCounter++);
							std::cout << "[" << m_DeqConnections.back()->GetID() << "] Connection Approved\n";
						}
						else
						{
							std::cout << "[------] Connection Denied!\n";
						}
					}
					else
					{
						std::cout << "[SERVER] New Connection Error: " << ec.message() << std::endl;
					}

					WaitForClientConnection();
				});
		}

		void MessageClient(std::shared_ptr<Connection<T>> client, const Message<T>& msg)
		{
			if (client && client->IsConnected())
			{
				client->Send(msg);
			}
			else
			{
				OnClientDisconnect(client);
				client.reset();
				m_DeqConnections.erase(std::remove(m_DeqConnections.begin(), m_DeqConnections.end(), client), m_DeqConnections.end());
			}
		}

		void MessageAllClients(const Message<T>& msg, std::shared_ptr<Connection<T>> ignoreClient = nullptr)
		{
			bool invalidClientExists = false;

			for (auto& client : m_DeqConnections)
			{
				if (client && client->IsConnected())
				{
					if (client != ignoreClient)
						client->Send(msg);
				}
				else
				{
					OnClientDisconnect(client);
					client.reset();
					invalidClientExists = true;
				}
			}

			if (invalidClientExists)
				m_DeqConnections.erase(std::remove(m_DeqConnections.begin(), m_DeqConnections.end(), nullptr), m_DeqConnections.end());
		}

		void Update(size_t maxMessages = -1, bool wait = false)
		{
			if (wait) m_QMessagesIn.Wait();

			size_t messageCount = 0;
			while (messageCount < maxMessages && !m_QMessagesIn.IsEmpty())
			{
				auto msg = m_QMessagesIn.PopFront();

				OnMessage(msg.remote, msg.msg);
				messageCount++;
			}
		}

		virtual void OnClientValidated(std::shared_ptr<Connection<T>> client)
		{

		}

	protected:
		virtual bool OnClientConnect(std::shared_ptr<Connection<T>> client)
		{
			return false;
		}

		virtual void OnClientDisconnect(std::shared_ptr<Connection<T>> client)
		{

		}

		virtual void OnMessage(std::shared_ptr<Connection<T>> client, Message<T>& msg)
		{

		}

	protected:
		TSQueue<OwnedMessage<T>> m_QMessagesIn;
		std::deque<std::shared_ptr<Connection<T>>> m_DeqConnections;
		asio::io_context m_ASIOContext;
		std::thread m_ThrContext;
		asio::ip::tcp::acceptor m_ASIOAcceptor;
		uint32_t m_IDCounter = 10000u;
	};
}