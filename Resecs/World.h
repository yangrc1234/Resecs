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

	class World {
	/* main interface. */
	public:
		friend Entity;
		Entity Create() {
			int startIndex = m_creationIndex;
			int loopCount = 0;
			while (m_alive[m_creationIndex])
			{
				if (loopCount++ > MAX_ENTITY_COUNT) {
					throw std::overflow_error("Max entity count reached!!!");
				}
				m_creationIndex = (m_creationIndex + 1) % MAX_ENTITY_COUNT;
				resizeEntityPoolIfNecessary(m_creationIndex + 1);
			}
			resizeEntityPoolIfNecessary(m_creationIndex + 1);

			m_alive[m_creationIndex] = true;
			auto entityID = EntityID(m_creationIndex, m_generation[m_creationIndex]);
			m_possibleAliveEntities.push_back(entityID);
			m_aliveEntityCount++;
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
			for (auto id : m_possibleAliveEntities) {
				auto entityHandle = GetEntityHandle(id);
				if (AllTrue(HasComponent<TSubComps>(id.index)...))
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
			m_generation[id.index]++;
			m_alive[id.index] = false;
			m_aliveEntityCount--;
		}
		std::vector<int> m_generation;
		std::bitset<MAX_ENTITY_COUNT> m_alive;
		size_t m_creationIndex = 0;
		void resizeEntityPoolIfNecessary(int newSize) {
			if (m_generation.size() < newSize) {
				m_generation.resize(newSize * 1.5f);
			}
		}
		int m_aliveEntityCount = 0;
		std::vector<EntityID> m_possibleAliveEntities;
	
	/*Component management.*/
	public:
		enum class ComponentEventType
		{
			Added,
			Removed,
		};
		struct ComponentEventArgs
		{
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
		ComponentEventDelegate OnComponentChanged;
		template<typename T>
		int ConvertComponentTypeToIndex() {
			return m_componentToIndex[typeid(T)];
		}
		template<typename T>
		T* AddComponent(EntityID entity) {
			if (!CheckEntityAlive(entity)) {
				throw std::runtime_error("This entity is already destroyed!");
			}
			if (HasComponent<T>(entity)) {
				throw std::runtime_error("This entity already has this component!");
			}
			auto cm = getComponentManager<T>();
			cm->Create(entity.index);
			getComponentActivationStatus<T>(entity) = true;
			OnComponentChanged.Invoke(ComponentEventArgs(
				ComponentEventType::Added,
				entity,
				m_componentToIndex[typeid(T)]
			));
			return cm->Get(entity.index);
		}
		template<typename T>
		T* GetComponent(EntityID entity) {
			if (!CheckEntityAlive(entity)) {
				throw std::runtime_error("This entity is already destroyed!");
			}
			if (!getComponentActivationStatus(entity)) {
				return nullptr;
			}
			auto cm = getComponentManager<T>();
			return cm->Get(entity.index);
		}
		template<typename T>
		void RemoveComponent(EntityID entity) {
			if (!CheckEntityAlive(entity)) {
				throw std::runtime_error("This entity is already destroyed!");
			}
			if (!HasComponent<T>(entity)) {
				throw std::runtime_error("This entity doesn't have this type of component!");
			}
			auto cm = getComponentManager<T>();
			cm->Release(entity.index);
			getComponentActivationStatus<T>(entity) = false;
			OnComponentChanged.Invoke(ComponentEventArgs(
				ComponentEventType::Removed,
				entity,
				m_componentToIndex[typeid(T)]
			));
		}
		template<typename T>
		bool HasComponent(EntityID entity) {
			if (!CheckEntityAlive(entity)) {
				throw std::runtime_error("This entity is already destroyed!");
			}
			if (m_componentToIndex.find(typeid(T)) == m_componentToIndex.end())
				return false;
			return getComponentActivationStatus<T>(entity);
		}
	private:
		std::unordered_map<std::type_index, std::unique_ptr<BaseComponentManager>> m_componentManagers;
		std::unordered_map<std::type_index, int> m_componentToIndex;
		int m_maxComponentIndex = 0;
		std::vector<std::vector<char>> m_componentActivationTable;	//first dim is EntityID, second dim is componentID
		template<typename T>
		char& getComponentActivationStatus(EntityID entity) {
			if (entity.index >= m_componentActivationTable.size())
			{
				m_componentActivationTable.resize((entity.index + 1) * 1.5f);
			}
			auto& vec = m_componentActivationTable[entity.index];
			auto componentIndex = m_componentToIndex[typeid(T)];
			if (componentIndex >= vec.size())
				vec.resize((componentIndex + 1)* 1.5f);
			return vec[componentIndex];
		}
		template<typename T>
		ComponentManager<T>* getComponentManager() {
			auto cmIte = m_componentManagers.find(typeid(T));
			if (cmIte == this->m_componentManagers.end()) {
				//Create cm.
				this->m_componentManagers[typeid(T)] = std::make_unique<ComponentManager<T>>();
				//AddComponent type->int map.
				m_componentToIndex[typeid(T)] = m_maxComponentIndex++;
				cmIte = this->m_componentManagers.find(typeid(T));
			}
			return static_cast<ComponentManager<T>*>(cmIte->second.get());
		}
	};

	template <typename... TGroupComps>
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
		World::ComponentEventDelegate::SignalConnection removed;
	public:
		Group(World* world) :
			world(world),
			added(world->OnComponentChanged.Connect(std::bind(&Group::OnChanged, this, std::placeholders::_1))) {
			Initialize();
		}
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
		void OnChanged(World::ComponentEventArgs arg) {
			if (cachedEntities.find(arg.entity) != cachedEntities.end())
				return;
			if (AllTrue(world->HasComponent<TGroupComps>(entityIndex)...)) {
				cachedEntities.insert(entityIndex);
			}
		}
		void Initialize() {
			world->Each<TGroupComps...>(
				[=](Entity entity, TGroupComps*... args) {
				cachedEntities.insert(entity.entityID);
			});
		}
	};
}