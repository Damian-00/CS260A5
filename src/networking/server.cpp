#include "server.hpp"
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
		mDead(false)
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

		srand(time(0));
	}

	Server::~Server()
	{
		closesocket(mSocket);
	}

	void Server::Tick()
	{
		// Update the timer that updates the asteroids
		mUpdateAsteroidsTimer += tickRate;
		
		// Update Keep alive timer
		for (auto& client : mClients)
			client.mAliveTimer += tickRate;
		
		mDisconnectedPlayersIDs.clear();
		mNewPlayersOnFrame.clear();
		
		// Handle all the receive packets
		ReceivePackets();

		// Resend unacknowledged messages if necessary
		mProtocol.Tick();
		
		// This is to avoid having the clients disconnecting while they are playing by their own
		SendVoidPackets();

		CheckTimeoutPlayer();

		HandleDisconnection();
		

		// TOOD: We may start the game with less than 4 players
		// For the moment, wait untill there are 4 players connected
		//if (mClients.size() < 2)
		//{
		//	HandleNewClients();
		//}
		//// Handle game state
		//else
		//{ 
		//	// Send player positions to all clients
		//	ReceivePlayersInformation();
		//	SendPlayerPositions();
		//}
	}

	int Server::PlayerCount()
	{
		return mClients.size();
	}

	std::vector<NewPlayerPacket> Server::GetNewPlayers()
	{
		return mNewPlayersOnFrame;
	}

	std::vector<ClientInfo> Server::GetPlayersInfo()
	{
		return mClients;
	}

	const std::vector<unsigned char>& Server::GetDisconnectedPlayersIDs()
	{
		return mDisconnectedPlayersIDs;
	}

	void Server::SendPlayerInfo(sockaddr _endpoint, PlayerInfo _playerinfo)
	{
		ShipUpdatePacket mPacket;
		mPacket.mPlayerInfo = _playerinfo;

		//PrintMessage("Sending player info with id " + std::to_string(static_cast<int>(_playerinfo.mID)));
		mProtocol.SendPacket(Packet_Types::ShipPacket, &mPacket, &_endpoint);
	}

	void Server::SendAsteroidCreation(unsigned short id, glm::vec2 position, glm::vec2 velocity, float scale, float angle)
	{
		AsteroidCreationPacket packet;
		packet.mObjectID = id;
		packet.mScale = scale;
		packet.mAngle = angle;
		packet.mPosition = position;
		packet.mVelocity = velocity;

		for (auto& client : mClients)
			mProtocol.SendPacket(Packet_Types::AsteroidCreation, &packet, &client.mEndpoint);
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

	void Server::SendAsteroidsUpdate()
	{
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
				client.mDead = true;

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
				ProtocolPacket packet{};
				Packet_Types type{};
				sockaddr senderAddress;
				unsigned size = 0;

				mProtocol.RecievePacket(&packet, &size, &type, &senderAddres);
				HandleReceivedPacket(packet, type, senderAddres);
			}
		}
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
		case Packet_Types::VoidPacket:
			break;
		case Packet_Types::ObjectCreation:
			break;
		case Packet_Types::ObjectDestruction:
			break;
		case Packet_Types::ObjectUpdate:
			break;
		case Packet_Types::ShipPacket:

			if (!mClients.empty())
			for (auto& i : mClients) {
				if (ShipUpdatePacket * mCastedPacket = reinterpret_cast<ShipUpdatePacket*>(&packet)) {
					if (i.mPlayerInfo.mID == mCastedPacket->mPlayerInfo.mID) {
						i.mPlayerInfo = mCastedPacket->mPlayerInfo;
						i.mAliveTimer = 0;
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
				
				mProtocol.SendPacket(Packet_Types::PlayerDisconnect, &packet, &discconectClient.mEndpoint);

				mDisconnectedPlayersIDs.push_back(discconectClient.mPlayerInfo.mID);
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