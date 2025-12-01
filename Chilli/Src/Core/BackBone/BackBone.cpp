#include "BackBone.h"
#include "Ch_PCH.h"
#include "BackBone.h"
#include "DeafultExtensions.h"

namespace Chilli
{
	namespace BackBone
	{
		uint32_t GetNewResourceID() {
			static uint32_t NewID = 0;
			return NewID++;
		}

		uint32_t GetNewComponentID()
		{
			static uint32_t NewComponentID = 0;
			return NewComponentID++;
		}

		uint32_t GetNewAssetID()
		{
			static uint32_t NewAssetID = 0;
			return NewAssetID++;
		}

		uint32_t GetNewServiceID()
		{
			static uint32_t NewServiceID = 0;
			return NewServiceID++;
		}

		void Schedule::AddSystem(ScheduleTimer Stage, const std::function<void(SystemContext&)>& Function)
		{
			_SystemFunctions[int(Stage)].push_back(Function);
		}

		void Schedule::AddSystemOverLayBefore(ScheduleTimer Stage, const std::function<void(SystemContext&)>& Function)
		{
			_SystemOverLayBefore[int(Stage)].push_back(Function);
		}

		void Schedule::AddSystemOverLayAfter(ScheduleTimer Stage, const std::function<void(SystemContext&)>& Function)
		{
			_SystemOverLayAfter[int(Stage)].push_back(Function);
		}

		void Schedule::Run(ScheduleTimer Stage, SystemContext& Ctxt)
		{
			{
				auto& SystemFuncss = _SystemOverLayBefore[int(Stage)];
				for (auto& ScheduledSytem : SystemFuncss)
					ScheduledSytem(Ctxt);
			}
			{
				auto& SystemFuncss = _SystemFunctions[int(Stage)];
				for (auto& ScheduledSytem : SystemFuncss)
					ScheduledSytem(Ctxt);
			} 
			{
				auto& SystemFuncss = _SystemOverLayAfter[int(Stage)];
				for (auto& ScheduledSytem : SystemFuncss)
					ScheduledSytem(Ctxt);
			}
		}
		// Extensions
		void ExtensionRegistry::AddExtension(std::unique_ptr<Extension> Ext, bool BuildNow, App* app)
		{
			if (BuildNow)
				Ext->Build(*app);
			Extensions[Ext->ID()] = std::move(Ext);
		}

		void BackBone::ExtensionRegistry::BuildAll(App& app)
		{
		}

		void App::Run()
		{
			// Initialition
			SystemScheduler.Run(ScheduleTimer::START_UP, Ctxt);
			Registry.AddResource<GenericFrameData>();
			auto WindowServie = ServiceRegistry.GetService<WindowManager>();

			bool Is_Running = true;
			float LastTime = 0.0f;

			const int Count = 200;
			std::array<float, Count> FrameTimes;
			int idx = 0;

			auto FrameData = Registry.GetResource<GenericFrameData>();
			while (FrameData->IsRunning)
			{
				FrameData->Ts = GetWindowTime() - LastTime;
				LastTime = GetWindowTime();

				if (idx < Count)
				{
					CH_CORE_TRACE("FPS: {}, Time: {} ms", 1 / FrameData->Ts.GetSecond()
						, FrameData->Ts.GetMilliSecond());
					FrameTimes[idx++] = FrameData->Ts.GetMilliSecond();
				}

				static bool Printed = false;
				if (!Printed && idx > Count)
				{
					CH_CORE_TRACE("Frame Time Record Done!");
					Printed = true;
				}

				SystemScheduler.Run(ScheduleTimer::UPDATE, Ctxt);

				SystemScheduler.Run(ScheduleTimer::RENDER, Ctxt);

				if (FrameData->IsRunning == false)
					CH_CORE_INFO("Window Close");
			}

			float AverageMs = 0.0f;
			for (auto Data : FrameTimes)
				AverageMs += Data / Count;
			CH_CORE_TRACE("Average Ms Over {} Frames: {}", Count, AverageMs);
			CH_CORE_TRACE("Average Frame Rate: {}", 1000 / AverageMs);
			// ShutDown
			SystemScheduler.Run(ScheduleTimer::SHUTDOWN, Ctxt);
		}
	}
}
