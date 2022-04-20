#include "server.hpp"
/*!
******************************************************************************
	\file    server.cpp
	\author  David Miranda
	\par     DP email: m.david@digipen.edu
	\par     Course: CS260
	\date    04/20/2022

	\brief
	Implementation for all the functionalities required by the server.
*******************************************************************************/

#include "server.hpp"

#include "utils.hpp"

namespace CS260
{
	ClientInfo::ClientInfo(sockaddr endpoint, PlayerInfo playerInfo, glm::vec4 col):
		mEndpoint(endpoint),
		mAliveTimer(0),
		mDisconnecting(false),
		mDisconnectTimeout(0),
		mDisconnectTries(0),
		mPlayerInfo(playerInfo),
		color(col),
		mDead(false),
		mRemainingLifes(3)
	{
	}

	Server::Server(bool verbose, const std::string& ip_address, uint16_t port) :
	mVerbose(verbose)
	{
		mCurrentID = rand() % 255 + 1;
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

		mProtocol.SetSocket(mSocket);

		srand(static_cast<unsigned int>(time(0)));
	}

	Server::~Server()
	{
		closesocket(mSocket);
	}

	void Server::Tick()
	{
		// Update the timer that updates the asteroids
		// Below, when needed it will send the current state of the asteroids to the clients
		mUpdateAsteroidsTimer += tickRate;
		
		// Update Keep alive timer
		for (auto& client : mClients)
			client.mAliveTimer += tickRate;
		
		// Clear all the vectors that are used to send data to the clients
		// To fill them again if needed below
		mNewPlayersOnFrame.clear();
		mDisconnectedPlayersIDs.clear();
		mBulletsToCreate.clear();

		
		// Handle all the receive packets
		ReceivePackets();

		// Resend unacknowledged messages if necessary
		mProtocol.Tick();
		
		// This is to avoid having the clients disconnecting while they are playing by their own
		SendVoidPackets();

		// Disconnect players if their timer runs out
		CheckTimeoutPlayer();

		// Handle save disconnection with clients
		HandleDisconnection();
	}

	int Server::PlayerCount()
	{
		return static_cast<int>(mClients.size());
	}

	const std::vector<NewPlayerPacket>& Server::GetNewPlayers()
	{
		return mNewPlayersOnFrame;
	}

	const std::vector<ClientInfo>& Server::GetPlayersInfo()
	{
		return mClients;
	}

	const std::vector<unsigned char>& Server::GetDisconnectedPlayersIDs()
	{
		return mDisconnectedPlayersIDs;
	}

	const std::vector<BulletRequestPacket>& Server::GetBulletsToCreate()
	{
		return mBulletsToCreate;
	}

	void Server::SendPlayerInfo(sockaddr _endpoint, PlayerInfo _playerinfo)
	{
		ShipUpdatePacket mPacket;
		mPacket.mPlayerInfo = _playerinfo;
		mProtocol.SendPacket(Packet_Types::ShipPacket, &mPacket, &_endpoint);
	}

	void Server::SendAsteroidCreation(unsigned short id, glm::vec2 position, glm::vec2 velocity, float scale, float angle)
	{
		// Store all the required information to create an asteroid
		AsteroidCreationPacket packet;
		packet.mObjectID = id;
		packet.mScale = scale;
		packet.mAngle = angle;
		packet.mPosition = position;
		packet.mVelocity = velocity;

		// Send the packet to all clients safely, the protocol will take care of it
		for (auto& client : mClients)
			mProtocol.SendPacket(Packet_Types::AsteroidCreation, &packet, &client.mEndpoint);
		
		// Insert the current asteroid information in the server copy of the alive asteroids list
		mAliveAsteroids.push_back(packet);
	}

	void Server::UpdateAsteroid(unsigned short id, glm::vec2 position, glm::vec2 velocity)
	{
		for (auto& asteroid : mAliveAsteroids)
		{
			if (asteroid.mObjectID == id)
			{
				asteroid.mPosition = position;
				asteroid.mVelocity = velocity;
				return;
			}
		}
	}

	void Server::SendAsteroidsForcedUpdate(unsigned id, glm::vec2 pos, glm::vec2 vel)
	{
		AsteroidUpdatePacket packet;
		packet.mID = id;
		packet.mPosition = pos;
		packet.mVelocity = vel;

		for (auto& client : mClients)
			mProtocol.SendPacket(Packet_Types::AsteroidUpdate, &packet, &client.mEndpoint);
	}

	void Server::SendAsteroidsUpdate()
	{
		// The timer updated in the tick function will be used in here
		if (mUpdateAsteroidsTimer > updateAsteroids)
		{
			for (auto& asteroid : mAliveAsteroids)
			{
				AsteroidUpdatePacket packet;
				packet.mID = asteroid.mObjectID;
				packet.mPosition = asteroid.mPosition;
				packet.mVelocity = asteroid.mVelocity;

				for (auto& client : mClients)
				{
					mProtocol.SendPacket(Packet_Types::AsteroidUpdate, &packet, &client.mEndpoint);
				}
			}
			mUpdateAsteroidsTimer = 0;
		}
	}

	void Server::SendPlayerDiePacket(unsigned char playerID, unsigned short remainingLifes)
	{
		PlayerDiePacket packet;
		packet.mPlayerID = playerID;
		packet.mRemainingLifes = remainingLifes;
		
		for (auto& client : mClients)
		{
			// Update the client dead state
			if (client.mPlayerInfo.mID == playerID)
			{
				client.mDead = true;
				client.mRemainingLifes = remainingLifes;
			}

			mProtocol.SendPacket(Packet_Types::PlayerDie, &packet, &client.mEndpoint);
		}
	}

	void Server::SendAsteroidDestroyPacket(unsigned short objectID)
	{
		mAliveAsteroids.erase(std::find_if(mAliveAsteroids.begin(), mAliveAsteroids.end(), [objectID](AsteroidCreationPacket& asteroid) { return asteroid.mObjectID == objectID; }));
		
		AsteroidDestructionPacket packet;
		packet.mObjectId = objectID;
		for (auto& client : mClients)
			mProtocol.SendPacket(Packet_Types::AsteroidDestroy, &packet, &client.mEndpoint);
	}

	void Server::SendBulletToAllClients(BulletCreationPacket mBullet)
	{
		for (auto& client : mClients)
			mProtocol.SendPacket(Packet_Types::BulletCreation, &mBullet, &client.mEndpoint);
	}

	void Server::SendBulletDestroyPacket(BulletDestroyPacket& packet)
	{
		for (auto& client : mClients)
			mProtocol.SendPacket(Packet_Types::BulletDestruction, &packet, &client.mEndpoint);
	}

	void Server::ReceivePackets()
	{
		sockaddr senderAddres;
		socklen_t size = sizeof(senderAddres);
		PlayerInfo packet;
		WSAPOLLFD poll;
		poll.fd = mSocket;
		poll.events = POLLIN;

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
				Packet_Types type;
				sockaddr senderAddress;
				unsigned size = 0;

				mProtocol.ReceivePacket(&packet, &size, &type, &senderAddres);
				HandleReceivedPacket(packet, type, senderAddres);
			}
		}
	}
	
	void Server::sendScorePacket(ScorePacket _packet)
	{
		for (auto& client : mClients)
			mProtocol.SendPacket(Packet_Types::ScoreUpdate, &_packet, &client.mEndpoint);
	}
	
	void Server::SendVoidPackets()
	{
		for (auto& client : mClients)
		{
			mProtocol.SendPacket(Packet_Types::VoidPacket, 0, &client.mEndpoint);
		}
	}
	
	void Server::HandleReceivedPacket(ProtocolPacket& packet, Packet_Types type, sockaddr& senderAddress)
	{
		switch (type) 
		{
		case Packet_Types::ShipPacket:
		{
			if (!mClients.empty())
			{
				for (auto& i : mClients)
				{
					if (ShipUpdatePacket* mCastedPacket = reinterpret_cast<ShipUpdatePacket*>(&packet))
					{
						if (i.mPlayerInfo.mID == mCastedPacket->mPlayerInfo.mID)
						{
							i.mPlayerInfo = mCastedPacket->mPlayerInfo;
							i.mAliveTimer = 0;
						}
					}
				}
			}
		}
			break;
		case Packet_Types::SYN:
		{
			PrintMessage("Received SYNACK");
			SYNACKPacket SYNACKpacket;
			SYNACKpacket.mPlayerID = mCurrentID++;

			glm::vec4 color(((double)rand() / (RAND_MAX)), ((double)rand() / (RAND_MAX)), ((double)rand() / (RAND_MAX)), 1.0f); 

			//avoid colors being too dark
			for (int i = 0; i < 3; i++) 
			{
				if (color[i] < 0.5) 
				{
					color[i] = 0.5;
				}
			}

			SYNACKpacket.color = color;

			PrintMessage("Sending SYNACK to client with id " + std::to_string(static_cast<int>(SYNACKpacket.mPlayerID)));
			mProtocol.SendPacket(Packet_Types::SYNACK, &SYNACKpacket, &senderAddress);
		}			
			break;
		case Packet_Types::SYNACK:
		{
			// Copy the payload into a more manageable structure
			SYNACKPacket receivedPacket;
			::memcpy(&receivedPacket, packet.mBuffer.data(), sizeof(receivedPacket));
			HandleNewPlayerACKPacket(receivedPacket, senderAddress);
			break;
		}
		case Packet_Types::PlayerDisconnect:
		{
			PlayerDisconnectPacket receivedPacket;
			::memcpy(&receivedPacket, packet.mBuffer.data(), sizeof(receivedPacket));
			
			PlayerDisconnectACKPacket disconnectPacket;
			mProtocol.SendPacket(Packet_Types::ACKDisconnect, &disconnectPacket, &senderAddress);
			
			for (auto& client : mClients)
			{
				if (client.mPlayerInfo.mID == receivedPacket.mPlayerID)
				{
					client.mDisconnecting = true;
					client.mAliveTimer = 0;
				}
			}
			break;
		}
		// We received the last disconnection ACK so remove the client from the list
		case Packet_Types::ACKDisconnect:
		{
			PlayerDisconnectACKPacket receivedPacket;
			::memcpy(&receivedPacket, packet.mBuffer.data(), sizeof(receivedPacket));

			// Remove the client from the list
			mClients.erase(std::remove_if(mClients.begin(), mClients.end(), [&receivedPacket](ClientInfo& client)
				{ 
					return client.mPlayerInfo.mID == receivedPacket.mPlayerID;
				}),
				mClients.end());
			mDisconnectedPlayersIDs.push_back(receivedPacket.mPlayerID);
			
			NotifyPlayerDisconnection(receivedPacket.mPlayerID);
			break;
		}
		// We received the last disconnection ACK so remove the client from the list
		case Packet_Types::PlayerDie:
		{
			PlayerDiePacket receivedPacket;
			::memcpy(&receivedPacket, packet.mBuffer.data(), sizeof(receivedPacket));
			
			// When receiving a player die packet message from a client it means
			// that it already received the die packet and is ready to keep playing
			for (auto& client : mClients)
			{
				if (client.mPlayerInfo.mID == receivedPacket.mPlayerID)
					client.mDead = false;
			}				
			break;
		}
		case Packet_Types::BulletRequest:
		{

			BulletRequestPacket rcpk;
			size_t size = sizeof(BulletRequestPacket);

			::memcpy(&rcpk, packet.mBuffer.data(), size);

			mBulletsToCreate.push_back(rcpk); // add it to the vector for the state_ingame to generate the bullets afterwards

			break;
		}
		}
	}

	void Server::HandleNewPlayerACKPacket(SYNACKPacket& packet, sockaddr& senderAddress)
	{
		NewPlayerPacket newPlayerPacket;
		newPlayerPacket.mPlayerInfo.mID = packet.mPlayerID;
		newPlayerPacket.mPlayerInfo.pos = { 0,0 };
		newPlayerPacket.mPlayerInfo.rot = 0;
		newPlayerPacket.color = packet.color;
		newPlayerPacket.mRemainingLifes = 3;
		
		mNewPlayersOnFrame.push_back(newPlayerPacket);

		PrintMessage("Notifying current clients of new player");
		PrintMessage("New client id" + std::to_string(static_cast<int>(packet.mPlayerID)));

		// Send to the current clients the information of the new client
		for (auto& client : mClients)
			mProtocol.SendPacket(Packet_Types::NewPlayer, &newPlayerPacket, &client.mEndpoint);

		PrintMessage("Sending current clients information");

		// Send to the new client the information of the current clients
		for (auto& client : mClients)
		{
			PrintMessage("Sending client with id" + std::to_string(static_cast<int>(client.mPlayerInfo.mID)));
			newPlayerPacket.mPlayerInfo.mID = client.mPlayerInfo.mID;
			newPlayerPacket.mPlayerInfo.pos = client.mPlayerInfo.pos;
			newPlayerPacket.mPlayerInfo.rot = client.mPlayerInfo.rot;
			newPlayerPacket.color = client.color;
			newPlayerPacket.mRemainingLifes = client.mRemainingLifes;
			mProtocol.SendPacket(Packet_Types::NewPlayer, &newPlayerPacket, &senderAddress);
		}

		// Send to the new client the information of the current asteroids
		for (auto& asteroid : mAliveAsteroids)
			mProtocol.SendPacket(Packet_Types::AsteroidCreation, &asteroid, &senderAddress);

		newPlayerPacket.mPlayerInfo.mID = packet.mPlayerID;
		newPlayerPacket.mPlayerInfo.pos = { 0,0 };
		newPlayerPacket.mPlayerInfo.rot = 0;

		// Add the new client to the players list
		mClients.push_back(ClientInfo(senderAddress, newPlayerPacket.mPlayerInfo, packet.color));
	}
	
	void Server::CheckTimeoutPlayer()
	{
		for (auto& discconectClient : mClients)
		{
			if (discconectClient.mAliveTimer > timeOutTimer)
			{
				PlayerDisconnectPacket packet;
				packet.mPlayerID = discconectClient.mPlayerInfo.mID;
				
				// Force the client to disconnect
				mProtocol.SendPacket(Packet_Types::PlayerDisconnect, &packet, &discconectClient.mEndpoint);

				// Update the game state
				mDisconnectedPlayersIDs.push_back(discconectClient.mPlayerInfo.mID);

				// Notify the rest of the clients of this player's disconnection
				NotifyPlayerDisconnection(discconectClient.mPlayerInfo.mID);
			}
		}
		// Remove the client from the list
		mClients.erase(std::remove_if(mClients.begin(), mClients.end(), [](ClientInfo& client) 
			{ return client.mAliveTimer > timeOutTimer; }),
			mClients.end());
	}

	void Server::NotifyPlayerDisconnection(unsigned char playerID)
	{
		PlayerDisconnectPacket packet;
		packet.mPlayerID = playerID;

		for (auto& client : mClients)
		{
			if(client.mPlayerInfo.mID != playerID)
				mProtocol.SendPacket(Packet_Types::NotifyPlayerDisconnection, &packet, &client.mEndpoint);
		}
	}

	void Server::HandleDisconnection()
	{
		for (auto& client : mClients)
		{
			// Check that the client is disconnecting
			if (client.mDisconnecting)
			{
				// Check that we did not exceed the maximum amount of tries for disconnection
				if (client.mDisconnectTries < disconnectTries)
				{
					// If so check if the timeout to send another packet arrived
					if (client.mDisconnectTimeout > timeOutTimer)
					{
						client.mDisconnectTries++;
						PlayerDisconnectACKPacket disconnectPacket;
						mProtocol.SendPacket(Packet_Types::ACKDisconnect, &disconnectPacket, &client.mEndpoint);
					}
					// If not update the timer
					else
						client.mDisconnectTimeout += tickRate;
				}
				// We exceeded the amount of tries, so notify the rest of the clients from the disconnection
				else
				{
					mDisconnectedPlayersIDs.push_back(client.mPlayerInfo.mID);
					NotifyPlayerDisconnection(client.mPlayerInfo.mID);
				}
			}
		}

		mClients.erase(std::remove_if(mClients.begin(), mClients.end(), [](ClientInfo& client)
			{
				return client.mDisconnectTries >= disconnectTries;
			}),
			mClients.end());
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