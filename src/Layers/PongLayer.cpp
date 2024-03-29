#include "src/Layers/PongLayer.h"

#include "src/DYETechDemoApp.h"

#include "Core/Application.h"
#include "Util/Logger.h"
#include "Util/Macro.h"
#include "Util/Time.h"

#include "Math/Color.h"
#include "Math/Math.h"

#include "ImGui/ImGuiUtil.h"

#include "Input/InputManager.h"
#include "Screen.h"
#include "Math/PrimitiveTest.h"

#include "Graphics/RenderCommand.h"
#include "Graphics/RenderPipelineManager.h"
#include "Graphics/RenderPipeline2D.h"

#include "Graphics/WindowBase.h"
#include "Graphics/WindowManager.h"
#include "Graphics/ContextBase.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Shader.h"
#include "Graphics/OpenGL.h"
#include "Graphics/Texture.h"
#include "Graphics/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/DebugDraw.h"

#include "Event/ApplicationEvent.h"

#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"

namespace DYE
{
	PongLayer::PongLayer(Application& application) : m_Application(application)
	{
	}

	void PongLayer::OnAttach()
	{
		RenderCommand::GetInstance().SetClearColor(glm::vec4{0.5f, 0.5f, 0.5f, 0.5f});
		RenderCommand::GetInstance().SetLinePrimitiveWidth(3);

		// Initialize screen parameters.
		auto displayMode = Screen::GetInstance().TryGetDisplayMode(0);
		m_ScreenDimensions = glm::vec<2, std::uint32_t>{displayMode->Width, displayMode->Height };

		// Create ball objects.
		m_Ball.Transform.Position = {0, 0, 0};
		m_Ball.Collider.Radius = 0.25f;
		m_Ball.Velocity.Value = {5.0f, -0.5f};
		m_Ball.LaunchBaseSpeed = 7;
		m_Ball.Sprite.Texture = Texture2D::Create("assets\\Sprite_Pong.png");
		m_Ball.Sprite.Texture->PixelsPerUnit = 32;
		m_Ball.Sprite.Color = Color::White;

		// Create player objects.
		float const mainPaddleWidth = 0.5f;
		float const paddleAttachOffsetX = m_Ball.Collider.Radius + mainPaddleWidth * 0.5f + 0.01f; // Add 0.01f to avoid floating point error

		MiniGame::PongPlayer player1 {};
		player1.Settings.ID = 0;
		player1.Settings.MainPaddleLocation = {-10, 0, 0};
		player1.Settings.MainPaddleAttachOffset = {paddleAttachOffsetX, 0};
		player1.Settings.HomebaseCenter = {-14, 0, 0};	// TODO:
		player1.Settings.HomebaseSize = {2, 16, 1}; 	// TODO:

		player1.State.Health = MaxHealth;

		MiniGame::PongPlayer player2 {};
		player2.Settings.ID = 1;
		player2.Settings.MainPaddleLocation = {10, 0, 0};
		player2.Settings.MainPaddleAttachOffset = {-paddleAttachOffsetX, 0};
		player2.Settings.HomebaseCenter = {14, 0, 0};	// TODO:
		player2.Settings.HomebaseSize = {2, 16, 1}; 	// TODO:

		player2.State.Health = MaxHealth;

		m_Players.emplace_back(player1);
		m_Players.emplace_back(player2);

		// Create paddle objects.
		auto paddleTexture = Texture2D::Create("assets\\Sprite_PongPaddle.png");
		for (auto const& player : m_Players)
		{
			MiniGame::PlayerPaddle paddle;
			paddle.PlayerID = player.Settings.ID;
			paddle.Transform.Position = player.Settings.MainPaddleLocation;
			paddle.Sprite.Texture = paddleTexture;
			paddle.Sprite.Texture->PixelsPerUnit = 32;
			paddle.Collider.Size = {mainPaddleWidth, 3, 1};

			registerBoxCollider(paddle.Transform, paddle.Collider);
			m_PlayerPaddles.emplace_back(paddle);
		}

		// Create goal areas.
		for (auto const& player : m_Players)
		{
			MiniGame::PongHomebase homebase;
			homebase.PlayerID = player.Settings.ID;
			homebase.Transform.Position = player.Settings.HomebaseCenter;
			homebase.Collider.Size = player.Settings.HomebaseSize;

			m_Homebases.emplace_back(homebase);
		}

		// Create wall objects.
		MiniGame::Wall leftUpperWall; leftUpperWall.Transform.Position = {12, 8, 0}; leftUpperWall.Collider.Size = {1, 8, 0};
		MiniGame::Wall rightUpperWall; rightUpperWall.Transform.Position = {-12, 8, 0}; rightUpperWall.Collider.Size = {1, 8, 0};
		MiniGame::Wall leftLowerWall; leftLowerWall.Transform.Position = {12, -8, 0}; leftLowerWall.Collider.Size = {1, 8, 0};
		MiniGame::Wall rightLowerWall; rightLowerWall.Transform.Position = {-12, -8, 0}; rightLowerWall.Collider.Size = {1, 8, 0};

		MiniGame::Wall topWall; topWall.Transform.Position = {0, 6.5f, 0}; topWall.Collider.Size = {36, 1, 0};
		MiniGame::Wall bottomWall; bottomWall.Transform.Position = {0, -6.5f, 0}; bottomWall.Collider.Size = {36, 1, 0};

		m_Walls.emplace_back(leftLowerWall);
		m_Walls.emplace_back(rightLowerWall);
		m_Walls.emplace_back(leftUpperWall);
		m_Walls.emplace_back(rightUpperWall);
		m_Walls.emplace_back(topWall);
		m_Walls.emplace_back(bottomWall);

		for (auto& wall : m_Walls)
		{
			registerBoxCollider(wall.Transform, wall.Collider);
		}

		m_BorderSprite.Texture = Texture2D::Create("assets\\Sprite_PongBorder.png");
		m_BorderSprite.Texture->PixelsPerUnit = 32;
		m_BorderTransform.Position = {0, 0, 0};

		// Create background object.
		m_BackgroundSprite.Texture = Texture2D::Create("assets\\Sprite_Grid.png");
		m_BackgroundSprite.Texture->PixelsPerUnit = 32;
		m_BackgroundSprite.IsTiled = true;
		m_BackgroundTransform.Scale = {64.0f, 64.0f, 1};
		m_BackgroundTransform.Position = {0, 0, -2};

		m_CenterLineSprite.Texture = Texture2D::Create("assets\\Sprite_DottedLine.png");
		m_CenterLineSprite.Texture->PixelsPerUnit = 32;
		m_CenterLineSprite.IsTiled = true;
		m_CenterLineTransform.Scale = {1, 14.0f, 1};
		m_CenterLineTransform.Position = {0, 0, -1.5f};

		// Setup main camera for debugging purpose.
		m_MainWindow = WindowManager::GetMainWindow();
		m_MainWindow->SetSize(1600, 900);
		m_MainWindow->CenterWindow();

		m_MainCamera.Position = glm::vec3 {0, 0, 10};
		m_MainCamera.Properties.IsOrthographic = true;
		m_MainCamera.Properties.OrthographicSize = 14.07f;
		m_MainCamera.Properties.TargetType = RenderTargetType::Window;
		m_MainCamera.Properties.TargetWindowIndex = WindowManager::MainWindowIndex;
		m_MainCamera.Properties.UseManualAspectRatio = false;
		m_MainCamera.Properties.ManualAspectRatio = (float) 1600 / 900;
		m_MainCamera.Properties.ViewportValueType = ViewportValueType::RelativeDimension;
		m_MainCamera.Properties.Viewport = { 0, 0, 1, 1 };

		// Create window camera.
		m_Player1WindowCamera.CreateWindow(m_MainWindow->GetContext(), WindowProperties("Player 1 - Use Right Analog Stick To Move Window", 800, 900));
		m_Player1WindowCamera.GetWindowPtr()->SetBorderedIfWindowed(false);
		m_Player1WindowCamera.GetWindowPtr()->CenterWindow();
		auto position = m_Player1WindowCamera.GetWindowPtr()->GetPosition();
		position.x -= 400;
		m_Player1WindowCamera.GetWindowPtr()->SetPosition(position.x, position.y);
		m_Player1WindowCamera.ResetCachedPosition();

		m_Player2WindowCamera.CreateWindow(m_MainWindow->GetContext(), WindowProperties("Player 2 - Use Right Analog Stick To Move Window", 800, 900));
		m_Player2WindowCamera.GetWindowPtr()->SetBorderedIfWindowed(false);
		m_Player2WindowCamera.GetWindowPtr()->CenterWindow();
		position = m_Player2WindowCamera.GetWindowPtr()->GetPosition();
		position.x += 400;
		m_Player2WindowCamera.GetWindowPtr()->SetPosition(position.x, position.y);
		m_Player2WindowCamera.ResetCachedPosition();

		// Create UI objects.
		m_Player1Number.LoadTexture();
		m_Player1Number.SetValue(0);
		m_Player1Number.Transform.Position = {1, 5, -1};
		m_Player1Number.DigitDistanceOffset = 0.5f;

		m_Player2Number.LoadTexture();
		m_Player2Number.SetValue(0);
		m_Player2Number.Transform.Position = {-1, 5, -1};
		m_Player2Number.DigitDistanceOffset = 0.5f;

		m_GameOverUISprite.Texture = Texture2D::Create("assets\\Sprite_GameOver.png");
		m_GameOverUISprite.Texture->PixelsPerUnit = 32;
		m_GameOverUITransform.Scale = {1, 1, 1};
		m_GameOverUITransform.Position = {0, -1.5f, -1};

		m_WinnerUITransform.Scale = {1, 1, 1};
		m_WinnerUITransform.Position = {0, 0.5f, -1};
		m_P1WinsTexture = Texture2D::Create("assets\\Sprite_P1Wins.png");
		m_P1WinsTexture->PixelsPerUnit = 32;
		m_P2WinsTexture = Texture2D::Create("assets\\Sprite_P2Wins.png");
		m_P2WinsTexture->PixelsPerUnit = 32;
		m_WinnerUISprite.Texture = m_P1WinsTexture;

		// Set the current context back to the main window.
		m_MainWindow->MakeCurrent();
		ContextBase::SetVSyncCountForCurrentContext(0);

		// Equip the ball to the first paddle.
		m_Ball.EquipToPaddle(m_PlayerPaddles[0], m_Players[0].Settings.MainPaddleAttachOffset);

		// Hide the main window by default (for debugging, press F9/F10 to toggle it).
		m_MainWindow->Minimize();

		m_WindowParticlesManager.Initialize(12);
		m_WindowParticlesManager.HasMaxParticlesLimit = true;
		m_WindowParticlesManager.MaxParticlesLimit = 12;
	}

	void PongLayer::OnDetach()
	{
		WindowManager::CloseWindow(m_Player1WindowCamera.GetWindowPtr()->GetWindowID());
		WindowManager::CloseWindow(m_Player2WindowCamera.GetWindowPtr()->GetWindowID());

		m_WindowParticlesManager.Shutdown();
	}

	void PongLayer::registerBoxCollider(MiniGame::Transform &transform, MiniGame::BoxCollider &collider)
	{
		if (collider.ID.has_value() && m_ColliderManager.IsColliderRegistered(collider.ID.value()))
		{
			// The collider has already been registered to the manager.
			return;
		}

		collider.ID = m_ColliderManager.RegisterAABB(Math::AABB::CreateFromCenter(transform.Position, collider.Size));
	}

	void PongLayer::unregisterBoxCollider(MiniGame::Transform &transform, MiniGame::BoxCollider &collider)
	{
		if (!collider.ID.has_value())
		{
			// The collider doesn't have an ID.
			return;
		}

		if (!m_ColliderManager.IsColliderRegistered(collider.ID.value()))
		{
			// The collider is not registered to this manager.
			return;
		}

		m_ColliderManager.UnregisterAABB(collider.ID.value());
	}

	void PongLayer::OnRender()
	{
		RenderPipelineManager::RegisterCameraForNextRender(m_MainCamera);
		RenderPipelineManager::RegisterCameraForNextRender(m_Player1WindowCamera.Camera);
		RenderPipelineManager::RegisterCameraForNextRender(m_Player2WindowCamera.Camera);

		renderSprite(m_Ball.Transform, m_Ball.Sprite);
		renderSprite(m_BorderTransform, m_BorderSprite);
		for (auto& paddle : m_PlayerPaddles)
		{
			renderSprite(paddle.Transform, paddle.Sprite);
		}

		// Scroll background texture.
		float const backgroundTilingOffsetChange = TIME.DeltaTime() * m_BackgroundScrollingSpeed;
		m_BackgroundSprite.TilingOffset += glm::vec2 {backgroundTilingOffsetChange, backgroundTilingOffsetChange};
		if (m_BackgroundSprite.TilingOffset.x > 1.0f)
		{
			m_BackgroundSprite.TilingOffset -= glm::vec2 {1.0f, 1.0f};
		}
		renderSprite(m_BackgroundTransform, m_BackgroundSprite);

		// Scroll center dotted line texture.
		float const dottedLineTilingOffsetChange = TIME.DeltaTime() * m_CenterDottedLineScrollingSpeed;
		m_CenterLineSprite.TilingOffset -= glm::vec2 {0, dottedLineTilingOffsetChange};
		if (m_CenterLineSprite.TilingOffset.y < 0.0f)
		{
			m_CenterLineSprite.TilingOffset += glm::vec2 {0, 1.0f};
		}
		renderSprite(m_CenterLineTransform, m_CenterLineSprite);

		// Render UI sprites
		m_Player1Number.Render();
		m_Player2Number.Render();

		if (m_GameState == GameState::GameOver)
		{
			renderSprite(m_GameOverUITransform, m_GameOverUISprite);
			renderSprite(m_WinnerUITransform, m_WinnerUISprite);
		}
	}

	void PongLayer::renderSprite(MiniGame::Transform &transform, MiniGame::Sprite &sprite)
	{
		glm::mat4 modelMatrix = glm::mat4 {1.0f};
		modelMatrix = glm::translate(modelMatrix, transform.Position);
		modelMatrix = modelMatrix * glm::toMat4(transform.Rotation);
		modelMatrix = glm::scale(modelMatrix, transform.Scale);

		if (sprite.IsTiled)
		{
			RenderPipelineManager::GetTypedActiveRenderPipelinePtr<RenderPipeline2D>()
				->SubmitTiledSprite(sprite.Texture, {transform.Scale.x * sprite.TilingScale.x, transform.Scale.y * sprite.TilingScale.y, sprite.TilingOffset}, sprite.Color, modelMatrix);
		}
		else
		{
			RenderPipelineManager::GetTypedActiveRenderPipelinePtr<RenderPipeline2D>()
				->SubmitSprite(sprite.Texture, sprite.Color, modelMatrix);
		}
	}

	void PongLayer::OnUpdate()
	{
		// Debug updates.
		debugInput();
		m_FPSCounter.NewFrame(TIME.DeltaTime());
		if (m_DrawColliderGizmos)
		{
			m_ColliderManager.DrawGizmos();
			for (auto const& area : m_Homebases)
			{
				DebugDraw::Cube(area.Transform.Position, area.Collider.Size, Color::Yellow);
			}
		}

		// Gameplay updates
		readPlayerInput(TIME.DeltaTime());

		auto player1WindowAnimationResult =  m_Player1WindowCamera.UpdateWindowResizeAnimation(TIME.DeltaTime());
		auto player2WindowAnimationResult = m_Player2WindowCamera.UpdateWindowResizeAnimation(TIME.DeltaTime());

		if (m_GameState == GameState::Intermission)
		{
			bool const window1Complete = player1WindowAnimationResult == MiniGame::WindowCamera::AnimationUpdateResult::Complete ||
											player1WindowAnimationResult == MiniGame::WindowCamera::AnimationUpdateResult::Idle;
			bool const window2Complete = player2WindowAnimationResult == MiniGame::WindowCamera::AnimationUpdateResult::Complete ||
											player2WindowAnimationResult == MiniGame::WindowCamera::AnimationUpdateResult::Idle;

			if (window1Complete && window2Complete)
			{
				// Resume to playing state after the intermission animation is completed.
				m_GameState = GameState::Playing;
				m_Ball.PlayRespawnAnimation();
				if (m_pNextPaddleToSpawnBallAfterIntermission != nullptr && m_pNextPlayerToSpawnBall != nullptr)
				{
					// Attach the ball to the paddle!
					m_Ball.EquipToPaddle(*m_pNextPaddleToSpawnBallAfterIntermission, m_pNextPlayerToSpawnBall->Settings.MainPaddleAttachOffset);
				}
				else
				{
					DYE_LOG_ERROR("Next spawn ball paddle / player is nullptr, something is wrong.");

					// Reset the ball to the center
					m_Ball.Transform.Position = {0, 0, 0};
					m_Ball.Velocity.Value = {5, 1};
				}
			}
		}

		m_Player1WindowCamera.UpdateCameraProperties();
		m_Player2WindowCamera.UpdateCameraProperties();

		// Animation updates
		m_RippleEffectManager.OnUpdate(TIME.DeltaTime());
		m_WindowParticlesManager.OnUpdate(TIME.DeltaTime());

		m_Player1Number.UpdateAnimation(TIME.DeltaTime());
		m_Player2Number.UpdateAnimation(TIME.DeltaTime());

		m_Ball.UpdateAnimation(TIME.DeltaTime());

		if (m_GameState == GameState::GameOver)
		{
			// Slow down the scrolling if the game is over.
			if (m_BackgroundScrollingSpeed <= 0.0f)
			{
				m_BackgroundScrollingSpeed = 0.0f;
			}
			else
			{
				m_BackgroundScrollingSpeed -= TIME.DeltaTime();
			}
		}
	}

	void PongLayer::debugInput()
	{
		if (INPUT.GetKeyDown(KeyCode::F9))
		{
			m_DrawImGui = !m_DrawImGui;
		}

		if (INPUT.GetKeyDown(KeyCode::Escape))
		{
			auto& miniGameApp = static_cast<DYETechDemoApp&>(m_Application);
			miniGameApp.LoadMainMenuLayer();
		}
	}

	void PongLayer::readPlayerInput(float timeStep)
	{
		if (m_GameState == GameState::GameOver)
		{
			auto& miniGameApp = static_cast<DYETechDemoApp&>(m_Application);
			if (INPUT.IsGamepadConnected(0) && INPUT.GetGamepadButton(0, GamepadButton::South))
			{
				miniGameApp.LoadMainMenuLayer();
			}
			else if (INPUT.IsGamepadConnected(1) && INPUT.GetGamepadButton(1, GamepadButton::South))
			{
				miniGameApp.LoadMainMenuLayer();
			}
		}

		// Player 1
		{
			auto &paddle = m_PlayerPaddles[0];
			auto &player = m_Players[0];
			MiniGame::WindowCamera &playerWindowCamera = m_Player1WindowCamera;
			if (INPUT.IsGamepadConnected(0))
			{
				float vertical = INPUT.GetGamepadAxis(0, GamepadAxis::LeftStickVertical);
				if (INPUT.GetGamepadButton(0, GamepadButton::DPadUp))
				{
					vertical = 1.0f;
				}
				else if (INPUT.GetGamepadButton(0, GamepadButton::DPadDown))
				{
					vertical = -1.0f;
				}

				glm::vec3 const axis = {0, vertical, 0};

				if (glm::length2(axis) > 0.01f)
				{
					paddle.MovementInputBuffer = axis;
				}

				if (INPUT.GetGamepadButtonDown(0, GamepadButton::South))
				{
					if (m_Ball.Attachable.IsAttached && m_Ball.Attachable.AttachedPaddle == &paddle)
					{
						m_Ball.LaunchFromAttachedPaddle();
					}
				}

				if (m_GameState != GameState::Intermission && player.State.CanMoveWindow)
				{
					float const windowVertical = INPUT.GetGamepadAxis(0, GamepadAxis::RightStickVertical);
					float const windowHorizontal = INPUT.GetGamepadAxis(0, GamepadAxis::RightStickHorizontal);

					glm::vec2 windowAxis = {windowHorizontal, windowVertical};
					if (glm::length2(windowAxis) > 0.01f)
					{
						if (glm::length2(windowAxis) > 1.0f)
						{
							windowAxis = glm::normalize(windowAxis);
						}
						glm::vec2 positionChange = windowAxis * playerWindowCamera.MoveSpeed * timeStep;
						positionChange.y = -positionChange.y;
						playerWindowCamera.Translate(positionChange);
					}
				}
			}
			else
			{
				if (INPUT.GetKey(KeyCode::W))
				{
					paddle.MovementInputBuffer = glm::vec2 {0, 1};
				}
				else if (INPUT.GetKey(KeyCode::S))
				{
					paddle.MovementInputBuffer = glm::vec2 {0, -1};
				}

				if (INPUT.GetKeyDown(KeyCode::Space))
				{
					if (m_Ball.Attachable.IsAttached && m_Ball.Attachable.AttachedPaddle == &paddle)
					{
						m_Ball.LaunchFromAttachedPaddle();
					}
				}

				if (m_GameState != GameState::Intermission && player.State.CanMoveWindow)
				{
					float windowVertical = 0;
					if (INPUT.GetKey(KeyCode::T))
					{
						windowVertical = 1;
					}
					else if (INPUT.GetKey(KeyCode::G))
					{
						windowVertical = -1;
					}

					float windowHorizontal = 0;
					if (INPUT.GetKey(KeyCode::H))
					{
						windowHorizontal = 1;
					}
					else if (INPUT.GetKey(KeyCode::F))
					{
						windowHorizontal = -1;
					}

					glm::vec2 windowAxis = {windowHorizontal, windowVertical};
					if (glm::length2(windowAxis) > 0.01f)
					{
						if (glm::length2(windowAxis) > 1.0f)
						{
							windowAxis = glm::normalize(windowAxis);
						}
						glm::vec2 positionChange = windowAxis * playerWindowCamera.MoveSpeed * timeStep;
						positionChange.y = -positionChange.y;
						playerWindowCamera.Translate(positionChange);
					}
				}
			}
		}

		// Player 2
		{
			auto &paddle = m_PlayerPaddles[1];
			auto &player = m_Players[1];
			auto &playerWindowCamera = m_Player2WindowCamera;
			if (INPUT.IsGamepadConnected(1))
			{
				float vertical = INPUT.GetGamepadAxis(1, GamepadAxis::LeftStickVertical);
				if (INPUT.GetGamepadButton(1, GamepadButton::DPadUp))
				{
					vertical = 1.0f;
				}
				else if (INPUT.GetGamepadButton(1, GamepadButton::DPadDown))
				{
					vertical = -1.0f;
				}

				glm::vec3 const axis = {0, vertical, 0};

				if (glm::length2(axis) > 0.01f)
				{
					paddle.MovementInputBuffer = axis;
				}

				if (INPUT.GetGamepadButtonDown(1, GamepadButton::South))
				{
					if (m_Ball.Attachable.IsAttached && m_Ball.Attachable.AttachedPaddle == &paddle)
					{
						m_Ball.LaunchFromAttachedPaddle();
					}
				}

				if (m_GameState != GameState::Intermission && player.State.CanMoveWindow)
				{
					float const windowVertical = INPUT.GetGamepadAxis(1, GamepadAxis::RightStickVertical);
					float const windowHorizontal = INPUT.GetGamepadAxis(1, GamepadAxis::RightStickHorizontal);
					glm::vec2 windowAxis = {windowHorizontal, windowVertical};
					if (glm::length2(windowAxis) > 0.01f)
					{
						if (glm::length2(windowAxis) > 1.0f)
						{
							windowAxis = glm::normalize(windowAxis);
						}
						glm::vec2 positionChange = windowAxis * playerWindowCamera.MoveSpeed * timeStep;
						positionChange.y = -positionChange.y;
						playerWindowCamera.Translate(positionChange);
					}
				}
			}
			else
			{
				if (INPUT.GetKey(KeyCode::Up))
				{
					paddle.MovementInputBuffer = glm::vec2 {0, 1};
				}
				else if (INPUT.GetKey(KeyCode::Down))
				{
					paddle.MovementInputBuffer = glm::vec2 {0, -1};
				}

				if (INPUT.GetKeyDown(KeyCode::Return))
				{
					if (m_Ball.Attachable.IsAttached && m_Ball.Attachable.AttachedPaddle == &paddle)
					{
						m_Ball.LaunchFromAttachedPaddle();
					}
				}

				if (m_GameState != GameState::Intermission && player.State.CanMoveWindow)
				{
					float windowVertical = 0;
					if (INPUT.GetKey(KeyCode::Numpad5))
					{
						windowVertical = 1;
					}
					else if (INPUT.GetKey(KeyCode::Numpad2))
					{
						windowVertical = -1;
					}

					float windowHorizontal = 0;
					if (INPUT.GetKey(KeyCode::Numpad3))
					{
						windowHorizontal = 1;
					}
					else if (INPUT.GetKey(KeyCode::Numpad1))
					{
						windowHorizontal = -1;
					}

					glm::vec2 windowAxis = {windowHorizontal, windowVertical};
					if (glm::length2(windowAxis) > 0.01f)
					{
						if (glm::length2(windowAxis) > 1.0f)
						{
							windowAxis = glm::normalize(windowAxis);
						}
						glm::vec2 positionChange = windowAxis * playerWindowCamera.MoveSpeed * timeStep;
						positionChange.y = -positionChange.y;
						playerWindowCamera.Translate(positionChange);
					}
				}
			}
		}
	}

	void PongLayer::updateBoxCollider(MiniGame::Transform &transform, MiniGame::BoxCollider &collider)
	{
		if (!collider.ID.has_value())
		{
			// The collider doesn't have an ID.
			return;
		}

		if (!m_ColliderManager.IsColliderRegistered(collider.ID.value()))
		{
			// The collider is not registered to this manager.
			return;
		}

		m_ColliderManager.SetAABB(collider.ID.value(), Math::AABB::CreateFromCenter(transform.Position, collider.Size));
	}

	void PongLayer::OnFixedUpdate()
	{
		auto const timeStep = (float) TIME.FixedDeltaTime();
		for (auto& paddle : m_PlayerPaddles)
		{
			updatePaddle(paddle, timeStep);
		}
		updateBall(timeStep);

		if (m_GameState == GameState::Playing)
		{
			checkIfBallHasReachedGoal(timeStep);
		}
	}

	void PongLayer::updatePaddle(MiniGame::PlayerPaddle& paddle, float timeStep)
	{
		float const inputSqr = glm::length2(paddle.MovementInputBuffer);
		if (inputSqr <= glm::epsilon<float>())
		{
			paddle.VelocityBuffer = {0, 0};
			return;
		}

		// Calculate the paddleVelocityForFrame
		float const magnitude = glm::length(paddle.MovementInputBuffer);
		glm::vec3 const direction = glm::normalize(glm::vec3 {paddle.MovementInputBuffer, 0});
		glm::vec3 const paddleVelocity = paddle.Speed * magnitude * direction;
		glm::vec3 const paddleVelocityForFrame = paddleVelocity * timeStep;

		// Calculate the new position for the paddle.
		// Clamp the position & velocity if it's over the max/min constraints.
		auto newPaddlePosition = paddle.Transform.Position + paddleVelocityForFrame;
		bool isPaddleVelocityClamped = false;
		if (newPaddlePosition.y < paddle.MinPositionY)
		{
			isPaddleVelocityClamped = true;
			newPaddlePosition.y = paddle.MinPositionY;
		}
		if (newPaddlePosition.y > paddle.MaxPositionY)
		{
			isPaddleVelocityClamped = true;
			newPaddlePosition.y = paddle.MaxPositionY;
		}

		auto actualPositionOffset = newPaddlePosition - paddle.Transform.Position;
		auto actualPositionOffsetInSecond = actualPositionOffset / timeStep;

		// In case when the velocity is clamped, we will buffer the actual position offset instead.
		paddle.VelocityBuffer = isPaddleVelocityClamped? actualPositionOffsetInSecond : paddleVelocity;

		// Push the ball if there is an overlap.
		Math::DynamicTestResult2D result2D;
		bool const intersectBall = Math::MovingCircleAABBIntersect(m_Ball.Transform.Position, m_Ball.Collider.Radius,
																   -actualPositionOffset, paddle.GetAABB(), result2D);
		if (intersectBall)
		{
			glm::vec2 const normal = glm::normalize(result2D.HitNormal);
			auto ballPaddleDot = glm::dot(actualPositionOffset, glm::vec3 {m_Ball.Velocity.Value, 0});

			if (ballPaddleDot < 0.0f)
			{
				// If the ball is moving in the opposite direction of the paddle,
				// bounce (deflect) the ball into reflected direction.
				m_Ball.Velocity.Value = m_Ball.Velocity.Value - 2 * glm::dot(normal, m_Ball.Velocity.Value) * normal;
				m_Ball.Velocity.Value += glm::vec2 {paddle.VelocityBuffer.x, paddle.VelocityBuffer.y};
			}

			// Move the ball away from the paddle to avoid tunneling
			m_Ball.Velocity.Value.x += glm::sign(m_Ball.Velocity.Value.x) * paddle.HorizontalBallSpeedIncreasePerHit;
			if (glm::length2(m_Ball.Velocity.Value) > m_Ball.MaxBallSpeed * m_Ball.MaxBallSpeed)
			{
				// Cap the ball speed to the max speed.
				m_Ball.Velocity.Value = glm::normalize(m_Ball.Velocity.Value) * m_Ball.MaxBallSpeed;
			}

			m_Ball.Transform.Position += actualPositionOffset;
			m_Ball.Hittable.LastHitByPlayerID = paddle.PlayerID;
			m_Ball.PlayHitAnimation();

			playOnBounceEffect(result2D.HitPoint);
		}

		// Actually update the paddle and its collider.
		paddle.Transform.Position = newPaddlePosition;
		paddle.MovementInputBuffer = {0, 0};
		updateBoxCollider(paddle.Transform, paddle.Collider);
	}

	void PongLayer::updateBall(float timeStep)
	{
		if (m_Ball.Attachable.AttachedPaddle != nullptr)
		{
			// The ball is attached to a paddle, move the ball with the paddle
			m_Ball.Transform.Position = m_Ball.Attachable.AttachedPaddle->Transform.Position + glm::vec3 {m_Ball.Attachable.AttachOffset, 0};
			return;
		}

		// Ball collision detection.
		glm::vec2 const positionChange = timeStep * m_Ball.Velocity.Value;
		auto const& hits = m_ColliderManager.CircleCastAll(m_Ball.Transform.Position, 0.25f, positionChange);
		if (!hits.empty())
		{
			auto const& hit = hits[0];
			glm::vec2 const normal = glm::normalize(hit.Normal);

			float const minimumTravelTimeAfterReflected = 0.001f;
			float const reflectedTravelTime = glm::max(minimumTravelTimeAfterReflected, timeStep - hit.Time);
			m_Ball.Velocity.Value = m_Ball.Velocity.Value - 2 * glm::dot(normal, m_Ball.Velocity.Value) * normal;
			m_Ball.Transform.Position = glm::vec3(hit.Centroid + reflectedTravelTime * m_Ball.Velocity.Value, 0);

			for (auto const& paddle : m_PlayerPaddles)
			{
				// If the box is a paddle, update velocity based on the paddle's state.
				if (hit.ColliderID != paddle.Collider.ID)
				{
					continue;
				}

				auto paddleVelocity = paddle.VelocityBuffer;

				// Increase the horizontal speed if it's a paddle.
				m_Ball.Velocity.Value.x += glm::sign(m_Ball.Velocity.Value.x) * paddle.HorizontalBallSpeedIncreasePerHit;

				// Add paddle velocity to the ball.
				m_Ball.Velocity.Value += glm::vec2 {paddleVelocity.x, paddleVelocity.y};

				if (glm::length2(m_Ball.Velocity.Value) > m_Ball.MaxBallSpeed * m_Ball.MaxBallSpeed)
				{
					// Cap the ball speed to the max speed.
					m_Ball.Velocity.Value = glm::normalize(m_Ball.Velocity.Value) * m_Ball.MaxBallSpeed;
				}

				m_Ball.Hittable.LastHitByPlayerID = paddle.PlayerID;
				m_Ball.PlayHitAnimation();

				break;
			}

			playOnBounceEffect(hit.Point);
		}
		else
		{
			m_Ball.Transform.Position += glm::vec3(positionChange, 0);
		}
	}

	void PongLayer::playOnBounceEffect(glm::vec2 worldPos)
	{
		m_RippleEffectManager.SpawnRippleAt(
			worldPos,
			RippleEffectParameters
				{
					.LifeTime = 1.0f,
					.StartRadius = 0.25f,
					.EndRadius = 0.75f,
					.StartColor = Color::Black,
					.EndColor = {0, 0, 0, 0}
				}
		);

		auto const contactPoint = worldPos;
		std::int32_t contactScreenPosX = contactPoint.x * 64;
		std::int32_t contactScreenPosY = contactPoint.y * 64;
		contactScreenPosY = m_ScreenDimensions.y - contactScreenPosY;

		contactScreenPosX += m_ScreenDimensions.x * 0.5f;
		contactScreenPosY -= m_ScreenDimensions.y * 0.5f;
		m_WindowParticlesManager.CircleEmitParticlesAt(
			{contactScreenPosX, contactScreenPosY},
			CircleEmitParams
				{
					.NumberOfParticles = 3,
					.Gravity = 0,
					.InitialMinSpeed = 250,
					.InitialMaxSpeed = 400,
					.DecelerationPerSecond = 450,
				}
		);
	}

	void PongLayer::checkIfBallHasReachedGoal(float timeStep)
	{
		// Check if it's a goal.
		for (auto const& homebase : m_Homebases)
		{
			if (!Math::AABBCircleIntersect(homebase.GetAABB(), m_Ball.Transform.Position, m_Ball.Collider.Radius))
			{
				continue;
			}

			// Deal 1 damage to the homebase.
			int const damagedPlayerID = homebase.PlayerID;
			auto playerItr = std::find_if(
				m_Players.begin(),
				m_Players.end(),
				[damagedPlayerID](MiniGame::PongPlayer const& player)
				{
					return damagedPlayerID == player.Settings.ID;
				});

			if (playerItr != m_Players.end())
			{
				m_pNextPlayerToSpawnBall = &(*playerItr);

				// Reduce health (earn score).
				playerItr->State.Health--;

				if (damagedPlayerID == 0)
				{
					m_Player1Number.SetValue(MaxHealth - playerItr->State.Health);
					m_Player1Number.PlayPopAnimation();
				}
				else
				{
					m_Player2Number.SetValue(MaxHealth - playerItr->State.Health);
					m_Player2Number.PlayPopAnimation();
				}

				if (playerItr->State.Health == 0)
				{
					m_GameState = GameState::GameOver;
					m_MainWindow->Restore();
					m_Player1WindowCamera.GetWindowPtr()->Raise();
					m_Player2WindowCamera.GetWindowPtr()->Raise();

					m_WinnerUISprite.Texture = damagedPlayerID == 0? m_P2WinsTexture : m_P1WinsTexture;
				}
				else
				{
					m_GameState = GameState::Intermission;

					// Shrink the winning player's window size.
					MiniGame::WindowCamera& windowCameraToShrink = damagedPlayerID == 0? m_Player2WindowCamera : m_Player1WindowCamera;
					MiniGame::PongPlayer& shrunkPlayer = damagedPlayerID == 0 ? m_Players[1] : m_Players[0];

					int const sizeIndex = WindowSizesCount - playerItr->State.Health;
					if (sizeIndex >= 0 && sizeIndex < HealthWindowSizes.size())
					{
						auto size = HealthWindowSizes[sizeIndex];
						windowCameraToShrink.SmoothResize(size.x, size.y);
					}

					// Enable window control/show border if the window shrinks.
					if (playerItr->State.Health <= HealthToEnableWindowInput)
					{
						windowCameraToShrink.GetWindowPtr()->SetBorderedIfWindowed(true);
						shrunkPlayer.State.CanMoveWindow = true;
					}
				}
			}
			else
			{
				m_pNextPlayerToSpawnBall = nullptr;
			}

			auto paddleItr = std::find_if(
				m_PlayerPaddles.begin(),
				m_PlayerPaddles.end(),
				[damagedPlayerID](MiniGame::PlayerPaddle const& paddle)
				{
					return damagedPlayerID == paddle.PlayerID;
				});

			if (paddleItr != m_PlayerPaddles.end())
			{
				m_pNextPaddleToSpawnBallAfterIntermission = &(*paddleItr);
			}
			else
			{
				m_pNextPaddleToSpawnBallAfterIntermission = nullptr;
			}

			// Play animations.
			m_Ball.PlayGoalAnimation();

			auto const contactPoint = m_Ball.Transform.Position;
			std::int32_t contactScreenPosX = contactPoint.x * 64;
			std::int32_t contactScreenPosY = contactPoint.y * 64;
			contactScreenPosY = m_ScreenDimensions.y - contactScreenPosY;

			contactScreenPosX += m_ScreenDimensions.x * 0.5f;
			contactScreenPosY -= m_ScreenDimensions.y * 0.5f;

			m_WindowParticlesManager.CircleEmitParticlesAt(
				{contactScreenPosX, contactScreenPosY},
				CircleEmitParams
					{
						.NumberOfParticles = 12,
						.Gravity = 0,
						.DecelerationPerSecond = 900
					}
			);

			break;
		}
	}

	void PongLayer::OnImGui()
	{
		if (!m_DrawImGui)
		{
			return;
		}

		if (ImGui::Begin("General"))
		{
			if (ImGui::CollapsingHeader("System"))
			{
				ImGui::Dummy(ImVec2(0.0f, 1.0f));
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Platform");
				ImGui::Text("%s", SDL_GetPlatform());
				ImGui::Text("CPU cores: %d", SDL_GetCPUCount());
				ImGui::Text("RAM: %.2f GB", (float) SDL_GetSystemRAM() / 1024.0f);
				ImGui::Text("DeltaTime: [%f]", TIME.DeltaTime());
				ImGui::Text("FPS: [%f]", m_FPSCounter.GetLastCalculatedFPS());
			}

			if (ImGui::CollapsingHeader("Game State"))
			{
				std::string state;
				if (m_GameState == GameState::Playing)
				{
					state = "Playing";
				}
				else if (m_GameState == GameState::Intermission)
				{
					state = "Intermission";
				}
				else if (m_GameState == GameState::GameOver)
				{
					state = "GameOver";
				}

				ImGuiUtil::DrawReadOnlyTextWithLabel("Game State", state);

				for (int i = 0; i < m_Players.size(); i++)
				{
					auto& player = m_Players[i];
					ImGuiUtil::DrawIntControl("Player " + std::to_string(i) + " Health", player.State.Health, 0);
				}

				if (m_Ball.Attachable.IsAttached)
				{
					ImGuiUtil::DrawReadOnlyTextWithLabel("Ball Attach To", std::to_string(m_Ball.Attachable.AttachedPaddle->PlayerID));
				}
				else
				{
					ImGuiUtil::DrawVector2Control("Ball Velocity", m_Ball.Velocity.Value, 0.0f);
				}
			}

			if (ImGui::CollapsingHeader("Window & Mouse", ImGuiTreeNodeFlags_DefaultOpen))
			{
				int const mainDisplayIndex = m_MainWindow->GetDisplayIndex();
				ImGui::Text("MainWindowDisplayIndex = %d", mainDisplayIndex);
				std::optional<DisplayMode> const displayMode = SCREEN.TryGetDisplayMode(mainDisplayIndex);
				if (displayMode.has_value())
				{
					ImGui::Text("Display %d Info = (w=%d, h=%d, r=%d)", mainDisplayIndex, displayMode->Width, displayMode->Height, displayMode->RefreshRate);
				}

				ImGui::Text("Mouse Position = %d, %d", INPUT.GetGlobalMousePosition().x, INPUT.GetGlobalMousePosition().y);
				ImGui::Text("Mouse Delta = %d, %d", INPUT.GetGlobalMouseDelta().x, INPUT.GetGlobalMouseDelta().y);

				if (ImGui::Button("Quit App"))
				{
					m_Application.Shutdown();
				}

				auto& miniGamesApp = static_cast<DYETechDemoApp&>(m_Application);
				ImGui::SameLine();
				if (ImGui::Button("Main Menu"))
				{
					miniGamesApp.LoadMainMenuLayer();
				}
				ImGui::SameLine();
				if (ImGui::Button("Pong"))
				{
					miniGamesApp.LoadPongLayer();
				}
				ImGui::SameLine();
				if (ImGui::Button("Land Ball"))
				{
					miniGamesApp.LoadLandBallLayer();
				}

				static bool isMainWindowBordered = true;
				if (ImGui::Button("Toggle Window Bordered"))
				{
					isMainWindowBordered = !isMainWindowBordered;
					m_MainWindow->SetBorderedIfWindowed(isMainWindowBordered);
				}

				ImGui::SameLine();
				if (ImGui::Button("Center Window"))
				{
					m_MainWindow->CenterWindow();
				}

				if (ImGui::Button("Minimize Window"))
				{
					m_MainWindow->Minimize();
				}
				ImGui::SameLine();
				if (ImGui::Button("Restore Window"))
				{
					m_MainWindow->Restore();
				}
			}

			if (ImGui::CollapsingHeader("Rendering"))
			{
				if (ImGui::Button("Line Mode"))
				{
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				}
				ImGui::SameLine();
				if (ImGui::Button("Fill Mode"))
				{
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				}

				if (ImGui::Button("Play Ball Respawn Animation"))
				{
					m_Ball.PlayRespawnAnimation();
				}

				ImGui::SameLine();
				if (ImGui::Button("Play Ball Hit Animation"))
				{
					m_Ball.PlayHitAnimation();
				}

				ImGui::SameLine();
				if (ImGui::Button("Play Ball Goal Animation"))
				{
					m_Ball.PlayGoalAnimation();
				}

				if (ImGui::Button("Toggle Debug Gizmos"))
				{
					m_DrawColliderGizmos = !m_DrawColliderGizmos;
				}
				ImGuiUtil::DrawFloatControl("BG Scrolling Speed", m_BackgroundScrollingSpeed, 1.0f);
			}

			if (ImGui::CollapsingHeader("World"))
			{
				ImGuiUtil::DrawCameraPropertiesControl("Main Camera Properties", m_MainCamera.Properties);

				ImGui::Separator();
				imguiSprite("PongBall", m_Ball.Transform, m_Ball.Sprite);
				ImGui::Separator();
				imguiSprite("Background", m_BackgroundTransform, m_BackgroundSprite);
			}
		}
		ImGui::End();

		m_ColliderManager.DrawImGui();
		INPUT.DrawInputManagerImGui();
		WindowManager::DrawWindowManagerImGui();
	}

	void PongLayer::imguiSprite(const std::string& name, MiniGame::Transform& transform, MiniGame::Sprite& sprite)
	{
		ImGui::PushID(name.c_str());

		ImGuiUtil::DrawVector3Control("Position", transform.Position);
		ImGuiUtil::DrawVector3Control("Scale", transform.Scale);

		glm::vec3 rotationInEulerAnglesDegree = glm::eulerAngles(transform.Rotation);
		rotationInEulerAnglesDegree += glm::vec3(0.f);
		rotationInEulerAnglesDegree = glm::degrees(rotationInEulerAnglesDegree);
		if (ImGuiUtil::DrawVector3Control("Rotation", rotationInEulerAnglesDegree))
		{
			rotationInEulerAnglesDegree.y = glm::clamp(rotationInEulerAnglesDegree.y, -90.f, 90.f);
			transform.Rotation = glm::quat {glm::radians(rotationInEulerAnglesDegree)};
		}

		ImGuiUtil::DrawVector2Control("_TilingScale", sprite.TilingScale, 1.0f);
		ImGuiUtil::DrawVector2Control("_TilingOffset", sprite.TilingOffset, 0.0f);
		ImGuiUtil::DrawColor4Control("_Color", sprite.Color);

		ImGui::PopID();
	}
}