// Copyright (C) 2019 J�r�me Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/LocalMatch.hpp>

namespace bw
{
	inline AnimationManager& LocalMatch::GetAnimationManager()
	{
		return m_animationManager;
	}
}