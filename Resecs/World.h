#pragma once
#include <vector>
#include <type_traits>
#include <functional>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <typeindex>
#include <exception>
#include <bitset>

#include "Utils\Signal.hpp"
#include "Utils\Common.hpp"
#include "Component.hpp"
#include "EntityID.hpp"
#include "ComponentManager.h"
#include "Entity.h"
namespace Resecs {

	/*Used for component filter. ( e.g. AllTrue(Has<TComp>()...) )*/
	template <typename T>
	bool AllTrue(T val) {
		return val;
	}
	template <typename T, typename... U>
	bool AllTrue(T a, U... vals) {
		if (a)
			return AllTrue(vals...);
		return false;
	}
	const static int MAX_COMPONENT_COUNT = 2 << 8;

	using ComponentActivationBitset = std::bitset<MAX_COMPONENT_COUNT>;

	enum class ComponentEventType
	{
		Added,
		Removed,
	};

	struct ComponentEventArgs
	{
		ComponentEventArgs() = default;
		ComponentEventArgs(
			ComponentEventType type,
			EntityID entity,
			uint16_t componentTypeIndex)
		{
			this->type = type;
			this->entity = entity;
			this->componentTypeIndex = componentTypeIndex;
		}
		ComponentEventType type;
		EntityID entity;
		uint16_t componentTypeIndex;
	};

	using ComponentEventDelegate = Signal<ComponentEventArgs>;

	class World {
	/* main interface. */
	public:
		friend Entity;
		World();
		Entity Create();
		template <typename T>
		struct Identity {
			typedef T type;
		};
		/* Iterate all entities that has TSubComps, then do func() */
		template<typename... TSubComps>
		void Each(typename Identity<std::function<void(Entity, TSubComps*...)>>::type func) {
			/* Remove all dead entities first. */
			m_possibleAliveEntities.erase(std::remove_if(m_possibleAliveEntities.begin(), m_possibleAliveEntities.end(),
				[=](EntityID& id) {
				return !this->CheckEntityAlive(id);
			}
			), m_possibleAliveEntities.end());
			ComponentActivationBitset componentFilter = ConvertComponentTypesToMask<TSubComps>();

			for (auto id : m_possibleAliveEntities) {
				auto entityHandle = GetEntityHandle(id);
				if ((GetActivationTableFor(entityHandle.entityID) & componentFilter) == componentFilter)
					func(entityHandle, GetComponent<TSubComps>(id.index)...);
			}
		}
		/* Iterate all entities. */
		void Each(typename Identity<std::function<void(Entity)>>::type func);
		/* Current alive entities */
		int EntityCount();
	/*Entity ID management*/
	public:
		bool CheckEntityAlive(EntityID toCheck);
		Entity GetEntityHandle(EntityID id);
		const static int MAX_ENTITY_COUNT = 2 << 20;	//max entity count.
	private:
		void destroyEntity(EntityID id);
		std::vector<int> m_generation;
		std::bitset<MAX_ENTITY_COUNT> m_alive;
		size_t m_creationIndex = 0;
		int m_aliveEntityCount = 0;
		std::vector<EntityID> m_possibleAliveEntities;
	
	/*Component management.*/
	public:
		ComponentEventDelegate OnComponentChanged;
		template<typename T>
		int ConvertComponentTypeToIndex() {
			getComponentManager<T>();	//make sure T is registered.
			return m_componentToIndex[typeid(T)];
		}
		template<typename... TComps>
		ComponentActivationBitset ConvertComponentTypesToMask() {
			ComponentActivationBitset result;
			convertComponentTypesToMaskInternal<TComps>(result);
			return result;
		}
	private:
		template<typename TComp,typename... TComps>
		void convertComponentTypesToMaskInternal(ComponentActivationBitset& bs) {
			convertComponentTypesToMaskInternal<TComp>(bs);
			convertComponentTypesToMaskInternal<TComps>(bs);
		}
		template<typename TComp>
		void convertComponentTypesToMaskInternal(ComponentActivationBitset& bs) {
			bs.set(ConvertComponentTypeToIndex<TComp>());
		}
	public:
		template<typename T>
		T* AddComponent(EntityID entity) {
			int compIndex = ConvertComponentTypeToIndex<T>();
			if (!CheckEntityAlive(entity)) {
				throw std::runtime_error("This entity is already destroyed!");
			}
			if (HasComponent(entity, compIndex)) {
				throw std::runtime_error("This entity already has this component!");
			}
			auto cm = getComponentManager<T>();
			cm->Create(entity.index);
			getComponentActivationStatus(entity, compIndex) = true;
			OnComponentChanged.Invoke(ComponentEventArgs(
				ComponentEventType::Added,
				entity,
				compIndex
			));
			return cm->Get(entity.index);
		}
		template<typename T>
		T* GetComponent(EntityID entity) {
			int compIndex = ConvertComponentTypeToIndex<T>();
			if (!CheckEntityAlive(entity)) {
				throw std::runtime_error("This entity is already destroyed!");
			}
			if (!getComponentActivationStatus(entity, compIndex)) {
				return nullptr;
			}
			auto cm = getComponentManager<T>();
			return cm->Get(entity.index);
		}
		void RemoveComponent(EntityID entity, int componentIndex);
		bool HasComponent(EntityID entity, int componentIndex);
		ComponentActivationBitset& GetActivationTableFor(EntityID entity);
	private:
		std::unordered_map<std::type_index, int> m_componentToIndex;
		std::vector<std::unique_ptr<BaseComponentManager>> m_componentManagers;
		std::vector<ComponentActivationBitset> m_componentActivationTable;	//first dim is EntityID, second dim is componentID
		ComponentActivationBitset::reference getComponentActivationStatus(EntityID entity, int componentIndex);

		int m_maxComponentTypeCount = 0;	//used to assign unique index to every new component type.
		template<typename T>
		ComponentManager<T>* getComponentManager() {
			auto compIndexIte = m_componentToIndex.find(typeid(T));
			int compIndex;
			if (compIndexIte == m_componentToIndex.end()) {
				//Create cm.
				this->m_componentManagers.emplace_back(std::make_unique<ComponentManager<T>>());
				//AddComponent type->int map.
				compIndex = m_maxComponentTypeCount;
				m_componentToIndex[typeid(T)] = m_maxComponentTypeCount++;
			}
			else
			{
				compIndex = compIndexIte->second;
			}
			return static_cast<ComponentManager<T>*>(m_componentManagers[compIndex].get());
		}
		BaseComponentManager* getComponentManager(int componentIndex);
	};
}