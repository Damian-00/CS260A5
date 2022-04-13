#pragma once
#include "networking.hpp"

#include <glm/vec2.hpp>

#include <vector>
#include <queue>
#include <array>

namespace CS260 {
	
	const unsigned MAX_BUFFER_SIZE = 8192;

	enum Packet_Types {

		VoidPacket,
		ObjectUpdate,
		ShipPacket,
		ObjectCreation,
		ObjectDestruction,
		SYN,
		SYNACK,
		NewPlayer
	};

	
	struct PacketHeader {

		unsigned mSeq;
		unsigned mAck;
		bool mNeedsAcknowledgement;
		Packet_Types mPackType;

	};
	struct ProtocolPacket
	{
		std::array<char, MAX_BUFFER_SIZE > mBuffer;
	};

	struct ObjectUpdatePacket{

		unsigned mObjectId;
		glm::vec2 mObjectPos;
		glm::vec2 mObjectVel;
		
	};

	struct PlayerInfo
	{
		unsigned char mID;
		glm::vec2 pos;
		float rot;
		glm::vec2 vel;
		bool inputPressed; //for particles
	};

	struct NewPlayerPacket
	{
		PlayerInfo mPlayerInfo;
	};
	struct ShipUpdatePacket
	{
		PlayerInfo mPlayerInfo;

	};

	struct ObjectCreationPacket {

		unsigned mObjectId;
		glm::vec2 mCreatePos;
	};
	
	struct SYNPacket
	{		
	};
	
	struct SYNACKPacket
	{
		unsigned char mPlayerID;
	};

	struct ObjectDestructionPacket
	{
		unsigned mObjectId;
	};
	
	
	class Protocol {
	
	public:
		Protocol();
		
		~Protocol();
		
		void SendPacket(Packet_Types, void* packet, const sockaddr* = nullptr);

		void RecievePacket(void* _payload, unsigned* _size, Packet_Types* _type, sockaddr* _addr = nullptr);

		void SetSocket(SOCKET);
		
	private:

		unsigned GetTypeSize(Packet_Types type, bool* needsACK = nullptr);

		unsigned mSequenceNumber;
		std::vector<std::array<char, 8192>> mUnacknowledgedMessages;
		SOCKET mSocket;
	};
}