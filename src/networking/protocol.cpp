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
		//for each message sent that hasnt been ack
		for (auto & message : mUnacknowledgedMessages) {
			//increase how much time has passed since we sent it
			std::get<1>(message) += tickRate;
			//if more time than the resend time has passed
			if (std::get<1>(message) > mResendTime) {
				//need to resend the message

				if (std::get<2>(message)) { // there is a sockaddr pointer

					sendto(mSocket, std::get<0>(message).data(), (int)std::get<0>(message).size(), 0, std::get<2>(message), (int)sizeof(sockaddr));
				}
				else { //there is not

					send(mSocket, std::get<0>(message).data(), (int)std::get<0>(message).size(), 0);

				}
			}
		}


	}
	void Protocol::SendPacket(Packet_Types _type, void* _packet, const sockaddr* _addr)
	{
		std::array<char, 8192 > mBuffer{};

		bool needsAck = false;
		unsigned mPacketSize = GetTypeSize(_type, &needsAck);
		
		//construct the header of the packet
		PacketHeader mHeader{ mSequenceNumber , 0, needsAck, _type };
		mSequenceNumber++;

		//store the ehader on the actual buffer
		memcpy(mBuffer.data(), &mHeader, sizeof(PacketHeader));
		//store the actual payload in the buffer
		memcpy(mBuffer.data() + sizeof(PacketHeader), _packet, mPacketSize);

		//check whether we need to send it to an endpoint or to the connected one
		if (_addr)
			sendto(mSocket, mBuffer.data(), sizeof(PacketHeader) + mPacketSize, 0, _addr, sizeof(sockaddr));
		else 
			send(mSocket, mBuffer.data(), sizeof(PacketHeader) + mPacketSize, 0); 

		//check whether we need to store it in those to resend if not acknowledged
		if (needsAck)
			mUnacknowledgedMessages.push_back(std::tuple< std::array<char, 8192>, unsigned,const sockaddr*>(mBuffer, 0, _addr));
	}

	bool Protocol::ReceivePacket(void* _payload, unsigned *_size, Packet_Types* _type, sockaddr * _addr)
	{

		std::array<char, 8192> mBuffer{};
		//boolean to keep track if we already handled this packet
		bool alreadyReceived = false;

		int received;

		if (_addr){ // we dont  know who we are receiving from
			int addr_size = sizeof(*_addr);
			
			received = recvfrom(mSocket, mBuffer.data(), (int)mBuffer.size(), 0, _addr, &addr_size);
			// Do nothing , this will not update the Keep alive timer so in case of multiple errors diconnection happens
			if (received == SOCKET_ERROR)
				return false;
		}
		else { // we do know
			
			received = recv(mSocket, mBuffer.data(), (int)mBuffer.size(), 0);

			// If received any error
			// Do nothing			
			if (received == SOCKET_ERROR)
				return false;
		}

		//cast the header message
		if (received > 0)
		{
			PacketHeader mHeader;
			memcpy(&mHeader, mBuffer.data(), sizeof(mHeader));

			//in case we need to acknowledge the packet
			if (mHeader.mNeedsAcknowledgement) {
				for (auto& i : mLast100AckMessages) {
					//we already received so dont handle it anymore
					if (i == mHeader.mSeq) {
						alreadyReceived = true;
					}
				}

				if (!alreadyReceived) {
					//create the acknowledgement void packet
					PacketHeader mAckHeader = { 0 , mHeader.mSeq , false, Packet_Types::VoidPacket };
					std::array<char, 8192> ackPack;
					memcpy(&ackPack, &mAckHeader, sizeof(mAckHeader));

					//send the ack packet
					if (_addr) {

						int sent = sendto(mSocket, ackPack.data(), sizeof(mAckHeader), 0, _addr, sizeof(sockaddr));
						//std::cout << WSAGetLastError();

					}
					else {

						int sent = send(mSocket, ackPack.data(), sizeof(PacketHeader), 0);
					}

					//add it to the ones we received, so that if we receive it again we discard it 
					mLast100AckMessages.push_back(mHeader.mSeq);

					//if the size is greater than 100 remove the first one
					if (mLast100AckMessages.size() > 100) {
						mLast100AckMessages.pop_front();
					}
				}
			}


			//in case it was an acknowledgement{
			if (mHeader.mAck != 0) {

				//erase the message if we just received its ack
				auto erased = std::erase_if(mUnacknowledgedMessages, [&](std::tuple<std::array<char, 8192>,unsigned, const sockaddr*> a) {

					PacketHeader mThisHeader;
					memcpy(&mThisHeader, std::get<0>(a).data(), sizeof(mThisHeader));

					if (mHeader.mAck == mThisHeader.mSeq) {
						return true; //we found the element we received the ack from
					}
					return false;
					});

			}

			
			//if we need to handle it
			if (!alreadyReceived){

				//store in the out parameters
				*_type = mHeader.mPackType;

				unsigned mPacketSize = GetTypeSize(mHeader.mPackType);

				*_size = mPacketSize;
				memcpy(_payload, mBuffer.data() + sizeof(PacketHeader), mPacketSize);
			}
			else {

				//store empty out paramteres as dummy
				*_type = Packet_Types::VoidPacket;
				*_size = 0;

			}
		}
		return true;
	}

	unsigned Protocol::GetTypeSize(Packet_Types type, bool* needsACKPtr)
	{
		
		unsigned packetSize = 0;
		bool needsACK = false;

		//just for each kind of message, store whether it needs acknowledgement and the size of the corresponding packet
		switch (type) 
		{
		case Packet_Types::VoidPacket:
			packetSize = 0;
			break;
		case Packet_Types::ObjectCreation:
			packetSize = sizeof(ObjectCreationPacket);
			needsACK = true;
			break;
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
		case Packet_Types::BulletDestruction:
			packetSize = sizeof(BulletDestroyPacket);
			needsACK = true;
			break;
		case Packet_Types::ScoreUpdate:
			packetSize = sizeof(ScorePacket);
			needsACK = true;
		}
		
		//store in the out param
		if(needsACKPtr)
			*needsACKPtr = needsACK;
		
		return packetSize;
	}

	void Protocol::SetSocket(SOCKET _s)
	{
		mSocket = _s;
	}
	
}
