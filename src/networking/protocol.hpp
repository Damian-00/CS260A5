#pragma once
#include "networking.hpp"
#include <vector>
#include <queue>
#include <glm/vec2.hpp>
#include "networking.hpp"




namespace CS260 {


	enum Packet_Types {

		VoidPacket,
		ObjectUpdate,
		ShipPacket,
		ObjectCreation,
		ObjectDestruction

	};

	struct PacketHeader {

		unsigned mSeq;
		unsigned mAck;
		bool mNeedsAcknowledgement;
		Packet_Types mPackType;

	};

	struct ObjectUpdatePacket{

		unsigned mObjectId;
		glm::vec2 mObjectPos;
		glm::vec2 mObjectVel;
		
	};

	struct ShipUpdatePacket {

		glm::vec2 mShipVel;
		glm::vec2 mShipAcc;
	};

	struct ObjectCreationPacket {

		unsigned mObjectId;
		glm::vec2 mCreatePos;
	};

	struct ObjectDestructionPacket {
		unsigned mObjectId;
	};
	
	
	class Protocol {
	
	public:
		Protocol();

		~Protocol();

		void SendPacket(Packet_Types, void* packet, unsigned size, const sockaddr* );

		void RecievePacket(void* _payload, unsigned* _size, Packet_Types* _type, sockaddr* _addr);

		void SetSocket(SOCKET);

	private:

		unsigned mSequenceNumber = 1;
		std::vector<std::array<char, 8192>> mUnacknowledgedMessages;
		
		SOCKET mSocket;

	};


}