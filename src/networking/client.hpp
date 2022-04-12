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

namespace CS260
{
	class Client
	{
		Protocol mProtocol;
		unsigned char mID;
		bool mConnected;
		bool mVerbose;
		SOCKET mSocket;
		std::vector<NewPlayerPacket> mNewPlayersOnFrame;
		std::vector<PlayerInfo> mPlayersState;
		
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

		void SendPlayerInfo(glm::vec2 pos, glm::vec2 vel, float rotation);

		std::vector<NewPlayerPacket> GetNewPlayers();

		std::vector<PlayerInfo> GetPlayersInfo();

		bool Connected();


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