/*!
******************************************************************************
	\file    dftp_client.hpp
	\author  David Miranda
	\par     DP email: m.david@digipen.edu
	\par     Course: CS260
	\date    03/17/2022

	\brief
	Implementation for all the functionalities required by the client.
*******************************************************************************/
#pragma once

#include "protocol.hpp"

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <map>

namespace CS260
{
	class Client
	{
		Protocol mProtocol;
		unsigned char mID;
		bool mConnected;
		bool mVerbose;
		bool mClose;
		unsigned mKeepAliveTimer;
		SOCKET mSocket;
		std::vector<NewPlayerPacket> mNewPlayersOnFrame;
		std::vector<PlayerInfo> mPlayersState;
		std::vector<unsigned char> mDisconnectedPlayersIDs;
		std::vector<AsteroidCreationPacket> mAsteroidsCreated;
		std::vector<AsteroidUpdatePacket> mAsteroidsUpdate;
		std::vector<AsteroidDestructionPacket> mAsteroidsDestroyed;
		std::vector<PlayerDiePacket> mPlayersDied;
		std::vector<BulletDestroyPacket> mBulletsToDestroy;
		glm::vec4 mColor;
		std::vector<BulletCreationPacket> mBulletsToCreate;

		std::vector<ScorePacket> mScorePacketsToHandle;
		
	public:

		/*	\fn Client
		\brief	Client default constructor
		*/
		Client(const std::string& ip_address, uint16_t port, bool verbose);

		/*	\fn ~Client
		\brief	Client destructor
		*/
		~Client();

		void Tick();

		void SendPlayerInfo(glm::vec2 pos, glm::vec2 vel, float rotation, bool input );

		std::vector<NewPlayerPacket> GetNewPlayers();

		std::vector<PlayerInfo> GetPlayersInfo();

		bool Connected();

		bool ShouldClose();

		void NotifyDisconnection();

		unsigned char GetPlayerID();

		const std::vector<unsigned char>& GetDisconnectedPlayersIDs();

		const std::vector<AsteroidCreationPacket>& GetCreatedAsteroids();
		
		const std::vector<AsteroidUpdatePacket>& GetUpdatedAsteroids();
		
		const std::vector<AsteroidDestructionPacket>& GetDestroyedAsteroids();
		
		const std::vector<PlayerDiePacket>& GetDiedPlayers();
		
		const std::vector<BulletDestroyPacket>& GetBulletsDestroyed();

		glm::vec4 GetColor();

		void RequestBullet(unsigned mOwnerID, glm::vec2 vel, glm::vec2 pos , float dir);

		std::vector<BulletCreationPacket> GetBulletsToCreate();

		std::vector<ScorePacket> GetScorePacketsToHandle();



	private:
		/*	\fn ConnectToServer
		\brief	Handles connection
		*/
		bool ConnectToServer();

		/*	\fn SendSYN
		\brief	Sends SYN message
		*/
		bool SendSYN();
		
		void ReceiveMessages();

		void HandleTimeOut();
		
		void HandleReceivedMessage(ProtocolPacket& packet, Packet_Types type);

		/*	\fn SendRST
		\brief	Sends a reset request
		*/
		void SendRST();
		
		/*	\fn DisconnectFromServer
		\brief	Handles disconnection
		*/
		void DisconnectFromServer();

		void PrintMessage(const std::string& msg);
		
		void PrintError(const std::string& msg);
		

		
	};
}