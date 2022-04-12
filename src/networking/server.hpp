#pragma once
#include "protocol.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace CS260
{
	struct ClientInfo
	{
		sockaddr mEndpoint;
		PlayerInfo mPlayerInfo;
	};
	
	class Server
	{
		Protocol mProtocol;
		bool mVerbose;
		SOCKET mSocket;
		sockaddr_in mEndpoint;
		std::vector<ClientInfo> mClients;


	public:
		/*	\fn Server
		\brief	Server constructor following RAII design
		*/
		Server(bool verbose, const std::string& ip_address, uint16_t port);

		/*	\fn ~Server
		\brief	Server destructor following RAII design
		*/
		~Server();

		void Tick();
		
		int PlayerCount();

	private:

		void ReceivePackets();
		
		void HandleReceivedPacket(ProtocolPacket& packet, Packet_Types type, sockaddr& senderAddress);

		void HandleNewClients();

		void ReceivePlayersInformation();

		/*	\fn SendRST
		\brief	Sends reset message
		*/
		bool SendRST(sockaddr& senderAddress);

		void SendPlayerPositions();

		void PrintMessage(const std::string& msg);
		void PrintError(const std::string& msg);
	};
}