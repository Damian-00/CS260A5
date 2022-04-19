#include "protocol.hpp"

#include "networking.hpp"

#include "utils.hpp"


namespace CS260 
{
	Protocol::Protocol():
		mSocket(0)
	{
		// Initialize the networking library
		NetworkCreate();
		mSequenceNumber = rand() % now_ns() + 1;
	}

	Protocol::~Protocol()
	{
		NetworkDestroy();
	}
	void Protocol::Tick()
	{
		for (auto & message : mUnacknowledgedMessages) {
			std::get<1>(message) += tickRate;
			if (std::get<1>(message) > mResendTime) {
				//need to resend the message

				if (std::get<2>(message)) { // there is a sockaddr pointer

					sendto(mSocket, std::get<0>(message).data(), std::get<0>(message).size(), 0, std::get<2>(message), sizeof(sockaddr));
				}
				else {

					send(mSocket, std::get<0>(message).data(), std::get<0>(message).size(), 0);

				}
			}
		}


	}
	void Protocol::SendPacket(Packet_Types _type, void* _packet, const sockaddr* _addr)
	{
		std::array<char, 8192 > mBuffer{};

		bool needsAck = false;
		unsigned mPacketSize = GetTypeSize(_type, &needsAck);
		
		PacketHeader mHeader{ mSequenceNumber , 0, needsAck, _type };
		mSequenceNumber++;

		memcpy(mBuffer.data(), &mHeader, sizeof(PacketHeader));
		memcpy(mBuffer.data() + sizeof(PacketHeader), _packet, mPacketSize);

		if (_addr)
			sendto(mSocket, mBuffer.data(), sizeof(PacketHeader) + mPacketSize, 0, _addr, sizeof(sockaddr));
		else 
			send(mSocket, mBuffer.data(), sizeof(PacketHeader) + mPacketSize, 0); 

		if (needsAck)
			mUnacknowledgedMessages.push_back(std::tuple< std::array<char, 8192>, unsigned,const sockaddr*>(mBuffer, 0, _addr));
	}

	void Protocol::RecievePacket(void* _payload, unsigned *_size, Packet_Types* _type, sockaddr * _addr)
	{

		std::array<char, 8192> mBuffer{};

		int received;

		if (_addr){ // we dont  know who we are receiving from
			int addr_size = sizeof(*_addr);
			// TODO: Error checking
			received = recvfrom(mSocket, mBuffer.data(), mBuffer.size(), 0, _addr, &addr_size);
		}
		else { // we do know
			// TODO: Error checking
			received = recv(mSocket, mBuffer.data(), mBuffer.size(), 0);
		}

		//cast the header message
		if (received > 0)
		{
			PacketHeader mHeader;
			memcpy(&mHeader, mBuffer.data(), sizeof(mHeader));

			//in case we need to acknowledge the packet
			if (mHeader.mNeedsAcknowledgement) {

				//create the acknowledgement void packet
				PacketHeader mAckHeader = { 0 , mHeader.mSeq , false, Packet_Types::VoidPacket };
				std::array<char, 8192> ackPack;
				memcpy(&ackPack, &mAckHeader, sizeof(mAckHeader));


				if (_addr) {

					int sent = sendto(mSocket, ackPack.data(), sizeof(mAckHeader), 0, _addr, sizeof(sockaddr));
					std::cout << WSAGetLastError();

				}
				else {

					int sent = send(mSocket, ackPack.data(), sizeof(PacketHeader), 0);
				}
			}


			//in case it was an acknowledgement{
			if (mHeader.mAck != 0) {

				auto erased = std::erase_if(mUnacknowledgedMessages, [&](std::tuple<std::array<char, 8192>,unsigned, const sockaddr*> a) {

					PacketHeader mThisHeader;
					memcpy(&mThisHeader, std::get<0>(a).data(), sizeof(mThisHeader));

					if (mHeader.mAck == mThisHeader.mSeq) {
						return true; //we found the element we received the ack from
					}

					});

			}

			*_type = mHeader.mPackType;

			unsigned mPacketSize = GetTypeSize(mHeader.mPackType);

			*_size = mPacketSize;
			memcpy(_payload, mBuffer.data() + sizeof(PacketHeader), mPacketSize);
		}
	}

	unsigned Protocol::GetTypeSize(Packet_Types type, bool* needsACKPtr)
	{
		unsigned packetSize = 0;
		bool needsACK = false;
		switch (type) 
		{
		case Packet_Types::VoidPacket:
			packetSize = 0;
			break;
		case Packet_Types::ObjectCreation:
			packetSize = sizeof(ObjectCreationPacket);
			needsACK = true;
			break;
		//case Packet_Types::ObjectDestruction:
		//	packetSize = sizeof(ObjectDestructionPacket);
		//	needsACK = true;
		//	break;
		case Packet_Types::ObjectUpdate:
			packetSize = sizeof(ObjectUpdatePacket);
			needsACK = false;
			break;
		case Packet_Types::ShipPacket:
			packetSize = sizeof(ShipUpdatePacket);
			needsACK = false;
			break;
		case Packet_Types::SYN:
			packetSize = sizeof(SYNPacket);
			needsACK = false;
			break;
		case Packet_Types::SYNACK:
			packetSize = sizeof(SYNACKPacket);
			needsACK = false;
			break;
		case Packet_Types::NewPlayer:
			packetSize = sizeof(NewPlayerPacket);
			needsACK = true;
			break;
		case Packet_Types::PlayerDisconnect:
			packetSize = sizeof(PlayerDisconnectPacket);
			needsACK = false;
			break;
		case Packet_Types::ACKDisconnect:
			packetSize = sizeof(PlayerDisconnectACKPacket);
			needsACK = false;
			break;
		case Packet_Types::NotifyPlayerDisconnection:
			packetSize = sizeof(PlayerDisconnectPacket);
			needsACK = true;
			break;
		case Packet_Types::AsteroidCreation:
			packetSize = sizeof(AsteroidCreationPacket);
			needsACK = true;
			break;
		case Packet_Types::AsteroidUpdate:
			packetSize = sizeof(AsteroidUpdatePacket);
			needsACK = false;
			break;
		case Packet_Types::AsteroidDestroy:
			packetSize = sizeof(AsteroidDestructionPacket);
			needsACK = true;
			break;
		case Packet_Types::PlayerDie:
			packetSize = sizeof(AsteroidUpdatePacket);
			needsACK = true;
			break;
		case Packet_Types::BulletRequest:
			packetSize =  sizeof(BulletRequestPacket);
			needsACK = true;
			break;
		case Packet_Types::BulletCreation:
			packetSize = sizeof(BulletCreationPacket);
			needsACK = true;
			break;
		}
		
		if(needsACKPtr)
			*needsACKPtr = needsACK;
		
		return packetSize;
	}

	void Protocol::SetSocket(SOCKET _s)
	{
		mSocket = _s;
	}
	
}
