#pragma once

namespace Resecs {
	class System {
	public:
		virtual void Start() {}
		virtual void Update() {}
	};

	class Feature : public System {
	public:
		std::vector<std::shared_ptr<System>> systems;
		virtual void Start() {
			for (auto pSys : systems) {
				pSys->Start();
			}
		}
		virtual void Update() {
			for (auto pSys : systems) {
				pSys->Update();
			}
		}
	};
}