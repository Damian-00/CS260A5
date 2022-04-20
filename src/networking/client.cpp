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
#include "client.hpp"

#include "utils.hpp"

#include <sstream>

namespace CS260
{	/*	\fn Client
	\brief	Client default constructor
	*/
	Client::Client(const std::string& ip_address, uint16_t port, bool verbose)
		:mVerbose(verbose),
		mSocket(0),
		mConnected(false),
		mClose(false),
		mKeepAliveTimer(0)
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

		//set the endpoint
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

		//set the protocol socket
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
		
		//DisconnectFromServer();
		closesocket(mSocket);
	
	}

	void Client::Tick()
	{
		mKeepAliveTimer += tickRate;
		
		// clear the vectors that refer to the previous tick
		mNewPlayersOnFrame.clear();
		mDisconnectedPlayersIDs.clear();
		mAsteroidsCreated.clear();
		mAsteroidsUpdate.clear();
		mAsteroidsDestroyed.clear();
		mBulletsToDestroy.clear();
		mPlayersDied.clear();
		mScorePacketsToHandle.clear();
		mBulletsToCreate.clear();

		
		ReceiveMessages();

		//resend unacknowledged messages
		mProtocol.Tick();
		
		HandleTimeOut();
		
	}

	void Client::SendPlayerInfo(glm::vec2 pos, glm::vec2 vel,  float rotation, bool input)
	{
		std::stringstream str;
		str << "Sending player info Pos: ";
		str << pos.x;
		str << ", ";
		str << pos.y;
		
		PrintMessage(str.str());
		{
			//send the packet with my ship info
			ShipUpdatePacket myShipPacket;
			myShipPacket.mPlayerInfo.mID = mID;
			myShipPacket.mPlayerInfo.pos = pos;
			myShipPacket.mPlayerInfo.vel = vel;
			myShipPacket.mPlayerInfo.rot = rotation;
			myShipPacket.mPlayerInfo.inputPressed = input;

			mProtocol.SendPacket(Packet_Types::ShipPacket, &myShipPacket, nullptr);
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
		auto clock = now();
		do
		{
			// TODO: Handle timeout 
			while (WSAPoll(&poll, 1, timeout) > 0)
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
					if (mProtocol.ReceivePacket(&packet, &size, &type))
						HandleReceivedMessage(packet, type);
				}
			}
		} while (ms_since(clock) < 10000 && !mConnected);
	}

	void Client::HandleTimeOut()
	{
		// We assume we lost connection with the server because we did not receive any message from it
		// The server is constantly sending messages, so the timer should be updated on each tick
		// If not, something bad happened and we force the application to close
		if (mKeepAliveTimer > timeOutTimer)
		{
			mClose = true;
			PrintMessage("Exiting by timeout of server");
		}
	}

	void Client::HandleReceivedMessage(ProtocolPacket& packet, Packet_Types type)
	{
		mKeepAliveTimer = 0;
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

			ShipUpdatePacket recPacket;
			memcpy(&recPacket, packet.mBuffer.data(), sizeof(recPacket));
			if (mID != recPacket.mPlayerInfo.mID)
			{
				//not my ship, so store the data
				for (auto& i : mPlayersState) 
				{
					if (i.mID == recPacket.mPlayerInfo.mID) 
					{
						i = recPacket.mPlayerInfo;
					}
				}
			}
			break;
		case Packet_Types::SYN:
			break;
		case Packet_Types::SYNACK:
		{
			PrintMessage("Received SYNACK code");
			// Copy the payload into a more manageable structure
			SYNACKPacket receivedPacket;
			::memcpy(&receivedPacket, packet.mBuffer.data(), sizeof(receivedPacket));
			mID = receivedPacket.mPlayerID;
			mColor = receivedPacket.color;
			// TODO: Send connection ACK properly
			mProtocol.SendPacket(SYNACK, &receivedPacket, 0);
			mConnected = true;
		}
			break;
		case Packet_Types::NewPlayer:
		{
			NewPlayerPacket receivedPacket;
			::memcpy(&receivedPacket, packet.mBuffer.data(), sizeof(receivedPacket));
			mNewPlayersOnFrame.push_back(receivedPacket);
			mPlayersState.push_back(receivedPacket.mPlayerInfo);
		}
			break;

		// We were forced to disconnect because the server's time out timer expired
		case Packet_Types::PlayerDisconnect:
		{
			mClose = true;
			PrintMessage("Forced to disconnect by server");
		}
		break;
		case Packet_Types::ACKDisconnect:
		{
			PlayerDisconnectACKPacket sendPacket;
			sendPacket.mPlayerID = mID;
			mProtocol.SendPacket(Packet_Types::ACKDisconnect, &sendPacket);
		}
			break;
			// We were notified that a client was disconnected either by the server or by the client itself
		case Packet_Types::NotifyPlayerDisconnection:
		{
			PlayerDisconnectPacket receivedPacket;
			::memcpy(&receivedPacket, packet.mBuffer.data(), sizeof(receivedPacket));
			mDisconnectedPlayersIDs.push_back(receivedPacket.mPlayerID);
		}
		break;
		case Packet_Types::AsteroidCreation:
		{
			AsteroidCreationPacket receivedPacket;
			::memcpy(&receivedPacket, packet.mBuffer.data(), sizeof(receivedPacket));
			mAsteroidsCreated.push_back(receivedPacket);
		}
			break;
		
		case Packet_Types::BulletCreation:
		{
			BulletCreationPacket receivedPCK;
			::memcpy(&receivedPCK, packet.mBuffer.data(), sizeof(receivedPCK));

			mBulletsToCreate.push_back(receivedPCK);
		}	
			break;
		case Packet_Types::BulletDestruction:
		{
			BulletDestroyPacket receivedPCK;
			::memcpy(&receivedPCK, packet.mBuffer.data(), sizeof(receivedPCK));

			mBulletsToDestroy.push_back(receivedPCK);
		}
			break;
		case Packet_Types::AsteroidUpdate:
		{
			AsteroidUpdatePacket receivedPacket;
			::memcpy(&receivedPacket, packet.mBuffer.data(), sizeof(receivedPacket));
			mAsteroidsUpdate.push_back(receivedPacket);
		}
		break;
		case Packet_Types::AsteroidDestroy:
		{
			AsteroidDestructionPacket receivedPacket;
			::memcpy(&receivedPacket, packet.mBuffer.data(), sizeof(receivedPacket));
			mAsteroidsDestroyed.push_back(receivedPacket);
		}
		break;
		case Packet_Types::PlayerDie:
		{
			PlayerDiePacket receivedPacket;
			::memcpy(&receivedPacket, packet.mBuffer.data(), sizeof(receivedPacket));
			
			if (receivedPacket.mPlayerID == mID)
			{
				PrintMessage("Received player die ourself");
				// Acknowledge the die packet
				mProtocol.SendPacket(Packet_Types::PlayerDie, &receivedPacket);
			}
			else
				PrintMessage("Received player die remote");
			
			auto found = std::find_if(mPlayersDied.begin(), mPlayersDied.end(), [&](auto&playerInfo)
				{
					return playerInfo.mPlayerID == receivedPacket.mPlayerID;
				}
			);
			
			if (found == mPlayersDied.end())
			{
				mPlayersDied.push_back(receivedPacket);
			}
		}
		break;

		case Packet_Types::ScoreUpdate:

			ScorePacket mCastedPack;
			memcpy(&mCastedPack, &packet, sizeof(mCastedPack));
			mScorePacketsToHandle.push_back(mCastedPack);

			break;
		}
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

	bool Client::Connected()
	{
		return mConnected;
	}

	bool Client::ShouldClose()
	{
		return mClose;
	}

	void Client::NotifyDisconnection()
	{
		PlayerDisconnectPacket packet;
		packet.mPlayerID = mID;
		mProtocol.SendPacket(Packet_Types::PlayerDisconnect, &packet);
	}

	unsigned char Client::GetPlayerID()
	{
		return mID;
	}

	const std::vector<unsigned char>& Client::GetDisconnectedPlayersIDs()
	{
		return mDisconnectedPlayersIDs;
	}

	const std::vector<AsteroidCreationPacket>& Client::GetCreatedAsteroids()
	{
		return mAsteroidsCreated;
	}

	const std::vector<AsteroidUpdatePacket>& Client::GetUpdatedAsteroids()
	{
		return mAsteroidsUpdate;
	}

	const std::vector<AsteroidDestructionPacket>& Client::GetDestroyedAsteroids()
	{
		return mAsteroidsDestroyed;
	}

	const std::vector<PlayerDiePacket>& Client::GetDiedPlayers()
	{
		return mPlayersDied;
	}

	const std::vector<BulletDestroyPacket>& Client::GetBulletsDestroyed()
	{
		return mBulletsToDestroy;
	}

	glm::vec4 Client::GetColor()
	{
		return mColor;
	}

	void Client::RequestBullet(unsigned mOwnerID, glm::vec2 vel, glm::vec2 pos, float dir)
	{
		//request the bullet creation to the server
		BulletRequestPacket mPacket;
		mPacket.mOwnerID = mOwnerID;
		mPacket.mPos = pos;
		mPacket.mVel = vel;
		mPacket.mDir = dir;
		mProtocol.SendPacket(Packet_Types::BulletRequest, &mPacket);
	}

	std::vector<BulletCreationPacket> Client::GetBulletsToCreate()
	{

		return mBulletsToCreate;
	}

	std::vector<ScorePacket> Client::GetScorePacketsToHandle()
	{
		return mScorePacketsToHandle;
	}

}