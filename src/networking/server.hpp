/*!
******************************************************************************
	\file    server.hpp
	\author  David Miranda
	\par     DP email: m.david@digipen.edu
	\par     Course: CS260
	\date    04/20/2022

	\brief
	Implementation for all the functionalities required by the server.
*******************************************************************************/
#pragma once
#include "protocol.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace CS260
{
	struct ClientInfo
	{
		/*	\fn ClientInfo
		\brief ClientInfo constructor initializes the endpoint and the player information
		*/
		ClientInfo(sockaddr endpoint, PlayerInfo playerInfo, glm::vec4 color);
		
		// Networking
		sockaddr mEndpoint;
		unsigned mAliveTimer;

		// Disconnection
		bool mDisconnecting;
		unsigned mDisconnectTimeout;
		unsigned mDisconnectTries;

		// Player Information
		PlayerInfo mPlayerInfo;
		glm::vec4 color;
		bool mDead = false;
	};
	
	const unsigned disconnectTries = 3; // In fact, there is a total of 4 tries because the first one is not counted
	const unsigned updateAsteroids = 5000;

	class Server
	{
		unsigned char mCurrentID;
		Protocol mProtocol;
		bool mVerbose;
		SOCKET mSocket;
		sockaddr_in mEndpoint;
		std::vector<ClientInfo> mClients;
		std::vector<NewPlayerPacket> mNewPlayersOnFrame;
		
		std::vector<unsigned char> mDisconnectedPlayersIDs;
		std::vector<AsteroidCreationPacket> mAliveAsteroids;
		unsigned mUpdateAsteroidsTimer;

		std::vector<BulletRequestPacket> mBulletsToCreate;
	public:
		/*	\fn Server
		\brief	Server constructor following RAII design
		*/
		Server(bool verbose, const std::string& ip_address, uint16_t port);

		/*	\fn ~Server
		\brief	Server destructor following RAII design
		*/
		~Server();

		/*	\fn Tick
		\brief	Responsible of receiving packets and handling timeouts
		*/
		void Tick();

		/*	\fn PlayerCount
		\brief	Return the total count of current players
		*/
		int PlayerCount();

		/*	\fn PlayerCount
		\brief	Return the total count of current players
		*/
		const std::vector<NewPlayerPacket>& GetNewPlayers();

		/*	\fn GetPlayersInfo
		\brief	Return the total count of current state of players
		*/
		const std::vector<ClientInfo>& GetPlayersInfo();

		/*	\fn GetBulletsToCreate
		\brief	Return the total count of bullets requests and the information related to each of them
		*/
		const std::vector<BulletRequestPacket>& GetBulletsToCreate();

		/*	\fn GetDisconnectedPlayersIDs
		\brief	Return the ids of the disconnected players
		*/
		const std::vector<unsigned char>& GetDisconnectedPlayersIDs();

		/*	\fn SendPlayerInfo
		\brief	Send the received player info to the rest of the players
		*/
		void SendPlayerInfo(sockaddr _endpoint, PlayerInfo _playerinfo);

		/*	\fn SendAsteroidCreation
		\brief	Send asteroid creation packet to all clients
		*/
		void SendAsteroidCreation(unsigned short id, glm::vec2 position, glm::vec2 velocity, float scale, float angle);

		/*	\fn UpdateAsteroid
		\brief	Updates the stored copy of the given asteroid
		*/
		void UpdateAsteroid(unsigned short id, glm::vec2 position, glm::vec2 velocity);

		/*	\fn SendAsteroidsForcedUpdate
		\brief	Force to update one asteroid instance
		*/
		void SendAsteroidsForcedUpdate(unsigned id, glm::vec2 pos, glm::vec2 vel);
		
		/*	\fn SendAsteroidsUpdate
		\brief	Send asteroid update packet to all clients, this function is called every 5000ms and by the game instance
		*/
		void SendAsteroidsUpdate();

		/*	\fn SendPlayerDiePacket
		\brief	Notify to the clients that one player died safely
		*/
		void SendPlayerDiePacket(unsigned char playerID, unsigned short remainingLifes);

		/*	\fn SendAsteroidDestroyPacket
		\brief	Notify to the clients that one asteroid was destroyed safely
		*/
		void SendAsteroidDestroyPacket(unsigned short objectID);

		/*	\fn SendBulletToAllClients
		\brief	Notify to the clients that one bullet was created safely
		*/
		void SendBulletToAllClients(BulletCreationPacket mBullet);

		/*	\fn SendBulletDestroyPacket
		\brief	Notify to the clients that one bullet was destroyed safely
		*/
		void SendBulletDestroyPacket(BulletDestroyPacket& packet);

		/*	\fn sendScorePacket
		\brief	Send the given score packet to all clients
		*/
		void sendScorePacket(ScorePacket _packet);
		
	private:

		/*	\fn ReceivePackets
		\brief	Receive all the packets in the buffer
		*/
		void ReceivePackets();

		/*	\fn SendVoidPackets
		\brief	Sends void packets to avoid having timeout players
		*/
		void SendVoidPackets();

		/*	\fn HandleReceivedPacket
		\brief	Sends void packets to avoid having timeout players
		*/
		void HandleReceivedPacket(ProtocolPacket& packet, Packet_Types type, sockaddr& senderAddress);

		/*	\fn HandleNewPlayerACKPacket
		\brief	At this step the client is finally connected, so notify the rest of the clients
		*/
		void HandleNewPlayerACKPacket(SYNACKPacket& packet, sockaddr& senderAddress);

		/*	\fn CheckTimeoutPlayer
		\brief	Forces disconnection of clients if they time out
		*/
		void CheckTimeoutPlayer();

		/*	\fn NotifyPlayerDisconnection
		\brief	Notify clients of the disconnection of a player
		*/
		void NotifyPlayerDisconnection(unsigned char playerID);

		/*	\fn HandleDisconnection
		\brief	Makes sure the disconnection is happening correctly
		*/
		void HandleDisconnection();

		/*	\fn PrintMessage
		\brief	Prints the given message if verbose is active
		*/
		void PrintMessage(const std::string& msg);

		/*	\fn PrintError
		\brief	Prints the given message and the networking error code
		*/
		void PrintError(const std::string& msg);
	};
}