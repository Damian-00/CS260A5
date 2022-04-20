/*!
******************************************************************************
	\file    client.cpp
	\author  David Miranda
	\par     DP email: m.david@digipen.edu
	\par     Course: CS260
	\date    04/20/2022

	\brief
	Implementation for all the functionalities required by the client.
*******************************************************************************/
#pragma once

#include "protocol.hpp"

#include <glm/glm.hpp>
#include <string>
#include <vector>
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

		/**
		* @brief
		* Client update function to receive and handle packets
		*/
		void Tick();

		/**
		* @brief
		* Send the player info of my client
		*/
		void SendPlayerInfo(glm::vec2 pos, glm::vec2 vel, float rotation, bool input );

		/**
		* @brief
		* Retrieve the info of the new players added this frame
		*/
		std::vector<NewPlayerPacket> GetNewPlayers();

		/**
		* @brief
		* Retrieve the info of the players
		*/
		std::vector<PlayerInfo> GetPlayersInfo();

		/**
		* @brief
		* Retrieve whether is connected or not
		*/
		bool Connected();

		/**
		* @brief
		* Retrieve whether it should close after disconnection
		*/
		bool ShouldClose();

		/**
		* @brief
		* Notify my disconnection to the server
		*/
		void NotifyDisconnection();

		/**
		* @brief
		* Retrieve my player ID
		*/
		unsigned char GetPlayerID();

		/**
		* @brief
		* Retrieve the player IDs of the disconnected players this frame
		*/
		const std::vector<unsigned char>& GetDisconnectedPlayersIDs();

		/**
		* @brief
		* Retrieve the asteroids created this frame
		*/
		const std::vector<AsteroidCreationPacket>& GetCreatedAsteroids();

		/**
		* @brief
		* Retrieve the updated asteroids this frame
		*/
		const std::vector<AsteroidUpdatePacket>& GetUpdatedAsteroids();
		
		/**
		* @brief
		* Retrieve the destroyed asteroids this this frame
		*/
		const std::vector<AsteroidDestructionPacket>& GetDestroyedAsteroids();

		/**
		* @brief
		* Retrieve the players who died this frame
		*/
		const std::vector<PlayerDiePacket>& GetDiedPlayers();

		/**
		* @brief
		* Retrieve the bullets destroyed this frame
		*/
		const std::vector<BulletDestroyPacket>& GetBulletsDestroyed();

		/**
		* @brief
		* Retrieve my color 
		*/
		glm::vec4 GetColor();

		/**
		* @brief
		* Ask the server to create a bullet for me
		*/
		void RequestBullet(unsigned mOwnerID, glm::vec2 vel, glm::vec2 pos , float dir);

		/**
		* @brief
		* Retrieve the bullets that need to be created this frame
		*/
		std::vector<BulletCreationPacket> GetBulletsToCreate();

		/**
		* @brief
		* Retrieve the socre packets that need to be handled this frame
		*/
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
		
		/**
		* @brief
		* Receive the packets from the server
		*/
		void ReceiveMessages();

		/**
		* @brief
		* Handle the time out from the server
		*/
		void HandleTimeOut();
		
		/**
		* @brief
		* Handle each packet as its type 
		*/
		void HandleReceivedMessage(ProtocolPacket& packet, Packet_Types type);

		/**
		* @brief
		* Wrapper around for printing
		*/
		void PrintMessage(const std::string& msg);
		
		/**
		* @brief
		* Wrapper around for printing error
		*/
		void PrintError(const std::string& msg);
		

		
	};
}