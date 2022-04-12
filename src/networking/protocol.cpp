#include <array>
#include "protocol.hpp"
#include "networking.hpp"


namespace CS260 
{
	Protocol::Protocol()
	{
		mSequenceNumber = 0;
	}

	void Protocol::SendPacket(Packet_Types _type, void* _packet, unsigned _size , const sockaddr* _addr)
	{

		std::array<char, 8192 > mBuffer{};

		PacketHeader mHeader{ mSequenceNumber , 0 , true, _type };
		mSequenceNumber++;

		memcpy(mBuffer.data(), &mHeader, sizeof(PacketHeader));


		unsigned mPacketSize = GetTypeSize(mHeader.mPackType);

		memcpy(mBuffer.data() + sizeof(PacketHeader), _packet, mPacketSize);

		if (_addr)
			sendto(mSocket, mBuffer.data(), sizeof(PacketHeader) + mPacketSize, 0, _addr, sizeof(sockaddr));
		else 
			send(mSocket, mBuffer.data(), sizeof(PacketHeader) + mPacketSize, 0); 
	}

	void Protocol::RecievePacket(void* _payload, unsigned *_size, Packet_Types* _type, sockaddr * _addr)
	{

		std::array<char, 8192> mBuffer{};

		int received;

		if (_addr){ // we dont  know who we are receiving from
			int addr_size = 0;
			received = recvfrom(mSocket, mBuffer.data(), sizeof(mBuffer), 0, _addr, &addr_size);
		}
		else { // we do know
			received = recv(mSocket, mBuffer.data(), sizeof(mBuffer), 0);
		}

		//cast the header message
		if (received > 0) {

			PacketHeader mHeader;
			memcpy(&mHeader, mBuffer.data(), sizeof(mHeader));

			if (mHeader.mNeedsAcknowledgement) {
				
				mMessagesToAck.push(mHeader.mSeq); //keeping track of the messages we need to ack
			}
			
			*_type = mHeader.mPackType;

			unsigned mPacketSize = GetTypeSize(mHeader.mPackType);
			
			*_size = mPacketSize;
			
			memcpy(_payload, mBuffer.data() + sizeof(PacketHeader), mPacketSize);
		}
	}

	unsigned Protocol::GetTypeSize(Packet_Types type)
	{
		unsigned packetSize = 0;
		switch (type) 
		{
		case Packet_Types::VoidPacket:
			packetSize = 0;
			break;
		case Packet_Types::ObjectCreation:
			packetSize = sizeof(ObjectCreationPacket);
			break;
		case Packet_Types::ObjectDestruction:
			packetSize = sizeof(ObjectDestructionPacket);
			break;
		case Packet_Types::ObjectUpdate:
			packetSize = sizeof(ObjectUpdatePacket);
			break;
		case Packet_Types::ShipPacket:
			packetSize = sizeof(ShipUpdatePacket);
			break;
		case Packet_Types::SYN:
			packetSize = sizeof(SYNPacket);
			break;
		case Packet_Types::SYNACK:
			packetSize = sizeof(SYNACKPacket);
			break;
		}
		return packetSize;
	}
}