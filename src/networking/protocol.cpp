#include <array>
#include "protocol.hpp"
#include "networking.hpp"


namespace CS260 {
	Protocol::Protocol()
	{
		mSequenceNumber = 0;
		mSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		//error checking
		if (mSocket == SOCKET_ERROR) {

			int error = ::WSAGetLastError();
			if (error != WSAEWOULDBLOCK) {
				throw(std::exception("Unable to create client socket"));
			}
		}

		//set socket non blocking
		unsigned long nonblocking = 1;
		ioctlsocket(mSocket, FIONBIO, &nonblocking);
	}

	void Protocol::SendPacket(Packet_Types _type, void* _packet, unsigned _size , const sockaddr* _addr)
	{

		std::array<char, 8192 > mBuffer{};

		PacketHeader mHeader{ mSequenceNumber , 0 , true, _type };
		mSequenceNumber++;

		memcpy(mBuffer.data(), &mHeader, sizeof(PacketHeader));

		size_t mPacketSize = 0;

		switch (_type) {
		case Packet_Types::VoidPacket:
			mPacketSize = 0;
			break;
		case Packet_Types::ObjectCreation:
			mPacketSize = sizeof(ObjectCreationPacket);
			break;
		case Packet_Types::ObjectDestruction:
			mPacketSize = sizeof(ObjectDestructionPacket);
			break;
		case Packet_Types::ObjectUpdate:
			mPacketSize = sizeof(ObjectUpdatePacket);
			break;
		case Packet_Types::ShipPacket:
			mPacketSize = sizeof(ShipUpdatePacket);
			break;
		}

		memcpy(mBuffer.data() + sizeof(PacketHeader), _packet, mPacketSize);

		if (_addr){
		
			sendto(mSocket, mBuffer.data(), sizeof(PacketHeader) + mPacketSize, 0, _addr, sizeof(sockaddr));
		}
		else {

			send(mSocket, mBuffer.data(), sizeof(PacketHeader) + mPacketSize, 0); 
		}

		
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

			unsigned mPacketSize = 0;

			switch (mHeader.mPackType) {

			case Packet_Types::VoidPacket:
				mPacketSize = 0;
				break;
			case Packet_Types::ObjectCreation:
				mPacketSize = sizeof(ObjectCreationPacket);
				break;
			case Packet_Types::ObjectDestruction:
				mPacketSize = sizeof(ObjectDestructionPacket);
				break;
			case Packet_Types::ObjectUpdate:
				mPacketSize = sizeof(ObjectUpdatePacket);
				break;
			case Packet_Types::ShipPacket:
				mPacketSize = sizeof(ShipUpdatePacket);
				break;
			}

			*_size = mPacketSize;
			memcpy(_payload, mBuffer.data() + sizeof(PacketHeader), mPacketSize);


		}





	}

}