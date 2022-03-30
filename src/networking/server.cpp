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

		sockaddr senderAddress;
		if(ReceiveSYN(senderAddress, packet))
			if (SendSYNACK(senderAddress, packet))
				if (ReceiveACKFromSYNACK(senderAddress, packet))
				{ }

	}

	bool Server::ReceiveSYN(sockaddr& senderAddress, ConnectionPacket& packet)
	{
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
					if (packet.mCode & SYNCODE)
					{
						PrintMessage("Received SYN code", mVerbose);
						return true;
					}
					// We were told to reset connection establishment
					else if (packet.mCode & RSTCODE)
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

	bool Server::SendSYNACK(sockaddr& senderAddress, ConnectionPacket& packet)
	{
		const int size = sizeof(sockaddr);

		// We will only send the SYNACKCODE and the ACK number
		// Which are included by themselsves, so we do not add anything
		packet.AttachACK(0);

		// Change the type of message
		packet.SetCode(SYNACKCODE);

		// We only need to send the header as it will contatin the code and the required ack
		int bytesSent = ::sendto(mSocket, reinterpret_cast<char*>(&packet), sizeof(ProtocolHeader), 0, &senderAddress, size);

		// Handle errors
		if (bytesSent == SOCKET_ERROR)
		{
			PrintError("Sending SYN ACK accepting new client");
			return false;
		}
		else
		{
			PrintMessage("Sending SYNACK with sequence " + std::to_string(packet.GetSequence()), mVerbose);
			return true;
		}
	}

	bool Server::ReceiveACKFromSYNACK(sockaddr& senderAddress, ConnectionPacket& packet)
	{
		unsigned expectedACK = packet.GetExpectACK();
		auto clock = now();
		socklen_t size = sizeof(senderAddress);
		do
		{
			WSAPOLLFD poll;
			poll.fd = mSocket;
			poll.events = POLLIN;
			if (WSAPoll(&poll, 1, timeout) > 0)
			{
				if (poll.revents & POLLERR)
				{
					PrintError("Polling receive ACK from SYNACK");
					return false;
				}
				else
				{
					PrintMessage("Waiting for ACK to SYNACK expected ACK " + std::to_string(expectedACK), mVerbose); ;
					// Receive ACK of SYNACK
					int bytesReceived = ::recvfrom(mSocket, reinterpret_cast<char*>(&packet), sizeof(ProtocolPacket), 0, &senderAddress, &size);
					if (bytesReceived == SOCKET_ERROR)
					{
						PrintError("Receiving ACK from SYNACK");
						return false;
					}
					// Make sure it is an ACK code
					if (packet.GetCode() & ACKCODE)
					{
						unsigned packetACK = packet.GetACK();
						PrintMessage("Received ACK to SYNACK " + std::to_string(packetACK), mVerbose); ;

						// Make sure the current ack is the expected one
						if (packetACK == expectedACK)
						{
							// Connect to the client socket to the given address to allow calling
							// send and recv with the client being the default receiver and sender
							// We will forget about new clients sending messages while we are transferring a file
							if (::connect(mSocket, &senderAddress, sizeof(senderAddress)) == SOCKET_ERROR)
							{
								PrintError("on ::connect to client socket");
								return false;
							}
							else
							{
								PrintMessage("[SERVER] Connected socket correctly.", mVerbose);

								unsigned msgSize = packet.GetMsgSize();

								// Make the filename string big enough
								mCurrentFilename.resize(msgSize);

								// Copy the attached filename
								::memcpy(mCurrentFilename.data(), packet.msg.data(), msgSize);
								return true;
							}
						}
					}
					// RESET
					else if (packet.GetCode() & RSTCODE)
					{
						PrintMessage("Received RST", mVerbose);
						return false;
					}
					// We received any other type of message probably mixed from a previous client
					else
					{
						PrintMessage("Received unknown type of message", mVerbose);
						// Although this message is most likely to be ignored, send a reset request in case
						// two clients are trying to connect at the same time
						SendRST(senderAddress, packet);
					}
				}
			}
			// Timeout of 5 seconds
		} while (ms_since(clock) < 5000);

		// In case of timeout waiting for the ACK from SYNACK restart connection
		PrintMessage("Timeout waiting for ACK from SYNACK", mVerbose);
		SendRST(senderAddress, packet);
		return false;
	}

	/*	\fn SendRST
	\brief	Sends reset message
	*/
	bool Server::SendRST(sockaddr& senderAddress, ConnectionPacket& packet)
	{
		packet.AttachACK(0);
		packet.SetCode(RSTCODE);
		int size = sizeof(senderAddress);
		int bytesReceived = ::sendto(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0, &senderAddress, size);
		if (bytesReceived == SOCKET_ERROR)
		{
			PrintError("Error sending RST");
		}
		else
			PrintMessage("[SERVER] Sending RST correctly", mVerbose);

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