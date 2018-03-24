#pragma once
#include "World.h"
#include "Entity.h"

namespace Resecs
{
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
		ComponentEventDelegate::SignalConnection added;
		Group(World* world);
		std::bitset<MAX_COMPONENT_COUNT> componentFilter;
	public:
		auto begin();
		auto end();
		/* Return the count of entities in the group. */
		auto Count();
		/* Return a copy of current entities inside the group.
		If you use range-for on Group, you can't destroy entities or RemoveComponent component, since it will edit the collection.
		Instead clone a vector then destroy entity in it.
		*/
		auto GetVectorClone();
	private:
		void OnChanged(ComponentEventArgs arg);
		void Initialize();

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
		template<typename TComp, typename... TComps>
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