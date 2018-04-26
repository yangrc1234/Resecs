#pragma once
#include "World.h"
#include "Entity.h"

namespace Resecs
{
	struct GroupEntityEventArgs
	{
		Entity entity;
		enum class Type
		{
			Added,
			Removed
		};
		Type type;
		GroupEntityEventArgs(Entity id, Type type) : entity(id),type(type)
		{

		}
		GroupEntityEventArgs() {}
	};

	using GroupEntityEventDelegate = Signal<GroupEntityEventArgs>;

	class Group {
	public:
		/* Iterator for group.
		Since group only contains EntityID, the iterator returns EntityHandle, which makes Group easier to use.
		*/
		struct GroupIterator {
		public:
			GroupIterator(World* world, std::unordered_set<EntityID>::const_iterator intIte);
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
		ComponentEventDelegate::SignalConnection componentChangedSignalConnection;
		Group(World* world, ComponentActivationBitset componentFilter);
		ComponentActivationBitset componentFilter;
	public:
		Group(const Group& copy);
		Group::GroupIterator begin();
		Group::GroupIterator end();
		/* Return the count of entities in the group. */
		size_t Count();
		/* Return a copy of current entities inside the group.
		If you use range-for on Group, you can't destroy entities or RemoveComponent component, since it will edit the collection.
		Instead clone a vector then destroy entity in it.
		*/
		std::vector<Entity> GetVectorClone();
		/* Called when an entity enter or leave the group. 
		*/
		GroupEntityEventDelegate onGroupChanged;
	private:
		void OnChanged(ComponentEventArgs arg);
		void Initialize();

		/* static methods for creating groups.*/
	public:
		/* Create a group. */
		template<typename... TComps>
		static Group CreateGroup(World* world) {
			ComponentActivationBitset bs;
			MarkBit<TComps...>(bs,world);
			auto group = Group(world, bs);
			return group;
		}
	private:
		template<typename T>
		static void MarkBit(ComponentActivationBitset& bs, World* world) {
			bs.set(world->ConvertComponentTypeToIndex<T>());
		}
		template<typename T,typename U, typename... Rest>
		static void MarkBit(ComponentActivationBitset& bs, World* world) {
			MarkBit<T>(bs, world);
			MarkBit<U,Rest...>(bs, world);
		}
};
}