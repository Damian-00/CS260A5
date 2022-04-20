#pragma once
#include "networking.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <vector>
#include <queue>
#include <array>

#include <tuple>


namespace CS260 {
	
	const unsigned MAX_BUFFER_SIZE = 8192;
	const unsigned tickRate = 16;
	const unsigned timeOutTimer = 1000;

	enum Packet_Types {

		VoidPacket,
		ObjectUpdate,
		ShipPacket,
		ObjectCreation,
		ObjectDestruction,
		SYN,
		SYNACK,
		NewPlayer,
		PlayerDisconnect,
		ACKDisconnect,
		NotifyPlayerDisconnection,
		AsteroidCreation,
		AsteroidUpdate,
		AsteroidDestroy,
		PlayerDie,
		BulletCreation,	
		BulletRequest,
		BulletDestruction
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
	struct AsteroidCreationPacket
	{
		unsigned short mObjectID;
		float mScale;
		float mAngle;
		glm::vec2 mPosition;
		glm::vec2 mVelocity;
	};
	struct AsteroidUpdatePacket
	{
		unsigned short mID;
		glm::vec2 mPosition;
		glm::vec2 mVelocity;
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
		glm::vec4 color;
	};
	struct ShipUpdatePacket
	{
		PlayerInfo mPlayerInfo;
	}; 

	struct BulletRequestPacket 
	{
		glm::vec2 mPos;
		glm::vec2 mVel;
		unsigned mOwnerID;
		float mDir;
	};

	struct BulletCreationPacket 
	{
		unsigned mOwnerID;
		glm::vec2 mPos;
		glm::vec2 mVel;
		float mDir;
		unsigned mObjectID;
	};

	struct BulletDestroyPacket
	{
		// Force this to match PTCL_EXPLOSION_M convention of the game
		enum ExplosionType
		{
			Small = 11,
			Medium = 12
		};
		unsigned mObjectID;
		
		// Particle management
		ExplosionType mType;
		glm::vec2 mPosition;
		unsigned mCount;
		float mAngleMin;
		float mAngleMax;
		float mSrcSize = 0.0f;
		float mVelScale = 1.0f;
		glm::vec2 mVelInit = {0, 0};
	};
	struct ObjectCreationPacket 
	{
		unsigned mObjectId;
		glm::vec2 mCreatePos;
	};
	
	struct SYNPacket
	{		
	};
	
	struct SYNACKPacket
	{
		unsigned char mPlayerID;
		glm::vec4 color;
	};

	struct PlayerDiePacket
	{
		unsigned char mPlayerID;
		unsigned char mRemainingLifes;
	};
	struct PlayerDisconnectPacket
	{
		unsigned char mPlayerID;
	};
	struct PlayerDisconnectACKPacket
	{
		unsigned char mPlayerID;
	};

	struct AsteroidDestructionPacket
	{
		unsigned short mObjectId;
	};
	
	
	class Protocol {
	
	public:
		Protocol();
		
		~Protocol();

		void Tick();
		
		void SendPacket(Packet_Types, void* packet, const sockaddr* = nullptr);

		bool RecievePacket(void* _payload, unsigned* _size, Packet_Types* _type, sockaddr* _addr = nullptr);

		void SetSocket(SOCKET);
		
	private:

		unsigned GetTypeSize(Packet_Types type, bool* needsACK = nullptr);

		unsigned mSequenceNumber;
		//std::vector<std::pair<std::array<char, 8192>, unsigned>> mUnacknowledgedMessages;
		std::vector<std::tuple<std::array<char, 8192>, unsigned, const sockaddr*>> mUnacknowledgedMessages;
		SOCKET mSocket;

		const unsigned mResendTime = 300;

		const unsigned tickRate = 16;
	};
}