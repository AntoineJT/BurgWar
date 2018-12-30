// Copyright (C) 2018 Jérôme Leclercq
// This file is part of the "Burgwar Shared" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Client/Scripting/ClientWeaponStore.hpp>
#include <Shared/Components/PlayerMovementComponent.hpp>
#include <Shared/Components/ScriptComponent.hpp>
#include <Nazara/Graphics/Sprite.hpp>
#include <Nazara/Math/Vector2.hpp>
#include <NDK/Components.hpp>
#include <NDK/LuaAPI.hpp>
#include <iostream>

namespace bw
{
	const Ndk::EntityHandle& ClientWeaponStore::InstantiateWeapon(Ndk::World& world, std::size_t entityIndex, const Ndk::EntityHandle& parent)
	{
		auto& weaponClass = *GetElement(entityIndex);

		Nz::MaterialRef mat = Nz::Material::New("Translucent2D");
		mat->SetDiffuseMap(weaponClass.spriteName);
		auto& sampler = mat->GetDiffuseSampler();
		sampler.SetFilterMode(Nz::SamplerFilter_Bilinear);

		Nz::SpriteRef sprite = Nz::Sprite::New();
		sprite->SetMaterial(mat);
		sprite->SetSize(sprite->GetSize() * weaponClass.scale);
		Nz::Vector2f burgerSize = sprite->GetSize();
		sprite->SetOrigin(weaponClass.spriteOrigin);

		const Ndk::EntityHandle& weapon = world.CreateEntity();
		weapon->AddComponent<Ndk::GraphicsComponent>().Attach(sprite, -1);

		SharedWeaponStore::InitializeWeapon(weaponClass, weapon, parent);

		return weapon;
	}

	void ClientWeaponStore::InitializeElementTable(sol::table& elementTable)
	{
		SharedWeaponStore::InitializeElementTable(elementTable);

		elementTable["Scale"] = 1.f;
	}

	void ClientWeaponStore::InitializeElement(sol::table& elementTable, ScriptedWeapon& weapon)
	{
		SharedWeaponStore::InitializeElement(elementTable, weapon);

		weapon.scale = elementTable["Scale"];
		weapon.spriteName = elementTable["Sprite"];
		weapon.spriteOrigin = elementTable["SpriteOrigin"];
	}
}
