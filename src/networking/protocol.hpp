#pragma once
#include "networking.hpp"
#include <vector>
#include <glm/vec2.hpp>




namespace CS260 {


	enum Packet_Types {
		ObjectPacket,
		ShipPacket,
		ObjectCreation,
		ObjectDestruction

	};

	struct ObjectUpdatePacket{

		unsigned mObjectId;
		glm::vec2 mVel;
	};

	struct ShipUpdatePacket {
		
	};

	struct ObjectCreationPacket {
		unsigned mObjectId;

	};

	struct ObjectDestructionPacket {
		unsigned mObjectId;

	};
	
	
	class Protocol {
	
	public:

		void SendPacket(Packet_Types, void* packet, unsigned size);

	private:

		unsigned mSequenceNumber;

		std::vector<std::vector<char>> mUnacknowledgedMessages;
		

	};


}