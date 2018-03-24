#include "Group.h"

Resecs::Group::GroupIterator::GroupIterator(World * world, std::unordered_set<EntityID>::const_iterator intIte) {
	this->internalIterator = intIte;
	this->world = world;
}

Resecs::Group::Group(World * world) :
	world(world),
	added(world->OnComponentChanged.Connect(std::bind(&Group::OnChanged, this, std::placeholders::_1))) {
	Initialize();
}

auto Resecs::Group::begin() {
	return GroupIterator(world, cachedEntities.cbegin());
}

auto Resecs::Group::end() {
	return GroupIterator(world, cachedEntities.cend());
}

/* Return the count of entities in the group. */

auto Resecs::Group::Count() {
	return cachedEntities.size();
}

/* Return a copy of current entities inside the group.
If you use range-for on Group, you can't destroy entities or RemoveComponent component, since it will edit the collection.
Instead clone a vector then destroy entity in it.
*/

auto Resecs::Group::GetVectorClone() {
	std::vector<Entity> result;
	for (auto& t : cachedEntities) {
		result.push_back(world->GetEntityHandle(t));
	}
	return result;
}

void Resecs::Group::OnChanged(ComponentEventArgs arg) {
	if ((world->GetActivationTableFor(arg.entity) & componentFilter) == componentFilter) {
		cachedEntities.insert(arg.entity);
	}
}

void Resecs::Group::Initialize() {
	world->Each(
		[&](Entity entity) {
		if ((world->GetActivationTableFor(entity.entityID) & componentFilter) == componentFilter) {
			cachedEntities.insert(entity.entityID);
		}
	});
}
