#pragma once
#include "protocol.hpp"

#include <glm/glm.hpp>

#include <vector>
#include "protocol.hpp"

namespace CS260
{
	struct ClientInfo
	{
		sockaddr mEndpoint;
		PlayerInfo mPlayerInfo;
	};
	
	class Server
	{
		unsigned char mCurrentID;
		Protocol mProtocol;
		bool mVerbose;
		SOCKET mSocket;
		sockaddr_in mEndpoint;
		std::vector<ClientInfo> mClients;
		std::vector<NewPlayerPacket> mNewPlayersOnFrame;

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

		std::vector<NewPlayerPacket> GetNewPlayers();
		
		std::vector<ClientInfo> GetPlayersInfo();

		void SendPlayerInfo(sockaddr _endpoint, PlayerInfo _playerinfo);

	private:

		void ReceivePackets();
		
		void HandleReceivedPacket(ProtocolPacket& packet, Packet_Types type, sockaddr& senderAddress);

		void HandleNewClients();

		void ReceivePlayersInformation();

		void NotifyNewPlayer();

		/*	\fn SendRST
		\brief	Sends reset message
		*/
		bool SendRST(sockaddr& senderAddress);

		void SendPlayerPositions();

		void PrintMessage(const std::string& msg);
		void PrintError(const std::string& msg);
	};
}