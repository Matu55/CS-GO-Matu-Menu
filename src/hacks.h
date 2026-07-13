#pragma once
#include "memory.h"

namespace hacks
{
	//Run visual hacks
	void VisualThread(const Memory& mem) noexcept;

	//Aim hacks
	void AimThread(const Memory& mem) noexcept;

	//Move hacks
	void MoveThread(const Memory& mem) noexcept;
}