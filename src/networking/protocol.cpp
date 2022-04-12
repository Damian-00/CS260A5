#include <array>
#include "protocol.hpp"
#include "networking.hpp"
#include <algorithm>


namespace CS260 {
	Protocol::Protocol()
	{
		// Initialize the networking library
		NetworkCreate();

		mSequenceNumber = 1;
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

	Protocol::~Protocol()
	{
		NetworkDestroy();
	}

	void Protocol::SendPacket(Packet_Types _type, void* _packet, unsigned _size , const sockaddr* _addr)
	{

		std::array<char, 8192 > mBuffer{};

		size_t mPacketSize = 0;
		bool needsAck = false;

		switch (_type) {
		case Packet_Types::VoidPacket:
			mPacketSize = 0;
			break;
		case Packet_Types::ObjectCreation:
			mPacketSize = sizeof(ObjectCreationPacket);
			needsAck = true;
			break;
		case Packet_Types::ObjectDestruction:
			mPacketSize = sizeof(ObjectDestructionPacket);
			needsAck = true;
			break;
		case Packet_Types::ObjectUpdate:
			mPacketSize = sizeof(ObjectUpdatePacket);
			needsAck = false;
			break;
		case Packet_Types::ShipPacket:
			mPacketSize = sizeof(ShipUpdatePacket);
			needsAck = false;
			break;
		}

		PacketHeader mHeader{ mSequenceNumber , 0 , needsAck, _type };
		mSequenceNumber++;

		memcpy(mBuffer.data(), &mHeader, sizeof(PacketHeader));

		memcpy(mBuffer.data() + sizeof(PacketHeader), _packet, mPacketSize);

		if (_addr){
		
			sendto(mSocket, mBuffer.data(), sizeof(PacketHeader) + mPacketSize, 0, _addr, sizeof(sockaddr));
		}
		else {

			send(mSocket, mBuffer.data(), sizeof(PacketHeader) + mPacketSize, 0); 
		}


		if (needsAck)
			mUnacknowledgedMessages.push_back(mBuffer); 

		
	}

	void Protocol::RecievePacket(void* _payload, unsigned *_size, Packet_Types* _type, sockaddr * _addr)
	{

		std::array<char, 8192> mBuffer{};

		int received;

		if (_addr){ // we dont  know who we are receiving from
			
			
			int addr_size = sizeof(*_addr);
			received = recvfrom(mSocket, mBuffer.data(), mBuffer.size(), 0, _addr, &addr_size);
			std::cout << WSAGetLastError(); 

		}
		else { // we do know

			received = recv(mSocket, mBuffer.data(), sizeof(mBuffer), 0);

		}

		//cast the header message
		if (received > 0) {

			PacketHeader mHeader;
			memcpy(&mHeader, mBuffer.data(), sizeof(mHeader));

			//in case we need to acknowledge the packet
			if (mHeader.mNeedsAcknowledgement) {
				
				//create the acknowledgement void packet
				PacketHeader mAckHeader = { 0 , mHeader.mSeq , false, Packet_Types::VoidPacket };
				std::array<char, 8192> ackPack;
				memcpy(&ackPack, &mAckHeader, sizeof(mAckHeader));


				if (_addr) {

					int sent = sendto(mSocket, ackPack.data(), sizeof(mAckHeader) , 0, _addr, sizeof(sockaddr));
					std::cout << WSAGetLastError();

				}
				else {

					int sent = send(mSocket, mBuffer.data(), sizeof(PacketHeader), 0);
				}
			}
			

			//in case it was an acknowledgement{
			if (mHeader.mAck != 0) {

				auto erased = std::erase_if(mUnacknowledgedMessages, [&](std::array<char, 8192> a) {

					PacketHeader mThisHeader;
					memcpy(&mThisHeader, a.data(), sizeof(mThisHeader));

					if (mHeader.mAck == mThisHeader.mAck) {
						return true; //we found the element we received the ack from
					}

					});

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

	void Protocol::SetSocket(SOCKET _s)
	{
		mSocket = _s;
	}

}