// Copyright (C) 2019 J�r�me Leclercq
// This file is part of the "Burgwar Map Editor" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <MapEditor/Widgets/Integer2SpinBox.hpp>

namespace bw
{
	inline Integer2SpinBox::Integer2SpinBox(const Nz::Vector2i64& value, LabelMode labelMode, QBoxLayout::Direction dir, QWidget* parent) :
	Integer2SpinBox(labelMode, dir, parent)
	{
		setValue(value);
	}
}