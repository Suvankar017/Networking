#pragma once

#include "NetCommon.h"
#include "NetTSQueue.h"
#include "NetMessage.h"

namespace Net
{
	template<typename T>
	class ServerInterface;

	template<typename T>
	class Connection : public std::enable_shared_from_this<Connection<T>>
	{
	public:
		enum class Owner
		{
			Server,
			Client
		};

		Connection(Owner parent, asio::io_context& context, asio::ip::tcp::socket socket, TSQueue<OwnedMessage<T>>& qIn) :
			m_ASIOContext(context), m_Socket(std::move(socket)), m_QMessagesIn(qIn)
		{
			m_OwnerType = parent;

			if (m_OwnerType == Owner::Server)
			{
				m_HandshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
				m_HandshakeCheck = Scramble(m_HandshakeOut);
			}
			else
			{
				m_HandshakeIn = 0;
				m_HandshakeOut = 0;
			}
		}
		virtual ~Connection() {}

		uint32_t GetID() const
		{
			return m_ID;
		}

		void ConnectToClient(Net::ServerInterface<T>* server, uint32_t id = 0)
		{
			if (m_OwnerType == Owner::Server)
			{
				if (m_Socket.is_open())
				{
					m_ID = id;
					WriteValidation();
					ReadValidation(server);
				}
			}
		}

		void ConnectToServer(const asio::ip::tcp::resolver::results_type& endPoints)
		{
			if (m_OwnerType == Owner::Client)
			{
				asio::async_connect(m_Socket, endPoints,
					[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
					{
						if (!ec)
						{
							ReadValidation();
						}
					});
			}
		}

		void Disconnect()
		{
			if (IsConnected())
			{
				asio::post(m_ASIOContext, [this]() { m_Socket.close(); });
			}
		}

		bool IsConnected() const
		{
			return m_Socket.is_open();
		}

		void StartListening()
		{

		}

		void Send(const Message<T>& msg)
		{
			asio::post(m_ASIOContext,
				[this, msg]()
				{
					bool writingMessage =  !m_QMessagesOut.IsEmpty();
					m_QMessagesOut.PushBack(msg);
					if (!writingMessage)
					{
						WriteHeader();
					}
				});
		}

	private:
		void ReadHeader()
		{
			asio::async_read(m_Socket, asio::buffer(&m_MsgTemporaryIn.header, sizeof(MessageHeader<T>)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_MsgTemporaryIn.header.size > 0)
						{
							m_MsgTemporaryIn.body.resize(m_MsgTemporaryIn.header.size);
							ReadBody();
						}
						else
						{
							AddToIncomingMessageQueue();
						}
					}
					else
					{
						std::cout << "[" << m_ID << "] Read Header Fail.\n";
						m_Socket.close();
					}
				});
		}

		void ReadBody()
		{
			asio::async_read(m_Socket, asio::buffer(m_MsgTemporaryIn.body.data(), m_MsgTemporaryIn.body.size()),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						AddToIncomingMessageQueue();
					}
					else
					{
						std::cout << "[" << m_ID << "] Read Body Fail.\n";
						m_Socket.close();
					}
				});
		}

		void WriteHeader()
		{
			asio::async_write(m_Socket, asio::buffer(&m_QMessagesOut.Front().header, sizeof(MessageHeader<T>)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_QMessagesOut.Front().body.size() > 0)
						{
							WriteBody();
						}
						else
						{
							m_QMessagesOut.PopFront();
							if (!m_QMessagesOut.IsEmpty())
							{
								WriteHeader();
							}
						}
					}
					else
					{
						std::cout << "[" << m_ID << "] Write Header Fail.\n";
						m_Socket.close();
					}
				});
		}

		void WriteBody()
		{
			asio::async_write(m_Socket, asio::buffer(m_QMessagesOut.Front().body.data(), m_QMessagesOut.Front().body.size()),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						m_QMessagesOut.PopFront();
						if (!m_QMessagesOut.IsEmpty())
						{
							WriteHeader();
						}
					}
					else
					{
						std::cout << "[" << m_ID << "] Write Body Fail.\n";
						m_Socket.close();
					}
				});
		}

		void AddToIncomingMessageQueue()
		{
			if (m_OwnerType == Owner::Server)
				m_QMessagesIn.PushBack({ this->shared_from_this(), m_MsgTemporaryIn });
			else
				m_QMessagesIn.PushBack({ nullptr, m_MsgTemporaryIn });

			ReadHeader();
		}

		uint64_t Scramble(uint64_t input)
		{
			uint64_t out = input ^ 0xdeadbeefc0decafe;
			out = (out & 0xf0f0f0f0f0f0f0) >> 4 | (out & 0x0f0f0f0f0f0f0f) << 4;
			return out ^ 0xc0deface12345678;
		}

		void WriteValidation()
		{
			asio::async_write(m_Socket, asio::buffer(&m_HandshakeOut, sizeof(uint64_t)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_OwnerType == Owner::Client)
							ReadHeader();
					}
					else
					{
						m_Socket.close();
					}
				});
		}

		void ReadValidation(Net::ServerInterface<T>* server = nullptr)
		{
			asio::async_read(m_Socket, asio::buffer(&m_HandshakeIn, sizeof(uint64_t)),
				[this, server](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_OwnerType == Owner::Server)
						{
							if (m_HandshakeIn == m_HandshakeCheck)
							{
								std::cout << "Client Validated" << std::endl;
								server->OnClientValidated(this->shared_from_this());

								ReadHeader();
							}
							else
							{
								std::cout << "Client Disconnected (Fail Validation)" << std::endl;
								m_Socket.close();
							}
						}
						else
						{
							m_HandshakeOut = Scramble(m_HandshakeIn);
							WriteValidation();
						}
					}
					else
					{
						std::cout << "Client Disconnected (ReadValidation)" << std::endl;
						m_Socket.close();
					}
				});
		}

	protected:
		asio::ip::tcp::socket m_Socket;
		asio::io_context& m_ASIOContext;
		TSQueue<Message<T>> m_QMessagesOut;
		TSQueue<OwnedMessage<T>>& m_QMessagesIn;
		Message<T> m_MsgTemporaryIn;
		Owner m_OwnerType = Owner::Server;
		uint32_t m_ID = 0;

		// Handshake validation
		uint64_t m_HandshakeOut = 0;
		uint64_t m_HandshakeIn = 0;
		uint64_t m_HandshakeCheck = 0;
	};
}