// Copyright (C) 2019 J�r�me Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Nazara/Audio/Audio.hpp>
#include <Nazara/Audio/SoundBuffer.hpp>
#include <Nazara/Core/AbstractLogger.hpp>
#include <Nazara/Renderer/RenderWindow.hpp>
#include <Nazara/Network/Network.hpp>
#include <CoreLib/Utility/CrashHandler.hpp>
#include <Client/ClientApp.hpp>

int main(int argc, char* argv[])
{
	bw::CrashHandler crashHandler;
	crashHandler.Install();

	Nz::Initializer<Nz::Network> network;
	bw::ClientApp app(argc, argv); 
	app.EnableFPSCounter(true);

	Nz::Audio::SetGlobalVolume(70.f);
	Nz::Audio::SetSpeedOfSound(343.3f * 100.f);

	Nz::SoundBufferParams params;
	params.forceMono = true;

	Nz::SoundBufferManager::SetDefaultParameters(params);

	return app.Run();
}
