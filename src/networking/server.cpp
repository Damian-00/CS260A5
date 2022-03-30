#include "server.hpp"

namespace CS260
{
	Server::Server(bool verbose):
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
			PrintMessage("Created socket successfully.", verbose);

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
			PrintMessage("Bound socket.", verbose);

		SetSocketBlocking(mSocket, false);
	}
	Server::~Server()
	{
		closesocket(mSocket);
		NetworkDestroy();
	}
	void Server::Tick()
	{
		// TOOD: We may start the game with less than 4 players
		// For the moment, wait untill there are 4 players connected
		if (mClients.size() < 4)
		{
			HandleNewClients();
		}
		// Handle game state
		else
		{ }
	}

	void Server::HandleNewClients()
	{
		PrintMessage("Accepting new client");

		WSAPOLLFD poll;
		poll.fd = mSocket;
		poll.events = POLLIN;
		sockaddr senderAddress;
		socklen_t size = sizeof(senderAddress);

		if (WSAPoll(&poll, 1, timeout) > 0)
		{
			if (poll.revents & POLLERR)
			{
				PrintError("Polling connection message");
				return false;
			}
			else if (poll.revents & (POLLIN | POLLHUP))
			{
				// Receive SYN
				int bytesReceived = ::recvfrom(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0, &senderAddress, &size);

				// Handle error
				if (bytesReceived == SOCKET_ERROR)
				{
					PrintError("Receiving SYN accepting new client");
					return false;
				}
				// Handle receiving a message
				else
				{
					// Make sure this is a SYN code
					if (packet.GetCode() & SYNCODE)
					{
						PrintMessage("Received SYN code", mVerbose);
						return true;
					}
					// We were told to reset connection establishment
					else if (packet.GetCode() & RSTCODE)
					{
						PrintMessage("Reseting connection.", mVerbose);
						return false;
					}
					// If not, this message needs a reset
					// Probably received a message from an old client
					// Anyway, send a reset message in case there are two clients trying to connect
					else
					{
						PrintMessage("Received wrong code while connecting", mVerbose);
						SendRST(senderAddress, packet);
						return false;
					}
				}
			}
		}
		return false;

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