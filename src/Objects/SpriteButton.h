#pragma once

#include "src/Components/Transform.h"
#include "src/Components/Sprite.h"

namespace DYE::MiniGame
{
	struct SpriteButton
	{
		Transform Transform;
		Sprite Sprite;

		std::shared_ptr<Texture2D> SelectedTexture;
		std::shared_ptr<Texture2D> DeselectedTexture;

		void SetSelectAppearance();
		void SetDeselectAppearance();
	};
}