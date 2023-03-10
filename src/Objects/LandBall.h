#pragma once

#include "src/Components/Transform.h"
#include "src/Components/Velocity.h"
#include "src/Components/Collider.h"
#include "src/Components/Sprite.h"

namespace DYE::MiniGame
{
	struct LandBall
	{
	public:
		// Constant settings.
		constexpr static float Apex = 10;
		constexpr static float InitialTimeToReachApex = 1;
		float TimePercentageLossPerBounce = 0.015f;
		float MinimumTimeToReachApex = 0.4f;
		float HorizontalMoveSpeed = 18.0f;

		// Game settings/states
		float TimeToReachApex = InitialTimeToReachApex;
		float HorizontalMovementBuffer = 0.0f;

		Transform Transform;
		Velocity Velocity;
		BoxCollider Collider;
		Sprite Sprite;

		float GetGravity() const { return -Apex / (TimeToReachApex * TimeToReachApex * 0.5f); }
		float GetLaunchVerticalSpeed() const { return glm::abs(GetGravity()) * TimeToReachApex; }

		void OnBounce();
	};
}