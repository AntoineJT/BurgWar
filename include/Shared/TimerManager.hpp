// Copyright (C) 2018 Jérôme Leclercq
// This file is part of the "Burgwar Client" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef BURGWAR_SHARED_TIMERMANAGER_HPP
#define BURGWAR_SHARED_TIMERMANAGER_HPP

#include <Shared/BurgApp.hpp>
#include <vector>

namespace bw
{
	class TimerManager
	{
		public:
			using Callback = std::function<void()>;

			inline TimerManager(BurgApp& app);
			~TimerManager() = default;

			inline void PushCallback(Nz::UInt64 expirationTime, Callback finish);

			inline void Update();

		private:
			struct Timer
			{
				Callback callback;
				Nz::UInt64 expirationTime;
			};

			std::vector<Timer> m_pendingTimers;
			BurgApp& m_app;
	};
}

#include <Shared/TimerManager.inl>

#endif