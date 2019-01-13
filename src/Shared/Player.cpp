// Copyright (C) 2018 Jérôme Leclercq
// This file is part of the "Burgwar Client" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Shared/Player.hpp>
#include <Nazara/Core/CallOnExit.hpp>
#include <Nazara/Graphics/Material.hpp>
#include <Nazara/Graphics/Sprite.hpp>
#include <Nazara/Physics2D/Collider2D.hpp>
#include <NDK/Components.hpp>
#include <Shared/Match.hpp>
#include <Shared/MatchClientSession.hpp>
#include <Shared/MatchClientVisibility.hpp>
#include <Shared/Components/HealthComponent.hpp>
#include <Shared/Components/NetworkSyncComponent.hpp>
#include <Shared/Components/PlayerControlledComponent.hpp>
#include <Shared/Components/PlayerMovementComponent.hpp>
#include <Shared/Components/ScriptComponent.hpp>
#include <Shared/Scripting/ServerGamemode.hpp>

namespace bw
{
	Player::Player(MatchClientSession& session, Nz::UInt8 playerIndex, std::string playerName) :
	m_layerIndex(std::numeric_limits<std::size_t>::max()),
	m_name(std::move(playerName)),
	m_playerIndex(playerIndex),
	m_session(session)
	{
	}

	Player::~Player()
	{
		if (m_match)
			m_match->Leave(this);
	}

	const Ndk::EntityHandle& Player::CreateEntity(Ndk::World& world)
	{
		ServerEntityStore& entityStore = m_match->GetEntityStore();
		ServerWeaponStore& weaponStore = m_match->GetWeaponStore();
		if (std::size_t entityIndex = entityStore.GetElementIndex("entity_burger"); entityIndex != ServerEntityStore::InvalidIndex)
		{
			const Ndk::EntityHandle& burger = entityStore.InstantiateEntity(world, entityIndex);
			if (!burger)
				return Ndk::EntityHandle::InvalidHandle;

			burger->AddComponent<PlayerControlledComponent>(CreateHandle());

			if (burger->HasComponent<HealthComponent>())
			{
				auto& healthComponent = burger->GetComponent<HealthComponent>();

				healthComponent.OnDied.Connect([ply = CreateHandle()](const HealthComponent* health, const Ndk::EntityHandle& attacker)
				{
					if (!ply)
						return;

					/*ply->GetMatch()->GetGamemode()->ExecuteCallback("OnPlayerDeath", [&](Nz::LuaState& state)
					{
						state.PushLightUserdata(ply.GetObject());
						state.Push(attacker);
						return 2;
					});*/
				});
			}

			static unsigned int huglyCount = 0;

			burger->GetComponent<Ndk::PhysicsComponent2D>().SetPosition({ 200.f + (huglyCount++) * 100.f, 100.f });

			UpdateControlledEntity(burger);

			// Create weapon
			if (std::size_t weaponIndex = weaponStore.GetElementIndex("weapon_sword_emmentalibur"); weaponIndex != ServerEntityStore::InvalidIndex)
				m_playerWeapon = weaponStore.InstantiateWeapon(world, weaponIndex, burger);

			return burger;
		}

		return Ndk::EntityHandle::InvalidHandle;
	}

	void Player::UpdateControlledEntity(const Ndk::EntityHandle& entity)
	{
		m_playerEntity = entity;

		Packets::ControlEntity controlEntity;
		controlEntity.entityId = (entity) ? static_cast<Nz::UInt32>(entity->GetId()) : 0;
		controlEntity.playerIndex = m_playerIndex;

		m_session.GetVisibility().SendEntityPacket(controlEntity.entityId, controlEntity);
	}

	void Player::UpdateInput(bool isAttacking, bool isJumping, bool isMovingLeft, bool isMovingRight)
	{
		if (m_playerEntity)
		{
			assert(m_playerEntity->HasComponent<PlayerControlledComponent>());

			auto& playerController = m_playerEntity->GetComponent<PlayerControlledComponent>();
			playerController.UpdateJumpingState(isJumping);
			playerController.UpdateMovingLeftState(isMovingLeft);
			playerController.UpdateMovingRightState(isMovingRight);

			if (isAttacking)
			{
				if (m_playerWeapon)
				{
					auto& weaponScript = m_playerWeapon->GetComponent<ScriptComponent>();
					weaponScript.ExecuteCallback("OnAttack", weaponScript.GetTable());
				}
			}
		}
	}

	void Player::UpdateLayer(std::size_t layerIndex)
	{
		m_layerIndex = layerIndex;
	}

	void Player::UpdateMatch(Match* match)
	{
		m_match = match;
	}
}
