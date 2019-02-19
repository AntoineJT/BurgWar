// Copyright (C) 2019 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef BURGWAR_CORELIB_SCRIPTING_SERVERGAMEMODE_HPP
#define BURGWAR_CORELIB_SCRIPTING_SERVERGAMEMODE_HPP

#include <CoreLib/Scripting/SharedGamemode.hpp>

namespace bw
{
	class Match;

	class ServerGamemode : public SharedGamemode
	{
		public:
			inline ServerGamemode(Match& match, std::shared_ptr<SharedScriptingContext> scriptingContext, std::filesystem::path gamemodePath);
			~ServerGamemode() = default;

		private:
			void InitializeGamemode();

			Match& m_match;
	};
}

#include <CoreLib/Scripting/ServerGamemode.inl>

#endif