// Copyright (C) 2019 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef BURGWAR_CLIENTLIB_CLIENTSESSION_HPP
#define BURGWAR_CLIENTLIB_CLIENTSESSION_HPP

#include <CoreLib/SessionBridge.hpp>
#include <CoreLib/Protocol/NetworkStringStore.hpp>
#include <ClientLib/LocalCommandStore.hpp>
#include <Nazara/Core/Signal.hpp>
#include <Nazara/Network/IpAddress.hpp>
#include <memory>
#include <optional>

namespace bw
{
	class BurgApp;

	class ClientSession
	{
		friend class LocalCommandStore;

		public:
			struct ConnectionInfo;

			inline ClientSession(BurgApp& app);
			ClientSession(const ClientSession&) = delete;
			ClientSession(ClientSession&&) = delete;
			virtual ~ClientSession();

			bool Connect(std::shared_ptr<SessionBridge> sessionBridge);
			void Disconnect();

			inline BurgApp& GetApp();
			inline const BurgApp& GetApp() const;
			inline const ConnectionInfo& GetConnectionInfo() const;
			inline const NetworkStringStore& GetNetworkStringStore() const;

			inline bool IsConnected() const;

			void HandleIncomingPacket(Nz::NetPacket& packet);

			template<typename T> void SendPacket(const T& packet);

			ClientSession& operator=(const ClientSession&) = delete;
			ClientSession& operator=(ClientSession&&) = delete;

			NazaraSignal(OnConnected, ClientSession* /*session*/);
			NazaraSignal(OnConnectionInfoUpdate, ClientSession* /*session*/, const ConnectionInfo& /*info*/);
			NazaraSignal(OnDisconnected, ClientSession* /*session*/);

			// Packet signals
			NazaraSignal(OnAuthFailure,                  ClientSession* /*session*/, const Packets::AuthFailure&                  /*data*/);
			NazaraSignal(OnAuthSuccess,                  ClientSession* /*session*/, const Packets::AuthSuccess&                  /*data*/);
			NazaraSignal(OnChatMessage,                  ClientSession* /*session*/, const Packets::ChatMessage&                  /*data*/);
			NazaraSignal(OnClientAssetList,              ClientSession* /*session*/, const Packets::ClientAssetList&              /*data*/);
			NazaraSignal(OnClientScriptList,             ClientSession* /*session*/, const Packets::ClientScriptList&             /*data*/);
			NazaraSignal(OnConsoleAnswer,                ClientSession* /*session*/, const Packets::ConsoleAnswer&                /*data*/);
			NazaraSignal(OnControlEntity,                ClientSession* /*session*/, const Packets::ControlEntity&                /*data*/);
			NazaraSignal(OnCreateEntities,               ClientSession* /*session*/, const Packets::CreateEntities&               /*data*/);
			NazaraSignal(OnDeleteEntities,               ClientSession* /*session*/, const Packets::DeleteEntities&               /*data*/);
			NazaraSignal(OnDownloadClientScriptResponse, ClientSession* /*session*/, const Packets::DownloadClientScriptResponse& /*data*/);
			NazaraSignal(OnEntitiesAnimation,            ClientSession* /*session*/, const Packets::EntitiesAnimation&            /*data*/);
			NazaraSignal(OnEntitiesInputs,               ClientSession* /*session*/, const Packets::EntitiesInputs&               /*data*/);
			NazaraSignal(OnEntityWeapon,                 ClientSession* /*session*/, const Packets::EntityWeapon&                 /*data*/);
			NazaraSignal(OnHealthUpdate,                 ClientSession* /*session*/, const Packets::HealthUpdate&                 /*data*/);
			NazaraSignal(OnInputTimingCorrection,        ClientSession* /*session*/, const Packets::InputTimingCorrection&        /*data*/);
			NazaraSignal(OnMatchData,                    ClientSession* /*session*/, const Packets::MatchData&                    /*data*/);
			NazaraSignal(OnMatchState,                   ClientSession* /*session*/, const Packets::MatchState&                   /*data*/);
			NazaraSignal(OnNetworkStrings,               ClientSession* /*session*/, const Packets::NetworkStrings&               /*data*/);
			NazaraSignal(OnPlayerWeapons,                ClientSession* /*session*/, const Packets::PlayerWeapons&                /*data*/);

			struct ConnectionInfo
			{
				Nz::UInt32 ping;
				Nz::UInt64 lastReceiveTime;
			};

		protected:
			inline void UpdateInfo(const ConnectionInfo& connectionInfo);

		private:
			void OnSessionConnected();
			void OnSessionDisconnected();
			
			NazaraSlot(SessionBridge, OnConnected, m_onConnectedSlot);
			NazaraSlot(SessionBridge, OnDisconnected, m_onDisconnectedSlot);
			NazaraSlot(SessionBridge, OnIncomingPacket, m_onIncomingPacketSlot);

			std::shared_ptr<SessionBridge> m_bridge;
			BurgApp& m_application;
			ConnectionInfo m_connectionInfo;
			LocalCommandStore m_commandStore;
			NetworkStringStore m_stringStore;
	};
}

#include <ClientLib/ClientSession.inl>

#endif
