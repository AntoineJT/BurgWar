// Copyright (C) 2019 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Scripting/ClientEntityLibrary.hpp>
#include <CoreLib/AssetStore.hpp>
#include <ClientLib/Components/SoundEmitterComponent.hpp>
#include <ClientLib/Scripting/ClientScriptingLibrary.hpp>
#include <ClientLib/Scripting/Sprite.hpp>
#include <ClientLib/Utility/TileMapData.hpp>
#include <Nazara/Graphics/Sprite.hpp>
#include <Nazara/Graphics/TileMap.hpp>
#include <NDK/Components/GraphicsComponent.hpp>
#include <NDK/Components/NodeComponent.hpp>

namespace bw
{
	void ClientEntityLibrary::RegisterLibrary(sol::table& elementMetatable)
	{
		SharedEntityLibrary::RegisterLibrary(elementMetatable);

		RegisterClientLibrary(elementMetatable);
	}

	void ClientEntityLibrary::RegisterClientLibrary(sol::table& elementMetatable)
	{
		elementMetatable["AddSprite"] = [this](const sol::table& entityTable, const sol::table& parameters)
		{
			const Ndk::EntityHandle& entity = AssertScriptEntity(entityTable);

			std::string texturePath = parameters["TexturePath"];
			int renderOrder = parameters.get_or("RenderOrder", 0);
			Nz::Vector2f offset = parameters.get_or("Offset", Nz::Vector2f(0.f, 0.f));
			Nz::DegreeAnglef rotation = parameters.get_or("Rotation", Nz::DegreeAnglef::Zero());
			Nz::Vector2f origin = parameters.get_or("Origin", Nz::Vector2f(0.5f, 0.5f));
			Nz::Vector2f scale = parameters.get_or("Scale", Nz::Vector2f::Unit());

			Nz::Matrix4 transformMatrix = Nz::Matrix4f::Transform(offset, rotation);

			Nz::MaterialRef mat = Nz::Material::New("Translucent2D");
			mat->SetDiffuseMap(m_assetStore.GetTexture(texturePath));
			auto& sampler = mat->GetDiffuseSampler();
			sampler.SetFilterMode(Nz::SamplerFilter_Bilinear);

			Nz::SpriteRef sprite = Nz::Sprite::New();
			sprite->SetMaterial(mat);
			sprite->SetSize(sprite->GetSize() * scale);
			sprite->SetOrigin(sprite->GetSize() * origin);

			Ndk::GraphicsComponent& gfxComponent = (entity->HasComponent<Ndk::GraphicsComponent>()) ? entity->GetComponent<Ndk::GraphicsComponent>() : entity->AddComponent<Ndk::GraphicsComponent>();
			gfxComponent.Attach(sprite, transformMatrix, renderOrder);

			return Sprite(entity, sprite, transformMatrix, renderOrder);
		};

		elementMetatable["AddTilemap"] = [this](const sol::table& entityTable, const Nz::Vector2ui& mapSize, const Nz::Vector2f& cellSize, const sol::table& content, const std::vector<TileData>& tiles)
		{
			const Ndk::EntityHandle& entity = AssertScriptEntity(entityTable);

			// Compute tilemap
			tsl::hopscotch_map<std::string /*materialPath*/, std::size_t /*materialIndex*/> materials;
			for (const auto& tile : tiles)
			{
				auto it = materials.find(tile.materialPath);
				if (it == materials.end())
					materials.emplace(tile.materialPath, materials.size());
			}

			Nz::TileMapRef tileMap = Nz::TileMap::New(mapSize, cellSize, materials.size());
			for (auto&& [materialPath, matIndex] : materials)
			{
				Nz::MaterialRef material = Nz::Material::New(); //< FIXME
				material->SetDiffuseMap(m_assetStore.GetTexture(materialPath));

				if (material)
				{
					// Force alpha blending
					material->Configure("Translucent2D");
					material->SetDiffuseMap(m_assetStore.GetTexture(materialPath)); //< FIXME
				}
				else
					material = Nz::Material::GetDefault();

				tileMap->SetMaterial(matIndex, material);
			}

			std::size_t cellCount = content.size();
			std::size_t expectedCellCount = mapSize.x * mapSize.y;
			if (cellCount != expectedCellCount)
			{
				std::cerr << "Expected " << expectedCellCount << " cells, got " << cellCount << std::endl;
				cellCount = std::min(cellCount, expectedCellCount);
			}

			for (std::size_t i = 0; i < cellCount; ++i)
			{
				unsigned int value = content[i + 1];

				Nz::Vector2ui tilePos = { static_cast<unsigned int>(i % mapSize.x), static_cast<unsigned int>(i / mapSize.x) };
				if (value > 0)
				{
					if (value - 1 >= content.size())
						throw std::runtime_error("");

					const auto& tileData = tiles[value - 1];

					auto matIt = materials.find(tileData.materialPath);
					assert(matIt != materials.end());

					tileMap->EnableTile(tilePos, tileData.texCoords, Nz::Color::White, matIt->second);
				}
			}

			Ndk::GraphicsComponent& gfxComponent = (entity->HasComponent<Ndk::GraphicsComponent>()) ? entity->GetComponent<Ndk::GraphicsComponent>() : entity->AddComponent<Ndk::GraphicsComponent>();
			gfxComponent.Attach(tileMap);
		};

		elementMetatable["PlaySound"] = [this](const sol::table& entityTable, const std::string& soundPath, bool isAttachedToEntity, bool isLooping, bool isSpatialized)
		{
			const Ndk::EntityHandle& entity = AssertScriptEntity(entityTable);

			const Nz::SoundBufferRef& soundBuffer = m_assetStore.GetSoundBuffer(soundPath);
			if (!soundBuffer)
				throw std::runtime_error("failed to load " + soundPath);

			auto& entityNode = entity->GetComponent<Ndk::NodeComponent>();

			if (!entity->HasComponent<SoundEmitterComponent>())
				entity->AddComponent<SoundEmitterComponent>();

			auto& soundEmitter = entity->GetComponent<SoundEmitterComponent>();
			return soundEmitter.PlaySound(soundBuffer, entityNode.GetPosition(), isAttachedToEntity, isLooping, isSpatialized);
		};
	}
}
