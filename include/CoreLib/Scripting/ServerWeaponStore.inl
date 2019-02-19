// Copyright (C) 2019 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CoreLib/Scripting/ServerWeaponStore.hpp>
#include <cassert>

namespace bw
{
	inline ServerWeaponStore::ServerWeaponStore(BurgApp& app, std::shared_ptr<SharedScriptingContext> context) :
	SharedWeaponStore(std::move(context), true),
	m_application(app)
	{
	}
}