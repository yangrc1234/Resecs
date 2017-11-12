#pragma once
#include <vector>
#include <type_traits>
#include <functional>
#include <list>
#include <unordered_set>

#include "Signal.hpp"
#include "Component.hpp"
#include "EntityID.hpp"

namespace Resecs {

	//***IF YOU SEE COMPILE ERRORS AROUND HERE***.
	//make sure you include all your components in your World template parameters.

	/* Index of Type T in Ts...
	 from https://stackoverflow.com/questions/26169198/how-to-get-the-index-of-a-type-in-a-variadic-type-pack.
	 does type - index conversion in compile time.( and print compile errors if type doesn't exists.)
	 really awesome.
	*/
	template <typename T, typename... Ts>
	struct Index;

	template <typename T, typename... Ts>
	struct Index<T, T, Ts...> : std::integral_constant<std::uint16_t, 0> {};

	template <typename T, typename U, typename... Ts>
	struct Index<T, U, Ts...> : std::integral_constant<std::uint16_t, 1 + Index<T, Ts...>::value> {};

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

	template <class... TComps>
	class World {
	private:
		/* Maximum entity count.
		The component pool will be fully-size allocated once World object constructed.
		TODO: make component pool resizable.
		*/
		const static uint32_t entityPoolSize = 2 << 16;

		using pComponent = Component*;
		/* all components stores here.
		The pComponent type actually doesn't do anything. Replace the pointer with (void*) will also work. Doing so makes it easier to understand.
		To actually get to a component, a static_cast<T*> is needed before using index.
		*/
		pComponent* components; 

		/* Store current entities. 
		It's not sure if all entities here are alive, but we can check that.
		*/
		std::list<EntityID> currentEntities;

	public:
		World() {
			components = new pComponent[sizeof...(TComps)];
			InitializeComponents<0, TComps...>();
			memset(generation, 0, sizeof(generation));
			memset(alive, 0, sizeof(alive));
			CreateEntity();	//create the singleton Entity.
		}

		~World() {
			FreeComponentPool<TComps...>();
			delete[] components;
		}

		/* Entity Handle
		Actually just a wrap-up for raw EntityID(besides a pointer to World), so you can do copy or anything you want.
		Get a Entity Handle from CreateEntity() or GetEntityHandle(EntityID)
		*/
		struct Entity {
		private:
			friend class World;
			Entity(World* world,EntityID entityID):world(world), entityID(entityID) {}
			World* world;  //reference to world.
			EntityID entityID;

		public:
			/* Return the entity's ID.
			An index cost less space than an Entity structure, 
			since it doesn't contains a pointer to the world.
			*/
			EntityID GetID() {
				return entityID;
			}

			/* Check if the Entity is alive */
			bool isAlive() {
				return world->CheckEntityAlive(this->entityID);
			}

			/* Set a component of the Entity. */
			template<class TComp, class... TArgs>
			TComp* Set(const TArgs&... args) {
				static_assert(!std::is_base_of<ISingletonComponent, TComp>::value, "Can't access singleton component from an entity handle");
				if (!isAlive()) {
					throw exception("Entity is already destroyed!");
				}
				return world->SetComponent<TComp, TArgs...>(this->entityID.index, args...);
			}

			/* Remove a component of the Entity. */
			template<class TComp>
			void Remove() {
				static_assert(!std::is_base_of<ISingletonComponent, TComp>::value, "Can't access singleton component from an entity handle");
				if (!isAlive()) {
					throw exception("Entity is already destroyed!");
				}
				if (!world->HasComponent<TComp>(this->entityID.index)) {
					throw exception("Entity doesn't have this component");
				}
				world->RemoveComponent<TComp>(this->entityID.index);
			}

			/* Check whether the entity contains a component. */
			template<class TComp>
			bool Has() {
				static_assert(!std::is_base_of<ISingletonComponent, TComp>::value, "Can't access singleton component from an entity handle");
				if (!isAlive()) {
					throw exception("Entity is already destroyed!");
				}
				return world->HasComponent<TComp>(this->entityID.index);
			}

			/* Get the pointer to the entity's component. */
			template<class TComp>
			TComp* Get() {
				static_assert(!std::is_base_of<ISingletonComponent, TComp>::value, "Can't access singleton component from an entity handle");
				if (!isAlive()) {
					throw exception("Entity is already destroyed!");
				}
				if (!world->HasComponent<TComp>(this->entityID.index)) {
					throw exception("Entity doesn't have this component");
				}
				return world->GetComponent<TComp>(this->entityID.index);
			}

			/* Destroy the entity.*/
			void Destroy() {
				if (!isAlive()) {
					throw exception("Entity is already destroyed!");
				}
				world->DestroyEntity(this->entityID);
			}

			/* Print the entity. only consider this for debug. */
			std::string ToString() {
				char buffer[500];
				sprintf_s(buffer, "Entity %d*%u :", this->entityID.generation, this->entityID.index);
				return buffer + compToString<TComps...>();
			}
		
		private:
			template <typename TComp>
			std::string compToString() {
				if (world->HasComponent<TComp>(this->entityID.index)) {
					char buffer[500];
					sprintf_s(buffer, "%d,", (int)Index<TComp, TComps...>::value);
					return buffer;
				}
				return "";
			}
			template <typename T, typename U, typename... Rest>
			std::string compToString() {
				return compToString<T>() + compToString<U, Rest...>();
			}
		};

		using ComponentEventDelegate = Signal<EntityID, uint16_t>;

		/* Component add event. */
		ComponentEventDelegate OnComponentAddedListeners;
		/* Component remove event. */
		ComponentEventDelegate OnComponentRemovedListeners;

		using EntityEventDelegate = Signal<EntityID>;
		
		/* Entity create event. */
		EntityEventDelegate OnEntityCreated;
		/* Entity destroy event */
		EntityEventDelegate OnEntityDestroyed;

		/* Group for entity filtering.
		e.g. Group<TA,TB> only contains entities with both TA and TB.
		it does entity filtering by listening to world's component added/removed event.
		*/
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
			ComponentEventDelegate::SignalConnection added;
			ComponentEventDelegate::SignalConnection removed;

		public:
			Group(World* world) :
				world(world),
				added(world->OnComponentAddedListeners.Connect(std::bind(&Group::Added, this, placeholders::_1, placeholders::_2))),
				removed(world->OnComponentRemovedListeners.Connect(std::bind(&Group::Removed, this, placeholders::_1, placeholders::_2))) {
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
			If you use range-for on Group, you can't destroy entities or remove component, since it will edit the collection. 
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
			void Added(EntityID entityIndex, EntityIndex_t compID) {
				if (cachedEntities.find(entityIndex) != cachedEntities.end())
					return;
				if (AllTrue(world->HasComponent<TGroupComps>(entityIndex.index)...)) {
					cachedEntities.insert(entityIndex);
				}
			}

			void Removed(EntityID entityIndex, EntityIndex_t compID) {
				if (cachedEntities.find(entityIndex) == cachedEntities.end())
					return;
				if (!AllTrue(world->HasComponent<TGroupComps>(entityIndex.index)...)) {
					cachedEntities.erase(entityIndex);
				}
			}

			void Initialize() {
				world->Each<TGroupComps...>(
					[=](Entity entity, TGroupComps*... args) {
					cachedEntities.insert(entity.entityID);
				});
			}
		};
		
	private:
		template <typename T>
		void FreeComponentPool() {
			delete[] static_cast<T*>(components[Index<T, TComps...>::value]);
		}

		template <typename T, typename U, typename... Rest>
		void FreeComponentPool() {
			FreeComponentPool<T>();
			FreeComponentPool<U, Rest...>();
		}

		/* Generation of current entities
		If the generation of an EntityID is not the same as the one here, it means the Entity is already destroyed.(and the one in the pool is a newly created one, if it exists.)
		Every time an entity is destroyed, its corresponding slot in generation will be plused by 1.
		So when the entity handle checks alive, it returns false, cuz the generation in World is not the same with the one in itself.
		*/
		int generation[entityPoolSize];
		/* Whether the entity is active. */
		bool alive[entityPoolSize];

	public:
		/* Check if the entity is alive. */
		bool CheckEntityAlive(EntityID toCheck) {
			if (alive[toCheck.index] && generation[toCheck.index] == toCheck.generation) {
				return true;
			}
			return false;
		}

		/* Get the handle of an Entity using its memory index
		An exception would be thrown if the entity is not alive.
		*/
		Entity GetEntityHandle(EntityIndex_t id) {
			if (alive[id]) {
				return GetEntityHandle(EntityID(id, generation[id]));
			}
			throw exception("The entity is destroyed! Use CheckEntityAlive() before GetEntityHandle()!");
		}

		/* Get the handle of an Entity using EntityID
		An exception would be thrown if the entity is not alive.
		*/
		Entity GetEntityHandle(EntityID id) {
			if (alive[id.index] && generation[id.index] == id.generation) {
				return Entity(this,id);
			}
			throw exception("The entity is destroyed! Use CheckEntityAlive() before GetEntityHandle()!");
		}

		/* Create an entity.
		Return its handle.
		*/
		Entity CreateEntity() {
			for (int i = 0; i < entityPoolSize; i++) {
				if (!alive[i]) {
					alive[i] = true;
					auto id = EntityID(i, generation[i]);
					OnEntityCreated.Invoke(id);
					currentEntities.push_back(id);
					return GetEntityHandle(id);
				}
			}
			throw exception("Entity pool overflow!");
		}

		/*
		this weird code makes lambda could be deduced to std::function.
		from https://stackoverflow.com/questions/13358672/how-to-convert-a-lambda-to-an-stdfunction-using-templates
		*/
		template <typename T>
		struct Identity {
			typedef T type;
		};
		
		/* Iterate all entities, and execute func on those containing TSubComps
		You should not create new entity during this process(cause the collection to change).
		If you want to create entity during iteration, use Group.GetVectorClone() to do so.
		*/
		template<typename... TSubComps>
		void Each(typename Identity<std::function<void(Entity, TSubComps*...)>>::type func) {
			/* Remove all dead entities first. */
			currentEntities.erase(std::remove_if(currentEntities.begin(), currentEntities.end(),
				[=](EntityID& id) {
				return !this->CheckEntityAlive(id);
			}
			), currentEntities.end());
			for (auto id : currentEntities) {
				auto entityHandle = GetEntityHandle(id);
				if (AllTrue(HasComponent<TSubComps>(id.index)...))
					func(entityHandle, GetComponent<TSubComps>(id.index)...);
			}
		}

		/* Set singleton component
		*/
		template<class T, class... Args>
		T* Set(const Args& ...args) {
			return SetComponent<T>(0, args...);
		}

		/* Remove a singleton component */
		template<class T>
		void Remove() {
			RemoveComponent<T>(0);
		}

		/* Get a singleton component */
		template<class T>
		T* Get() noexcept{
			return GetComponent<T>(0);
		}

		/* Check if a singleton component exists */
		template<class T>
		bool Has() noexcept {
			return HasComponent<T>(0);
		}

		/* Print the world. for debug.*/
		void Print() {
			for (int i = 0; i < entityPoolSize; i++) {
				if (alive[i]) {
					cout << "Entity " << i << ": " << endl;
					PrintEntity<0, TComps...>(i);
				}
			}
		}

	private:
		void DestroyEntity(EntityID id) {
			if (!CheckEntityAlive(id)) {
				throw exception("The entity is already destroyed!");
			}
			if (id.index == 0) {
				throw exception("Destroying singleton entity is not allowed!");
			}
			OnEntityDestroyed.Invoke(id);
			DestroyEntity<TComps...>(id.index);
			alive[id.index] = false;
			//we don't remove index from currentEntities.
			//when iterating, dead entities will be removed.
			generation[id.index] ++;
		}

		template <typename T>
		void DestroyEntity(EntityIndex_t id) {
			if (std::is_base_of<ISingletonComponent, T>::value) {
				return;
			}
			RemoveComponent<T>(id);
		}

		template <typename T, typename U, typename... Rest>
		void DestroyEntity(EntityIndex_t id) {
			DestroyEntity<T>(id);
			DestroyEntity<U, Rest...>(id);
		}

		template<class T, class... Args>
		T* SetComponent(EntityIndex_t entityID, const Args& ...args) {
			auto index = Index<T, TComps...>::value;
			T* ptr = static_cast<T*>(components[index]);
			ptr[entityID] = T(args...);
			ptr[entityID].actived = true;
			OnComponentAddedListeners.Invoke(EntityID(entityID, generation[entityID]), index);
			return ptr + entityID;
		}

		template<class T>
		void RemoveComponent(EntityIndex_t entityID){
			auto index = Index<T, TComps...>::value;
			if (entityID > entityPoolSize)
				return;
			T* ptr = static_cast<T*>(components[index]);
			if (!ptr[entityID].actived)
				return;
			ptr[entityID].actived = false;
			OnComponentRemovedListeners.Invoke(EntityID(entityID, generation[entityID]), index);
		}

		template<class T>
		T* GetComponent(EntityIndex_t entityID) noexcept {
			auto index = Index<T, TComps...>::value;
			T* ptr = static_cast<T*>(components[index]);
			return ptr + entityID;
		}

		template<class T>
		bool HasComponent(EntityIndex_t entityID) noexcept {
			auto index = Index<T, TComps...>::value;
			T* ptr = static_cast<T*>(components[index]);
			return ptr[entityID].actived;
		}

		template<int TCompIndex, class T>
		void PrintEntity(EntityIndex_t entity) {
			T* ptr = static_cast<T*>(components[TCompIndex]);
			if (ptr[entity].actived) {
				cout << "Component " << TCompIndex << " Actived" << endl;
			}
		}

		template<int TCompIndex, class T, class U, class... TRest>
		void PrintEntity(EntityIndex_t entity) {
			PrintEntity<TCompIndex, T>(entity);
			PrintEntity<TCompIndex + 1, U, TRest...>(entity);
		}

		template<int index, class T>
		void InitializeComponents() {
			if (std::is_base_of<ISingletonComponent, T>::value) {
				components[index] = static_cast<Component*>(new T[1]);
			}
			else {
				components[index] = static_cast<Component*>(new T[entityPoolSize]);
			}
		}

		template<int index, class T, class V, class... U>
		void InitializeComponents() {
			InitializeComponents<index, T>();
			InitializeComponents<index + 1, V, U...>();
		}
	};
}