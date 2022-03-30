#pragma once
#include "networking.hpp"

#include <vector>

namespace CS260
{

	const unsigned char SYNCODE = 0b1;
	const unsigned char SYNACKCODE = 0b10;
	const unsigned char ACKCODE = 0b1000;
	const unsigned char RSTCODE = 0b1000000;

	struct ClientInfo
	{
		unsigned char mID;
		sockaddr mEndpoint;
	};
	struct ConnectionPacket
	{
		unsigned mACK;
		unsigned char mCode;		
	};

	class Server
	{
		bool mVerbose;
		SOCKET mSocket;
		sockaddr_in mEndpoint;
		std::vector<ClientInfo> mClients;


	public:
		/*	\fn Server
		\brief	Server constructor following RAII design
		*/
		Server(bool verbose);

		/*	\fn ~Server
		\brief	Server destructor following RAII design
		*/
		~Server();

		void Tick();
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



		void PrintMessage(const std::string& msg);
		void PrintError(const std::string& msg);
	};
}