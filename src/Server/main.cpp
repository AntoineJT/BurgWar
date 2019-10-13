// Copyright (C) 2019 J�r�me Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Nazara/Core/Initializer.hpp>
#include <Nazara/Network/Network.hpp>
#include <Server/ServerApp.hpp>
#include <filesystem>

int main(int argc, char* argv[])
{
	Nz::Initializer<Nz::Network> network;
	bw::ServerApp app(argc, argv);

	return app.Run();
}
