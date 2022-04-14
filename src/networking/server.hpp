#pragma once
#include "protocol.hpp"

#include <glm/glm.hpp>

#include <vector>
#include "protocol.hpp"

namespace CS260
{
	struct ClientInfo
	{
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
	};
	
	const unsigned disconnectTries = 3; // In fact, there is a total of 4 tries because the first one is not counted

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

		const std::vector<unsigned char>& GetDisconnectedPlayersIDs();

		void SendPlayerInfo(sockaddr _endpoint, PlayerInfo _playerinfo);

	private:

		void ReceivePackets();

		void SendVoidPackets();
		
		void HandleReceivedPacket(ProtocolPacket& packet, Packet_Types type, sockaddr& senderAddress);

		void HandleNewPlayerACKPacket(SYNACKPacket& packet, sockaddr& senderAddress);

		void CheckTimeoutPlayer();

		void NotifyPlayerDisconnection(unsigned char playerID);

		void HandleDisconnection();

		/*	\fn SendRST
		\brief	Sends reset message
		*/
		bool SendRST(sockaddr& senderAddress);

		void SendPlayerPositions();

		void PrintMessage(const std::string& msg);
		void PrintError(const std::string& msg);
	};
}