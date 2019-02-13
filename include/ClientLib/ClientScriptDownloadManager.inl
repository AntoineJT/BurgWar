// Copyright (C) 2019 J�r�me Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientScriptDownloadManager.hpp>

namespace bw
{
	inline ClientScriptDownloadManager::ClientScriptDownloadManager(std::filesystem::path clientFileCache) :
	m_clientFileCache(std::move(clientFileCache)),
	m_currentFileIndex(0)
	{
	}
}