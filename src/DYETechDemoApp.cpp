#include "Core/Application.h"
#include "AppEntryPoint.h"

namespace DYE
{
	class DYETechDemoApp final : public Application
	{
	public:
		DYETechDemoApp() = delete;
		DYETechDemoApp(const DYETechDemoApp &) = delete;

		explicit DYETechDemoApp(const std::string &windowName, int fixedFramePerSecond = 60)
			: Application(windowName, fixedFramePerSecond)
		{
		}

		~DYETechDemoApp() final = default;
	};

	Application *CreateApplication()
	{
		DYE_LOG("TEST");
		return new DYETechDemoApp("DYEngine Tech Demo");
	}
}