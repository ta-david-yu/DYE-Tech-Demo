#pragma once

#include "src/Components/Transform.h"
#include "src/Components/Velocity.h"
#include "src/Components/Collider.h"
#include "src/Components/Sprite.h"
#include "src/Components/Hittable.h"
#include "src/Components/AttachableToPaddle.h"

namespace DYE::MiniGame
{
	struct PongBall
	{
	public:
		Transform Transform;
		Velocity Velocity;
		CircleCollider Collider;
		Hittable Hittable;
		AttachableToPaddle Attachable;
		Sprite Sprite;
		float LaunchBaseSpeed = 7;
		float MaxBallSpeed = 40;

		float RespawnAnimationDuration = 0.5f;
		float HitAnimationDuration = 0.25f;
		float GoalAnimationDuration = 0.25f;

		void EquipToPaddle(PlayerPaddle& paddle, glm::vec2 offset);
		void LaunchFromAttachedPaddle();

		void PlayRespawnAnimation();
		void PlayHitAnimation();
		void PlayGoalAnimation();

		void UpdateAnimation(float timeStep);

	private:
		float m_RespawnAnimationTimer = 0.0f;
		float m_HitAnimationTimer = 0.0f;
		float m_GoalAnimationTimer = 0.0f;
	};
}