#pragma once

#include "src/Components/Transform.h"
#include "src/Components/Collider.h"

namespace DYE::MiniGame
{
	struct PongHomebase
	{
		int PlayerID = 0;
		Transform Transform;
		BoxCollider Collider;

		inline Math::AABB GetAABB() const { return Math::AABB::CreateFromCenter(Transform.Position, Collider.Size); }
	};
}
