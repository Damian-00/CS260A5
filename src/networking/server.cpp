#include "server.hpp"
#include "server.hpp"
#include "server.hpp"
#include "server.hpp"
#include "server.hpp"
#include "server.hpp"

#include "utils.hpp"

namespace CS260
{
	Server::Server(bool verbose, const std::string& ip_address, uint16_t port) :
	mVerbose(verbose)
	{
		// Initialize the networking library
		NetworkCreate();

		// Create UDP socket for the Server
		mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (mSocket == INVALID_SOCKET)
		{
			CS260::NetworkDestroy();
			PrintError("Creating server socket");
			throw std::runtime_error("Error creating server socket");
		}
		else
			PrintMessage("Created socket successfully.");

		mEndpoint.sin_family = AF_INET;
		mEndpoint.sin_addr = CS260::ToIpv4(ip_address);
		mEndpoint.sin_port = htons(port);

		// Bind the server socket
		if (bind(mSocket, reinterpret_cast<sockaddr*>(&mEndpoint), sizeof(mEndpoint)) == SOCKET_ERROR)
		{
			closesocket(mSocket);
			CS260::NetworkDestroy();
			PrintError("Binding socket");
			throw std::runtime_error("Error binding server socket");
		}
		else
			PrintMessage("Bound socket.");

		SetSocketBlocking(mSocket, false);
	}

	Server::~Server()
	{
		closesocket(mSocket);
		NetworkDestroy();
	}

	void Server::Tick()
	{
		// Handle all the receive packets
		

		// TOOD: We may start the game with less than 4 players
		// For the moment, wait untill there are 4 players connected
		if (mClients.size() < 2)
		{
			HandleNewClients();
		}
		// Handle game state
		else
		{ 
			// Send player positions to all clients
			ReceivePlayersInformation();
			SendPlayerPositions();
		}
	}

	int Server::PlayerCount()
	{
		return mClients.size();
	}

	void Server::ReceivePackets()
	{
		sockaddr senderAddres;
		socklen_t size = sizeof(senderAddres);
		PlayerInfo packet;
		WSAPOLLFD poll;
		poll.fd = mSocket;
		poll.events = POLLIN;

		if (WSAPoll(&poll, 1, timeout) > 0)
		{
			if (poll.revents & POLLERR)
			{
				PrintError("Polling receiving players information");
			}
			else
			{
				ProtocolPacket packet{};
				Packet_Types type{};
				sockaddr senderAddress;
				unsigned size = 0;

				mProtocol.RecievePacket(&packet, &size, &type, &senderAddres);
				HandleReceivedPacket(packet, type, senderAddres);
			}
		}
	}

	void Server::HandleReceivedPacket(ProtocolPacket& packet, Packet_Types type, sockaddr& senderAddress)
	{
		switch (type) 
		{
		case Packet_Types::VoidPacket:
			break;
		case Packet_Types::ObjectCreation:
			break;
		case Packet_Types::ObjectDestruction:
			break;
		case Packet_Types::ObjectUpdate:
			break;
		case Packet_Types::ShipPacket:
			break;
		case Packet_Types::SYN:
		{
			SYNACKPacket SYNACKpacket;
			SYNACKpacket.mPlayerID = mClients.size();
			mProtocol.SendPacket(Packet_Types::SYNACK, &SYNACKpacket, 0, &senderAddress);
		}			
			break;
		case Packet_Types::SYNACK:
		{
			// Copy the payload into a more manageable structure
			SYNACKPacket receivedPacket;
			::memcpy(&receivedPacket, packet.mBuffer.data(), sizeof(receivedPacket));
			mClients.push_back(ClientInfo{ senderAddress, PlayerInfo{receivedPacket.mPlayerID, {0,0}, 0 } });
			break;
		}
		}
	}

	void Server::HandleNewClients()
	{
		PrintMessage("Accepting new client");

		sockaddr senderAddress;
		//if(ReceiveSYN(senderAddress, packet))
		//	if (SendSYNACK(senderAddress, packet))
		//		if (ReceiveACKFromSYNACK(senderAddress, packet))
				{
					NewPlayerPacket newPlayer;
					newPlayer.mCode = NEWPLAYER;
					newPlayer.mID = mClients.size();
					// TODO: Notify the rest of the clients of the new client relialably
					for (auto& client : mClients)
					{
						::sendto(mSocket, reinterpret_cast<char*>(&newPlayer), sizeof(NewPlayerPacket),0, &client.mEndpoint, sizeof(client.mEndpoint));
						// TODO: Handle error checking
					}
					// TODO: Notify the new client of the previus clients relialably
					for (auto& client : mClients)
					{
						newPlayer.mID = client.mPlayerInfo.mID;
						::sendto(mSocket, reinterpret_cast<char*>(&newPlayer), sizeof(NewPlayerPacket), 0, &senderAddress, sizeof(senderAddress));
						// TODO: Handle error checking
					}
					mClients.push_back(ClientInfo{ senderAddress, static_cast<unsigned char>(mClients.size())});
				}
	}

	void Server::ReceivePlayersInformation()
	{
		sockaddr senderAddres;
		socklen_t size = sizeof(senderAddres);
		PlayerInfo packet;
		WSAPOLLFD poll;
		poll.fd = mSocket;
		poll.events = POLLIN;
		
		if (WSAPoll(&poll, 1, timeout) > 0)
		{
			if (poll.revents & POLLERR)
			{
				PrintError("Polling receiving players information");
			}
			else
			{
				// Receive ACK of SYNACK
				int bytesReceived = ::recvfrom(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0, &senderAddres, &size);
				if (bytesReceived == SOCKET_ERROR)
				{
					PrintError("Receiving message.");
				}
				// Make sure it is an ACK code
				///if (packet.mCode & PLAYERINFO)
				{
					for (auto& client : mClients)
					{
						if (client.mPlayerInfo.mID == packet.mID)
						{
							client.mPlayerInfo.pos[0] = packet.pos[0];
							client.mPlayerInfo.pos[1] = packet.pos[1];

							client.mPlayerInfo.rot = packet.rot;
						}
					}
				}
			}
		}
	}


	/*	\fn SendRST
	\brief	Sends reset message
	*/
	bool Server::SendRST(sockaddr& senderAddress)
	{
		//packet.AttachACK(0);
		//packet.SetCode(RSTCODE);
		//int size = sizeof(senderAddress);
		//int bytesReceived = ::sendto(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0, &senderAddress, size);
		//if (bytesReceived == SOCKET_ERROR)
		//{
		//	PrintError("Error sending RST");
		//}
		//else
		//	PrintMessage("[SERVER] Sending RST correctly");
		//
		return false;
	}

	void Server::SendPlayerPositions()
	{
		for (auto& senderClient : mClients)
		{
			for (auto& receiverClient : mClients)
			{
				// Avoid sending to itself
				if (senderClient.mPlayerInfo.mID != receiverClient.mPlayerInfo.mID)
				{
					//senderClient.mPlayerInfo.mCode = PLAYERINFO;
					auto bytesReceived = ::sendto(mSocket, reinterpret_cast<char*>( &senderClient.mPlayerInfo), sizeof(PlayerInfo), 0, &receiverClient.mEndpoint, sizeof(receiverClient.mEndpoint));
					if(bytesReceived == SOCKET_ERROR)
					{
						// TODO: Handle error
					}
				}
			}
		}
	}

	void Server::PrintMessage(const std::string& msg)
	{
		if (mVerbose)
			std::cerr << "[SERVER] " << msg << std::endl;
	}

	void Server::PrintError(const std::string& msg)
	{
		std::cerr << "[SERVER] Error  " << std::to_string(WSAGetLastError()) << " at " << msg << std::endl;
	}

}