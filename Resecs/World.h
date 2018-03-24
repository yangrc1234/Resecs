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

	class World {
	/* main interface. */
	public:
		friend Entity;
		World():
			m_componentActivationTable(1024),
			m_generation(1024),
			m_possibleAliveEntities(1024)
		{

		}
		Entity Create() {
			int loopCount = 0;
			while (m_alive[m_creationIndex])
			{
				if (loopCount++ > MAX_ENTITY_COUNT) {
					throw std::overflow_error("Max entity count reached!!!");
				}
				m_creationIndex = (m_creationIndex + 1) % MAX_ENTITY_COUNT;
			}
			EnlargeVectorToFit(m_generation, m_creationIndex);
			EnlargeVectorToFit(m_componentActivationTable, m_creationIndex);

			m_alive[m_creationIndex] = true;
			auto entityID = EntityID(m_creationIndex, m_generation[m_creationIndex]);
			m_possibleAliveEntities.push_back(entityID);
			m_aliveEntityCount++;
			m_componentActivationTable[entityID.index].reset();	//clean activation table.
			return Entity(this, entityID);
		}
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
			ActivationTable componentFilter = ConvertComponentTypesToMask<TSubComps>();

			for (auto id : m_possibleAliveEntities) {
				auto entityHandle = GetEntityHandle(id);
				if ((GetActivationTableFor(entityHandle.entityID) & componentFilter) == componentFilter)
					func(entityHandle, GetComponent<TSubComps>(id.index)...);
			}
		}
		/* Iterate all entities. */
		void Each(typename Identity<std::function<void(Entity)>>::type func) {
			/* Remove all dead entities first. */
			m_possibleAliveEntities.erase(std::remove_if(m_possibleAliveEntities.begin(), m_possibleAliveEntities.end(),
				[=](EntityID& id) {
				return !this->CheckEntityAlive(id);
			}
			), m_possibleAliveEntities.end());
			for (auto id : m_possibleAliveEntities) {
				auto entityHandle = GetEntityHandle(id);
				func(entityHandle);
			}
		}
		/* Current alive entities */
		int EntityCount() {
			return m_aliveEntityCount;
		}
	/*Entity ID management*/
	public:
		bool CheckEntityAlive(EntityID toCheck) {
			if (toCheck.index >= m_alive.size()) {
				return false;
			}
			if (m_alive[toCheck.index] && m_generation[toCheck.index] == toCheck.generation) {
				return true;
			}
			return false;
		}
		Entity GetEntityHandle(EntityID id) {
			return Entity(this, id);
		}
		const static int MAX_ENTITY_COUNT = 2 << 20;	//max entity count.
	private:
		void destroyEntity(EntityID id) {
			for (size_t i = 0; i < m_componentManagers.size(); i++)
			{
				if (HasComponent(id, i))
					RemoveComponent(id, i);
			}
			m_generation[id.index]++;
			m_alive[id.index] = false;
			m_aliveEntityCount--;
		}
		std::vector<int> m_generation;
		std::bitset<MAX_ENTITY_COUNT> m_alive;
		size_t m_creationIndex = 0;
		int m_aliveEntityCount = 0;
		std::vector<EntityID> m_possibleAliveEntities;
	
	/*Component management.*/
	public:
		const static int MAX_COMPONENT_COUNT = 2 << 8;
		using ActivationTable = std::bitset<MAX_COMPONENT_COUNT>;
		using ComponentEventDelegate = Signal<ComponentEventArgs>;
		ComponentEventDelegate OnComponentChanged;
		template<typename T>
		int ConvertComponentTypeToIndex() {
			getComponentManager<T>();	//make sure T is registered.
			return m_componentToIndex[typeid(T)];
		}
		template<typename... TComps>
		ActivationTable ConvertComponentTypesToMask() {
			ActivationTable result;
			convertComponentTypesToMaskInternal<TComps>(result);
			return result;
		}
	private:
		template<typename TComp,typename... TComps>
		void convertComponentTypesToMaskInternal(ActivationTable& bs) {
			convertComponentTypesToMaskInternal<TComp>(bs);
			convertComponentTypesToMaskInternal<TComps>(bs);
		}
		template<typename TComp>
		void convertComponentTypesToMaskInternal(ActivationTable& bs) {
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
			if (!getComponentActivationStatus(entity,compIndex)) {
				return nullptr;
			}
			auto cm = getComponentManager<T>();
			return cm->Get(entity.index);
		}
		void RemoveComponent(EntityID entity,int componentIndex) {
			if (!CheckEntityAlive(entity)) {
				throw std::runtime_error("This entity is already destroyed!");
			}
			if (!HasComponent(entity,componentIndex)) {
				throw std::runtime_error("This entity doesn't have this type of component!");
			}
			auto cm = getComponentManager(componentIndex);
			cm->Release(entity.index);
			getComponentActivationStatus(entity,componentIndex) = false;
			OnComponentChanged.Invoke(ComponentEventArgs(
				ComponentEventType::Removed,
				entity,
				componentIndex
			));
		}
		bool HasComponent(EntityID entity,int componentIndex) {
			if (!CheckEntityAlive(entity)) {
				throw std::runtime_error("This entity is already destroyed!");
			}
			return getComponentActivationStatus(entity,componentIndex);
		}
		ActivationTable& GetActivationTableFor(EntityID entity) {
			if (!CheckEntityAlive(entity)) {
				throw std::runtime_error("This entity is already destroyed!");
			}
			return m_componentActivationTable[entity.index];
		}
	private:
		std::unordered_map<std::type_index, int> m_componentToIndex;
		std::vector<std::unique_ptr<BaseComponentManager>> m_componentManagers;
		std::vector<ActivationTable> m_componentActivationTable;	//first dim is EntityID, second dim is componentID
		ActivationTable::reference getComponentActivationStatus(EntityID entity,int componentIndex) {
			EnlargeVectorToFit(m_componentActivationTable, entity.index);
			auto& vec = m_componentActivationTable[entity.index];
			return vec[componentIndex];
		}

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
		BaseComponentManager* getComponentManager(int componentIndex) {
			return static_cast<BaseComponentManager*>(m_componentManagers[componentIndex].get());
		}
};

	class Group {
	public:
		/* Iterator for group.
		Since group only contains EntityID, the iterator returns EntityHandle, which makes Group easier to use.
		*/
		struct GroupIterator {
		public:
			GroupIterator(World* world, std::unordered_set<EntityID>::const_iterator intIte) {
				this->internalIterator = intIte;
				this->world = world;
			}
			void operator++() {
				internalIterator++;
			}
			bool operator==(const GroupIterator& ano) const {
				return this->internalIterator == ano.internalIterator;
			}
			bool operator!=(const GroupIterator& ano) const {
				return this->internalIterator != ano.internalIterator;
			}

			Entity operator*() {
				return world->GetEntityHandle(*internalIterator);
			}
		private:
			World* world;
			std::unordered_set<EntityID>::const_iterator internalIterator;
		};
	private:
		World* world;
		std::unordered_set<EntityID> cachedEntities;
		World::ComponentEventDelegate::SignalConnection added;
		Group(World* world) :
			world(world),
			added(world->OnComponentChanged.Connect(std::bind(&Group::OnChanged, this, std::placeholders::_1))) {
			Initialize();
		}
		std::bitset<World::MAX_COMPONENT_COUNT> componentFilter;
	public:
		auto begin() {
			return GroupIterator(world, cachedEntities.cbegin());
		}
		auto end() {
			return GroupIterator(world, cachedEntities.cend());
		}
		/* Return the count of entities in the group. */
		auto Count() {
			return cachedEntities.size();
		}
		/* Return a copy of current entities inside the group.
		If you use range-for on Group, you can't destroy entities or RemoveComponent component, since it will edit the collection.
		Instead clone a vector then destroy entity in it.
		*/
		auto GetVectorClone() {
			std::vector<Entity> result;
			for (auto& t : cachedEntities) {
				result.push_back(world->GetEntityHandle(t));
			}
			return result;
		}
	private:
		void OnChanged(ComponentEventArgs arg) {
			if ((world->GetActivationTableFor(arg.entity) & componentFilter) == componentFilter) {
				cachedEntities.insert(arg.entity);
			}
		}
		void Initialize() {
			world->Each(
				[&](Entity entity) {
				if ((world->GetActivationTableFor(entity.entityID) & componentFilter) == componentFilter) {
					cachedEntities.insert(entity.entityID);
				}
			});
		}
	
	/* static methods for creating groups.*/
	public:
		/* Create a group. */
		template<typename... TComps>
		static Group CreateGroup(World* world) {
			auto group = Group();
			group.world = world;
			MarkBit<TComps>(group.componentFilter);
			group.Initialize();
			return group;
		}
	private:
		template<typename TComp,typename... TComps>
		static void MarkBit(Group& group) {
			MarkBit<TComp>(group);
			MarkBit<TComps>(group);
		}
		template<typename TComp>
		static void MarkBit(Group& group) {
			group.componentFilter.set(group.world->ConvertComponentTypeToIndex<TComp>());
		}
	};
}