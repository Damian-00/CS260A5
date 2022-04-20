#pragma once
#include "networking.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <vector>
#include <queue>
#include <array>
#include <list>

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
		BulletDestruction,
		ScoreUpdate
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
		unsigned short mRemainingLifes;
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
		unsigned short mRemainingLifes;
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

	struct ScorePacket
	{
		unsigned mPlayerID;
		unsigned CurrentScore;
	};
	
	class Protocol {
	
	public:
		/**
		* @brief
		*  Constructs the protocol initializing variables
		*/
		Protocol();
		
		/**
		* @brief
		*  Destroys the protocol and frees the network
		*/
		~Protocol();

		/**
		* @brief
		* Update function to resend unackowledged messages
		*/
		void Tick();
		
		/**
		* @brief
		* Sends a packet given its type and the content
		* @param type : type of the packet
		* @param packet : content of the packet
		* @param sockaddr : address to send the packet to, if null, sends to the connected endpoint
		*/
		void SendPacket(Packet_Types, void* packet, const sockaddr* = nullptr);

		/**
		* @brief
		* Receives a packet and fills the information as out parameters
		* @param _payload : out parameter for the actual packet contentes
		* @param _size : out parameter for the size of the pacekt
		* @param _type : out parameter for the type of the packet
		* @param _addr : optional out parameter for the endpoint of the sender of the packet
		*/
		bool ReceivePacket(void* _payload, unsigned* _size, Packet_Types* _type, sockaddr* _addr = nullptr);
		
		/**
		* @brief
		* Socket settor to know with which socket the protocol is working with
		*/
		void SetSocket(SOCKET);
		
	private:

		/**
		* @brief
		* Given a type, returns the size of the packet of that size and whether or not it needs acknowledgement
		* @param needsAck : out parameter for whether or not the packet of that type needs acknowledgement
		* @return
		* The size of that kind of packet
		*/
		unsigned GetTypeSize(Packet_Types type, bool* needsACK = nullptr);
		
		//actual sequence number of the protocol for when sending packets
		unsigned mSequenceNumber;
		
		//messages we have sent but have not been acknowledged, we store the message, the time we have sent it ago, and to who we have sent it
		std::vector<std::tuple<std::array<char, 8192>, unsigned, const sockaddr*>> mUnacknowledgedMessages;

		//socket we are working with
		SOCKET mSocket;

		//time to resend an unacknowledged message
		const unsigned mResendTime = 300;

		const unsigned tickRate = 16;

		//to not handle the same message twice, we store the last 100 messages we received, to check against them
		std::list<unsigned> mLast100AckMessages{};
	};
}