#pragma once
#include "EntityID.hpp"

namespace Resecs {
	
	class Entity {
	private:
		friend class World;
		World* world;  //reference to world.
	public:
		Entity() {}
		Entity(World* world, EntityID entityID);
		EntityID entityID;
		bool IsAlive();
		void Destroy();
		/* Replace T with new one, if entity doesn't have T, do Add only.
		This will trigger Removed(if there is a component before replace) and Added event at once.
		To avoid that, use Get() and set fields manually.
		*/
		template<typename T, typename... TArgs>
		void Replace(TArgs&&... args) {
			ThrowIfSingletonTestFailed<T>();
			if (Has<T>())
				Remove<T>();
			world->AddComponent<T>(entityID, std::forward<TArgs>(args)...);
		}

		/* Get pointer to T. 
		Will return nullptr if this component doesn't exist.
		*/
		template<typename T>
		T* Get() {
			ThrowIfSingletonTestFailed<T>();
			return world->GetComponent<T>(entityID);
		}

		/* Check if the entity has T */
		template<typename T>
		bool Has() {
			ThrowIfSingletonTestFailed<T>();
			int compIndex = world->ConvertComponentTypeToIndex<T>();
			return world->HasComponent(entityID,compIndex);
		}

		/* Add a T to the entity.
		Will throw exception if T already exists.
		*/
		template<typename T,typename... TArgs>
		T* Add(TArgs&&... args) {
			ThrowIfSingletonTestFailed<T>();
			auto p = world->AddComponent<T>(entityID, std::forward<TArgs>(args)...);
			return p;
		}

		/* Add a T to the entity.
		Will throw exception if T already exists.
		*/
		template<typename T>
		T* Add(T&& tval) {
			ThrowIfSingletonTestFailed<T>();
			auto p = world->AddComponent<T>(entityID, std::forward<T>(tval));
			return p;
		}

		/* Remove a component form entity.
		Throw exception if entity doesn't have T.
		*/
		template<typename T>
		void Remove() {
			ThrowIfSingletonTestFailed<T>();
			int compIndex = world->ConvertComponentTypeToIndex<T>();
			world->RemoveComponent(entityID, compIndex);
		}
	private:
		template <typename T>
		void ThrowIfSingletonTestFailed() {
			if (std::is_base_of<ISingletonComponent, T>::value && entityID.index != 0) {
				throw std::runtime_error("Can't add singleton to a normal entity!");
			}
			if (!std::is_base_of<ISingletonComponent, T>::value && entityID.index == 0) {
				throw std::runtime_error("Can't add normal component to singleton entity!");
			}
		}
	};
}