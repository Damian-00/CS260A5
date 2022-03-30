#pragma once
#include "networking.hpp"

#include <vector>

namespace CS260
{
	struct ShipPacket
	{
		vec2 pos;
		vec2 vel;
		float score;
	};

	struct ClientInfo
	{
		unsigned char mID;
		sockaddr mEndpoint;
	};
	class Server
	{
		bool mVerbose;
		SOCKET mSocket;
		sockaddr_in mEndpoint;
		std::vector<ClientInfo> mClients;


	public:
		/*	\fn Server
		\brief	Server constructor following RAII design
		*/
		Server(bool verbose);

		/*	\fn ~Server
		\brief	Server destructor following RAII design
		*/
		~Server();

		void Tick();
	private:

		void HandleNewClients();
		void PrintMessage(const std::string& msg);
		void PrintError(const std::string& msg);
	};
}