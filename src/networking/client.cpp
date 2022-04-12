#include "client.hpp"
#include "client.hpp"
#include "client.hpp"
#include "client.hpp"
#include "client.hpp"
#include "client.hpp"
#include "client.hpp"

#include "utils.hpp"

namespace CS260
{	/*	\fn Client
	\brief	Client default constructor
	*/
	Client::Client(const std::string& ip_address, uint16_t port, bool verbose)
		:mVerbose(verbose),
		mSocket(0)
	{	

		// Create UDP socket for the client
		mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		// Error checking
		if (mSocket == INVALID_SOCKET)
		{
			PrintMessage("Invalid socket created " + std::to_string(WSAGetLastError()));
			NetworkDestroy();
			throw std::runtime_error("Error allocating socket");
		}
		else
			PrintMessage("Created socket successfully.");

		sockaddr_in remoteEndpoint;

		remoteEndpoint.sin_family = AF_INET;
		remoteEndpoint.sin_addr = CS260::ToIpv4(ip_address);
		remoteEndpoint.sin_port = htons(port);

		// Connect the client socket to the given address to allow calling
		// send and recv with the server being the default receiver and sender
		if (connect(mSocket, reinterpret_cast<sockaddr*>(&remoteEndpoint), sizeof(remoteEndpoint)) == SOCKET_ERROR)
		{
			PrintMessage("Invalid connection" + std::to_string(WSAGetLastError()));
			closesocket(mSocket);
			NetworkDestroy();
			throw std::runtime_error("Error connecting");
		}
		else
			PrintMessage("Connected socket correctly.");

		SetSocketBlocking(mSocket, false);

		mProtocol.SetSocket(mSocket);
		if (ConnectToServer())
		{
			PrintMessage("Connected to server correctly.");
		}
		else
			throw(std::runtime_error("Could not connect to server"));
	}

	/*	\fn ~Client
	\brief	Client destructor
	*/
	Client::~Client()
	{
		DisconnectFromServer();
		closesocket(mSocket);
	
	}

	void Client::Tick()
	{
		// TODO: Reset the counter properly
		mNewPlayersOnFrame.clear();
		//mPlayersState.clear();

		PlayerInfo packet;
		WSAPOLLFD poll;
		poll.fd = mSocket;
		poll.events = POLLIN;

		while (WSAPoll(&poll, 1, timeout) > 0)
		{
			if (poll.revents & POLLERR)
			{
				PrintError("Error polling message");
			}
			else if (poll.revents & (POLLIN | POLLHUP))
			{
				int bytesReceived = ::recv(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);

				if (bytesReceived == SOCKET_ERROR)
				{
					PrintError("Error receiving message");
				}
				else
				{
					// This should be handled playerpacket
					//if (packet.mCode & NEWPLAYER)
					//{
					//	NewPlayerPacket newPacket;
					//	newPacket.mCode = packet.mCode;
					//	newPacket.mID = packet.mID;
					//	mNewPlayersOnFrame.push_back(newPacket);
					//
					//	PlayerInfo newPlayerPacket;
					//	newPlayerPacket.mID = packet.mID;
					//	mPlayersState.push_back(newPlayerPacket);
					//}
					//// This should be handled playerpacket
					//if (packet.mCode & PLAYERINFO)
					//{
					//	for (auto& state : mPlayersState)
					//	{
					//		if (state.mID = packet.mID)
					//		{
					//			state.pos[0] = packet.pos[0];
					//			state.pos[1] = packet.pos[1];
					//			state.rot = packet.rot;
					//		}
					//	}
					//}
				}
			}
		}
	}

	void Client::SendPlayerInfo(glm::vec2 pos, float rotation)
	{
		// TODO: Remove from here
		{
			PlayerInfo packet;
			packet.mID = mID;
			packet.pos[0] = pos.x;
			packet.pos[1] = pos.y;
			packet.rot = rotation;

			// TODO: Handle error
			::send(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
		}
	}

	std::vector<NewPlayerPacket>  Client::GetNewPlayers()
	{
		return mNewPlayersOnFrame;
	}

	std::vector<PlayerInfo> Client::GetPlayersInfo()
	{
		return mPlayersState;
	}

	/*	\fn ConnectToServer
	\brief	Handles connection
	*/
	bool Client::ConnectToServer()
	{
		do
		{
			if (SendSYN())
			{
				ReceiveMessages();
			}
			else
			{
				PrintMessage("[CLIENT] Error sending SYN message.");
				return false;
			}
			// Receive SYNACK

		} while (!mConnected);

		return true;
	}

	/*	\fn SendSYN
	\brief	Sends SYN message
	*/
	bool Client::SendSYN()
	{
		PrintMessage("[CLIENT] Sending connection request SYN ");

		SYNPacket packet;
		mProtocol.SendPacket(Packet_Types::SYN, &packet);
		
		return true;
	}

	void Client::ReceiveMessages()
	{
		WSAPOLLFD poll;
		poll.fd = mSocket;
		poll.events = POLLIN;

		// TODO: Handle timeout 
		while(WSAPoll(&poll, 1, timeout) > 0)
		{
			if (poll.revents & POLLERR)
			{
				PrintError("Polling receiving players information");
			}
			else
			{
				ProtocolPacket packet;
				unsigned size = sizeof(ProtocolPacket);
				Packet_Types type;
				mProtocol.RecievePacket(&packet, &size, &type);
				HandleReceivedMessage(packet, type);				
			}
		}

	}

	void Client::HandleReceivedMessage(ProtocolPacket& packet, Packet_Types type)
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
			break;
		case Packet_Types::SYNACK:
		{
			// Copy the payload into a more manageable structure
			SYNACKPacket receivedPacket;
			::memcpy(&receivedPacket, packet.mBuffer.data(), sizeof(receivedPacket));
			mID = receivedPacket.mPlayerID;
			// TODO: Send connection ACK properly
			mProtocol.SendPacket(SYNACK, &receivedPacket, 0);
		}
			break;
		}
	}

	/*	\fn SendRST
	\brief	Sends a reset request
	*/
	void Client::SendRST()
	{
		//packet.AttachACK(0);
		//packet.SetCode(RSTCODE);
		//int bytesSent = ::send(mSocket, reinterpret_cast<char*>(&packet), sizeof(ConnectionPacket), 0);
		//if (bytesSent == SOCKET_ERROR)
		//{
		//	PrintMessage("[CLIENT] Error sending RST " + std::to_string(WSAGetLastError()));
		//}
		//else
		//	PrintMessage("[CLIENT] Sending RST correctly");
	}
	/*	\fn DisconnectFromServer
	\brief	Handles disconnection
	*/
	void Client::DisconnectFromServer()
	{
	//	packet.SetSequence(0);
	//	packet.AttachACK(0);
	//	auto clock = now();

	//	// Send ACK to FIN 1
	//	PrintMessage("Sending ACK FIN 1");

	//	// We only need to send the header
	//	int bytesSent = ::send(mSocket, reinterpret_cast<char*>(&packet), sizeof(ConnectionPacket), 0);

	//	if (bytesSent == SOCKET_ERROR)
	//	{
	//		PrintMessage("Error sending ACK FIN 1" + std::to_string(WSAGetLastError()));
	//		return;
	//	}
	//	else
	//	{
	//		packet.AttachACK(0);
	//		packet.SetCode(FIN2CODE);
	//		unsigned expectedACK = packet.GetExpectedACK();

	//		PrintMessage("Sending FIN 2");
	//		bytesSent = ::send(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);

	//		if (bytesSent == SOCKET_ERROR)
	//		{
	//			PrintMessage("Error sending FIN 2" + std::to_string(WSAGetLastError()));
	//			return;
	//		}
	//		else
	//		{
	//			bool acknowledged = false;
	//			do
	//			{
	//				WSAPOLLFD poll;
	//				poll.fd = mSocket;
	//				poll.events = POLLIN;

	//				if (WSAPoll(&poll, 1, timeout) > 0)
	//				{
	//					// Receive all messages
	//					if (poll.revents & POLLERR)
	//					{
	//						PrintMessage("Error polling FIN1 or LASTACK message");
	//						return;
	//					}
	//					// We received a message
	//					else if (poll.revents & (POLLIN | POLLHUP))
	//					{
	//						int bytesReceived = ::recv(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
	//						if (bytesReceived == SOCKET_ERROR)
	//						{
	//							PrintError("Error receiving FIN1 or LASTACK message");
	//							return;
	//						}
	//						else
	//						{
	//							// Need to resend ACK to FIN 1
	//							if (packet.GetCode() == FIN1CODE)
	//							{
	//								packet.AttachACK(0);

	//								// Send ACK to FIN 1
	//								PrintMessage("Resending ACK to FIN 1");
	//								bytesSent = ::send(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
	//								if (bytesSent == SOCKET_ERROR)
	//								{
	//									PrintError("Error resending ACK to FIN 1");
	//									return;
	//								}

	//								packet.AttachACK(0);
	//								packet.SetCode(FIN2CODE);
	//								expectedACK = packet.GetExpectedACK();

	//								PrintMessage("Resending FIN 2");
	//								bytesSent = ::send(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
	//								if (bytesSent == SOCKET_ERROR)
	//								{
	//									PrintError("Error resending FIN 2");
	//									return;
	//								}

	//							}
	//							// Received last ACK
	//							else if (packet.GetCode() & LASTACKCODE && expectedACK == packet.GetACK())
	//							{
	//								PrintMessage("Received last ACK");
	//								acknowledged = true;
	//							}
	//						}
	//					}
	//				}
	//				// Timeout of 30 seconds
	//				// if we timeout, we disconnect directly since we could not receive FIN2
	//				// This time the timeout is much bigger since each FIN1 send attempt
	//				// will have 5 attempts and each one will take as much 5 seconds
	//				// which makes a total of minimum 25 seconds
	//				// We give a little more to avoid issues
	//			} while (!acknowledged && ms_since(clock) < 30000);
	//		}
	//	}
	}

	void Client::PrintMessage(const std::string& msg)
	{
		if (mVerbose)
			std::cerr << "[CLIENT] " << msg << std::endl;
	}

	void Client::PrintError(const std::string& msg)
	{
		std::cerr << "[CLIENT] Error  " << std::to_string(WSAGetLastError()) << " at " << msg << std::endl;
	}

}