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
#include <string>
#include <vector>

#include "networking.hpp"
#include "protocol.hpp"

namespace CS260
{
	class Client
	{
		bool mVerbose;
		SOCKET mSocket;
		std::vector<NewPlayerPacket> mNewPlayersOnFrame;
		Protocol mProtocol;
		
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

		std::vector<NewPlayerPacket> GetNewPlayers();

	private:
		/*	\fn ConnectToServer
		\brief	Handles connection
		*/
		bool ConnectToServer();

		/*	\fn SendSYN
		\brief	Sends SYN message
		*/
		bool SendSYN(ConnectionPacket& packet);

		/*	\fn ReceiveSYNACK
		\brief	Receives SYNACK
		*/
		bool ReceiveSYNACK(ConnectionPacket& packet);

		/*	\fn SendRST
		\brief	Sends a reset request
		*/
		void SendRST(ConnectionPacket& packet);
		
		/*	\fn DisconnectFromServer
		\brief	Handles disconnection
		*/
		void DisconnectFromServer();

		void PrintMessage(const std::string& msg);
		
		void PrintError(const std::string& msg);
	};
}