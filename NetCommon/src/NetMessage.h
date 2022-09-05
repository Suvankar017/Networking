#pragma once

#include "NetCommon.h"

namespace Net
{
	template<typename T>
	struct MessageHeader
	{
		T id{};
		uint32_t size = 0;
	};

	template<typename T>
	struct Message
	{
		MessageHeader<T> header{};
		std::vector<uint32_t> body;

		size_t Size() const
		{
			return /*sizeof(MessageHeader<T>) + */ body.size();
		}

		friend std::ostream& operator <<(std::ostream& os, const Message<T>& msg)
		{
			os << "ID: " << int(msg.header.id) << ", Size: " << msg.header.size;
			return os;
		}

		template<typename DataType>
		friend Message<T>& operator <<(Message<T>& msg, const DataType& data)
		{
			// Check that the type of the data being pushed is simply copyable.
			static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector.");

			// Cache current size of vector, as this will be the point we insert the data.
			size_t prevSize = msg.body.size();

			// Resize the vector by the size of the data being pushed.
			msg.body.resize(prevSize + sizeof(DataType));

			// Physically copy the data into the newly allocated vector space.
			std::memcpy(msg.body.data() + prevSize, &data, sizeof(DataType));

			// Recalculate the message size.
			msg.header.size = (uint32_t)msg.Size();

			return msg;
		}

		template<typename DataType>
		friend Message<T>& operator >>(Message<T>& msg, DataType& data)
		{
			// Check that the type of the data being pushed is simply copyable.
			static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled from vector.");

			// Cache the location towards the end of the vector where the pulled data start.
			size_t start = msg.body.size() - sizeof(DataType);

			// Physically copy the data from the vector into the user variable.
			std::memcpy(&data, msg.body.data() + start, sizeof(DataType));

			// Shrink the vector to remove read bytes, and reset end position.
			msg.body.resize(start);

			// Recalculate the message size.
			msg.header.size = (uint32_t)msg.Size();

			return msg;
		}
	};

	template<typename T>
	class Connection;

	template<typename T>
	struct OwnedMessage
	{
		std::shared_ptr<Connection<T>> remote = nullptr;
		Message<T> msg;

		friend std::ostream& operator <<(std::ostream& os, const OwnedMessage<T>& msg)
		{
			os << msg.msg;
			return os;
		}
	};
}