// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef BURGWAR_CORELIB_SCRIPTING_SCRIPTEDEVENT_HPP
#define BURGWAR_CORELIB_SCRIPTING_SCRIPTEDEVENT_HPP

#include <Thirdparty/sol3/sol.hpp>

namespace bw
{
	struct ScriptedEvent
	{
		std::size_t index;
		std::string name;
		std::string returnType;
		sol::main_protected_function combinator;
	};

	ScriptedEvent InitEventFromLua(std::size_t index, const sol::table& table);
}

#endif
