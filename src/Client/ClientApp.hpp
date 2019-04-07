// Copyright (C) 2019 J�r�me Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef BURGWAR_CLIENTAPP_HPP
#define BURGWAR_CLIENTAPP_HPP

#include <CoreLib/BurgApp.hpp>
#include <ClientLib/LocalCommandStore.hpp>
#include <ClientLib/NetworkReactorManager.hpp>
#include <Nazara/Renderer/RenderWindow.hpp>
#include <NDK/Application.hpp>
#include <NDK/StateMachine.hpp>
#include <vector>

namespace bw
{
	class LocalMatch;
	class Match;
	class NetworkReactor;
	class NetworkSessionBridge;
	struct StateData;

	class ClientApp : public Ndk::Application, public BurgApp
	{
		public:
			ClientApp(int argc, char* argv[]);
			~ClientApp();

			inline NetworkReactorManager& GetReactorManager();

			int Run();

		private:
			std::shared_ptr<StateData> m_stateData;
			Ndk::StateMachine m_stateMachine;
			Nz::RenderWindow& m_mainWindow;
			NetworkReactorManager m_networkReactors;
	};
}

#include <Client/ClientApp.inl>

#endif