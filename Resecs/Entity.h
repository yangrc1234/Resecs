#pragma once
#include "EntityID.hpp"

namespace Resecs {
	
	class Entity {
	private:
		friend class World;
		World* world;  //reference to world.
		Entity(World* world, EntityID entityID);
	public:
		EntityID entityID;

		bool IsAlive();

		void Destroy();

		/* Replace the component of type T with new one, if entity doesn't have T, do Add only.
		This will trigger Removed(if there is a component before replace) and Added event at once.
		To avoid that, use Get() and set fields manually.
		*/
		template<typename T>
		void Replace(T&& val) {
			world->RemoveComponent<T>(entityID);
			(*world->AddComponent<T>(entityID)) = val;
		}

		/* Get pointer to the component of type T. 
		Will return nullptr if this component doesn't exist.
		*/
		template<typename T>
		T* Get() {
			return world->GetComponent<T>(entityID);
		}

		/* Check if the entity has component of type T */
		template<typename T>
		bool Has() {
			return world->HasComponent<T>(entityID);
		}

		/* Add a component of type T to the entity.
		Will throw exception if T already exists.
		*/
		template<typename T>
		T* Add(T val) {
			auto p = world->AddComponent<T>(entityID);
			*p = val;
			return p;
		}
	};
}