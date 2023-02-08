#pragma once

#include "Graphics/CameraProperties.h"

#include "src/Components/Transform.h"

namespace DYE::MiniGame
{
	struct Camera
	{
		Transform Transform;
		CameraProperties Properties;

		CameraProperties GetTransformedProperties() const
		{
			CameraProperties properties = Properties;
			properties.Position = Transform.Position;
			return properties;
		}
	};
}