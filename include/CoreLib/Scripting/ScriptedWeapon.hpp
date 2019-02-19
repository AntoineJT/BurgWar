// Copyright (C) 2019 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef BURGWAR_CORELIB_SCRIPTING_SCRIPTEDWEAPON_HPP
#define BURGWAR_CORELIB_SCRIPTING_SCRIPTEDWEAPON_HPP

#include <CoreLib/AnimationStore.hpp>
#include <CoreLib/Scripting/ScriptedElement.hpp>
#include <Nazara/Math/Vector2.hpp>
#include <memory>

namespace bw
{
	struct ScriptedWeapon : ScriptedElement
	{
		Nz::Vector2f spriteOrigin;
		Nz::Vector2f weaponOrigin;
		Nz::Vector2f weaponOffset;
		std::shared_ptr<AnimationStore> animations;
		std::string spriteName;
		float scale;
		sol::reference animationStartFunction;
	};
}

#endif