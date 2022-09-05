#pragma once

#include "NetCommon.h"
#include "NetTSQueue.h"
#include "NetMessage.h"
#include "NetConnection.h"

namespace Net
{
	template<typename T>
	class ClientInterface
	{
	public:
		ClientInterface()
		{

		}

		virtual ~ClientInterface()
		{
			Disconnect();
		}

		bool Connect(const std::string& host, const uint16_t& port)
		{
			try
			{
				asio::ip::tcp::resolver resolver(m_Context);
				asio::ip::tcp::resolver::results_type endPoints = resolver.resolve(host, std::to_string(port));

				m_Connection = std::make_unique<Connection<T>>(Connection<T>::Owner::Client,
					m_Context,
					asio::ip::tcp::socket(m_Context),
					m_QMessagesIn);

				m_Connection->ConnectToServer(endPoints);

				m_ThrContext = std::thread([this]() { m_Context.run(); });
			}
			catch (const std::exception& e)
			{
				std::cerr << "Client exception: " << e.what() << "\n";
				return false;
			}

			return false;
		}

		void Disconnect()
		{
			if (IsConnected())
			{
				m_Connection->Disconnect();
			}

			m_Context.stop();
			if (m_ThrContext.joinable())
				m_ThrContext.join();

			m_Connection.release();
		}

		bool IsConnected()
		{
			return m_Connection ? m_Connection->IsConnected() : false;
		}

		void Send(const Message<T>& msg)
		{
			if (IsConnected())
			{
				m_Connection->Send(msg);
			}
		}

		TSQueue<OwnedMessage<T>>& Incoming()
		{
			return m_QMessagesIn;
		}

	protected:
		asio::io_context m_Context;
		std::thread m_ThrContext;
		std::unique_ptr<Connection<T>> m_Connection;

	private:
		TSQueue<OwnedMessage<T>> m_QMessagesIn;
	};
}