#pragma once
#include "networking.hpp"

#include <vector>

namespace CS260
{
	struct ClientInfo
	{
		unsigned char mID;
		sockaddr mEndpoint;
	};
	class Server
	{
		std::vector<ClientInfo> mClients;

		void HandleNewClients();

	public:
		/*	\fn Server
		\brief	Server constructor following RAII design
		*/
		Server();
		~Server();
	};
}