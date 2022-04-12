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
			PrintMessage("Invalid socket created " + std::to_string(WSAGetLastError()) );
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
		NewPlayerPacket packet;
		WSAPOLLFD poll;
		poll.fd = mSocket;
		poll.events = POLLIN;
		
		while(WSAPoll(&poll, 1, timeout) > 0)
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
					if (packet.mCode & NEWPLAYER)
					{
						mNewPlayersOnFrame.push_back(packet);
					}
				}
			}
		}
	}

	std::vector<NewPlayerPacket>  Client::GetNewPlayers()
	{
		return mNewPlayersOnFrame;
	}

	/*	\fn ConnectToServer
	\brief	Handles connection
	*/
	bool Client::ConnectToServer()
	{
		bool connected = false;

		do
		{
			ConnectionPacket packet;
			if (SendSYN(packet))
			{
				if (ReceiveSYNACK(packet))
					connected = true;
				else
					PrintMessage("[CLIENT] Error receiving SYNACK message.");

			}
			else
			{
				PrintMessage("[CLIENT] Error sending SYN message.");
				return false;
			}
			// Receive SYNACK

		} while (!connected);

		return true;
	}

	/*	\fn SendSYN
	\brief	Sends SYN message
	*/
	bool Client::SendSYN(ConnectionPacket& packet)
	{
		PrintMessage("[CLIENT] Sending connection request SYN ");

		// Attach to the packet a sequence number that should be
		// too difficult to be duplicated
		packet.SetSequence(now_ns());
		packet.AttachACK( rand() % now_ns() + 1);
		packet.SetCode(SYNCODE);

		// Send SYN message to the server
		// we only need to send the header
		int bytesReceived = ::send(mSocket, reinterpret_cast<char*>(&packet), sizeof(ConnectionPacketHeader), 0);

		// Upon error close the client.
		if (bytesReceived == SOCKET_ERROR)
		{
			PrintMessage("[CLIENT] Error sending SYN connection " + std::to_string(WSAGetLastError()));
			throw std::runtime_error("[CLIENT] Error sending SYN connection message");
			return false;
		}
		else
		{
			PrintMessage("[CLIENT] SYN sent correctly");
			return true;
		}
	}

	/*	\fn ReceiveSYNACK
	\brief	Receives SYNACK
	*/
	bool Client::ReceiveSYNACK(ConnectionPacket& packet)
	{
		// First of all store the expected ACK
		// from the SYN we just sent
		unsigned char expectedACK = packet.GetExpectedACK();
		auto clock = now();

		PrintMessage("[CLIENT] Waiting for SYNACK expected " + std::to_string(packet.GetExpectedACK()));
		do
		{
			WSAPOLLFD poll;
			poll.fd = mSocket;
			poll.events = POLLIN;
			if (WSAPoll(&poll, 1, timeout) > 0)
			{
				if (poll.revents & POLLERR)
				{
					PrintMessage("[CLIENT] Error polling SYNACK" + std::to_string(WSAGetLastError()));
					// Tell to the server to reset connection if polling gives error
					SendRST(packet);
					throw std::runtime_error("[CLIENT] Error polling SYNACK ");
					return false;
				}
				else if (poll.revents & (POLLIN | POLLHUP))
				{
					// We only need to receive the packet header
					int bytesReceived = ::recv(mSocket, reinterpret_cast<char*>(&packet), sizeof(ConnectionPacket), 0);

					if (bytesReceived == SOCKET_ERROR)
					{
						PrintMessage("[CLIENT] Error receiving SYNACk" + std::to_string(WSAGetLastError()));
						throw std::runtime_error("[CLIENT] Error receiving SYNACK connection message");
						return false;
					}
					else
					{
						// Make sure it is a SYNACK code
						if (packet.GetCode() & SYNACKCODE)
						{
							PrintMessage("[CLIENT] Received SYNACK message " + std::to_string(packet.GetACK()));
							unsigned packetACK = packet.GetACK();

							// Make sure the current ack is the expected one
							if (packetACK == expectedACK)
							{
								// Attach the filename we want to receive
								packet.SetCode(ACKCODE);
								packet.AttachACK(packet.GetSequence() + packet.GetACK());

								// Send ACK of SYNACK
								bytesReceived = ::send(mSocket, reinterpret_cast<char*>(&packet), sizeof(ConnectionPacket), 0);

								if (bytesReceived == SOCKET_ERROR)
								{
									PrintMessage("[CLIENT] Error sending ACK" + std::to_string(WSAGetLastError()));
									closesocket(mSocket);
									NetworkDestroy();
									throw std::runtime_error("[CLIENT] Error sending last ACK connection message");
									return false;
								}
								else
								{
									PrintMessage("[CLIENT] ACK to SYNACK message sent correctly");
									return true;
								}
							}
							// If we received and ACK that was not expected it means something went wrong and connection 
							// must be restarted
							else
							{
								PrintMessage("[CLIENT] Expected SYNACK message, but something unknown received");
								SendRST(packet);
								return false;
							}
						}
					}
				}
			}
			// 5 seconds timeout
		} while (ms_since(clock) < 5000);

		// When we timout waiting to receive SYNACK message
		// Send a reset request to the server and start again
		PrintMessage("[CLIENT] Timeout receiving SYNACK");
		SendRST(packet);

		return false;
	}

	/*	\fn SendRST
	\brief	Sends a reset request
	*/
	void Client::SendRST(ConnectionPacket& packet)
	{
		packet.AttachACK(0);
		packet.SetCode(RSTCODE);
		int bytesSent = ::send(mSocket, reinterpret_cast<char*>(&packet), sizeof(ConnectionPacket), 0);
		if (bytesSent == SOCKET_ERROR)
		{
			PrintMessage("[CLIENT] Error sending RST " + std::to_string(WSAGetLastError()));
		}
		else
			PrintMessage("[CLIENT] Sending RST correctly");
	}
	/*	\fn DisconnectFromServer
	\brief	Handles disconnection
	*/
	void Client::DisconnectFromServer()
	{
		ConnectionPacket packet;

		packet.SetSequence(0);
		packet.AttachACK(0);
		auto clock = now();

		// Send ACK to FIN 1
		PrintMessage("Sending ACK FIN 1");

		// We only need to send the header
		int bytesSent = ::send(mSocket, reinterpret_cast<char*>(&packet), sizeof(ConnectionPacket), 0);

		if (bytesSent == SOCKET_ERROR)
		{
			PrintMessage("Error sending ACK FIN 1" + std::to_string(WSAGetLastError()));
			return;
		}
		else
		{
			packet.AttachACK(0);
			packet.SetCode(FIN2CODE);
			unsigned expectedACK = packet.GetExpectedACK();

			PrintMessage("Sending FIN 2");
			bytesSent = ::send(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);

			if (bytesSent == SOCKET_ERROR)
			{
				PrintMessage("Error sending FIN 2" + std::to_string(WSAGetLastError()));
				return;
			}
			else
			{
				bool acknowledged = false;
				do
				{
					WSAPOLLFD poll;
					poll.fd = mSocket;
					poll.events = POLLIN;

					if (WSAPoll(&poll, 1, timeout) > 0)
					{
						// Receive all messages
						if (poll.revents & POLLERR)
						{
							PrintMessage("Error polling FIN1 or LASTACK message");
							return;
						}
						// We received a message
						else if (poll.revents & (POLLIN | POLLHUP))
						{
							int bytesReceived = ::recv(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
							if (bytesReceived == SOCKET_ERROR)
							{
								PrintError("Error receiving FIN1 or LASTACK message");
								return;
							}
							else
							{
								// Need to resend ACK to FIN 1
								if (packet.GetCode() == FIN1CODE)
								{
									packet.AttachACK(0);

									// Send ACK to FIN 1
									PrintMessage("Resending ACK to FIN 1");
									bytesSent = ::send(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
									if (bytesSent == SOCKET_ERROR)
									{
										PrintError("Error resending ACK to FIN 1");
										return;
									}

									packet.AttachACK(0);
									packet.SetCode(FIN2CODE);
									expectedACK = packet.GetExpectedACK();

									PrintMessage("Resending FIN 2");
									bytesSent = ::send(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
									if (bytesSent == SOCKET_ERROR)
									{
										PrintError("Error resending FIN 2");
										return;
									}

								}
								// Received last ACK
								else if (packet.GetCode() & LASTACKCODE && expectedACK == packet.GetACK())
								{
									PrintMessage("Received last ACK");
									acknowledged = true;
								}
							}
						}
					}
					// Timeout of 30 seconds
					// if we timeout, we disconnect directly since we could not receive FIN2
					// This time the timeout is much bigger since each FIN1 send attempt
					// will have 5 attempts and each one will take as much 5 seconds
					// which makes a total of minimum 25 seconds
					// We give a little more to avoid issues
				} while (!acknowledged && ms_since(clock) < 30000);
			}
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

}