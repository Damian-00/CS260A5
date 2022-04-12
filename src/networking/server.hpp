#pragma once
#include "networking.hpp"

#include <vector>
#include "protocol.hpp"

namespace CS260
{
	struct ClientInfo
	{
		sockaddr mEndpoint;
		unsigned char mID;
		float pos[2];
	};
	
	class Server
	{
		bool mVerbose;
		SOCKET mSocket;
		sockaddr_in mEndpoint;
		std::vector<ClientInfo> mClients;
		Protocol mProtocol;


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

		void HandleNewClients();

		/*	\fn ReceiveSYN
		\brief	Receive SYN message
		*/
		bool ReceiveSYN(sockaddr& senderAddress, ConnectionPacket& packet);

		/*	\fn SendSYNACK
		\brief	Sends SYNACK message
		*/
		bool SendSYNACK(sockaddr& senderAddress, ConnectionPacket& packet);

		/*	\fn ReceiveACKFromSYNACK
		\brief	Receive ACK from SYNACK
		*/
		bool ReceiveACKFromSYNACK(sockaddr& senderAddress, ConnectionPacket& packet);

		/*	\fn SendRST
		\brief	Sends reset message
		*/
		bool SendRST(sockaddr& senderAddress, ConnectionPacket& packet);

		void SendPlayerPositions();

		void PrintMessage(const std::string& msg);
		void PrintError(const std::string& msg);
	};
}