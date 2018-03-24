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

		/* Replace T with new one, if entity doesn't have T, do Add only.
		This will trigger Removed(if there is a component before replace) and Added event at once.
		To avoid that, use Get() and set fields manually.
		*/
		template<typename T>
		void Replace(T&& val) {
			world->RemoveComponent<T>(entityID);
			(*world->AddComponent<T>(entityID)) = val;
		}

		/* Get pointer to T. 
		Will return nullptr if this component doesn't exist.
		*/
		template<typename T>
		T* Get() {
			return world->GetComponent<T>(entityID);
		}

		/* Check if the entity has T */
		template<typename T>
		bool Has() {
			int compIndex = world->ConvertComponentTypeToIndex<T>();
			return world->HasComponent(entityID,compIndex);
		}

		/* Add a T to the entity.
		Will throw exception if T already exists.
		*/
		template<typename T>
		T* Add(T val) {
			auto p = world->AddComponent<T>(entityID);
			*p = val;
			return p;
		}

		/* Remove a component form entity.
		Throw exception if entity doesn't have T.
		*/
		template<typename T>
		void Remove() {
			int compIndex = world->ConvertComponentTypeToIndex<T>();
			world->RemoveComponent(entityID, compIndex);
		}
	};
}