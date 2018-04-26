#include "Group.h"
using namespace Resecs;

Resecs::Group::GroupIterator::GroupIterator(World * world, std::unordered_set<EntityID>::const_iterator intIte) {
	this->internalIterator = intIte;
	this->world = world;
}

Resecs::Group::Group(World* world, ComponentActivationBitset componentFilter) :
	world(world),
	componentChangedSignalConnection(world->OnComponentChanged.Connect(std::bind(&Group::OnChanged, this, std::placeholders::_1))),
	componentFilter(componentFilter){
	Initialize();
}

Resecs::Group::Group(const Group & copy) :
	world(copy.world),
	componentChangedSignalConnection(copy.world->OnComponentChanged.Connect(std::bind(&Group::OnChanged, this, std::placeholders::_1))),
	componentFilter(copy.componentFilter)
{
	Initialize();
}

Group::GroupIterator Resecs::Group::begin() {
	return GroupIterator(world, cachedEntities.cbegin());
}

Group::GroupIterator Resecs::Group::end() {
	return GroupIterator(world, cachedEntities.cend());
}

size_t Resecs::Group::Count() {
	return cachedEntities.size();
}

/* Return a copy of current entities inside the group.
If you use range-for on Group, you can't destroy entities or RemoveComponent component, since it will edit the collection.
Instead clone a vector then destroy entity in it.
*/

std::vector<Entity> Resecs::Group::GetVectorClone() {
	std::vector<Entity> result;
	for (auto& t : cachedEntities) {
		result.push_back(world->GetEntityHandle(t));
	}
	return result;
}

void Resecs::Group::OnChanged(ComponentEventArgs arg) {
	auto find = cachedEntities.find(arg.entity);
	if (arg.type == ComponentEventType::Added) {
		if (find == cachedEntities.end()) {
			if ((world->GetActivationTableFor(arg.entity) & componentFilter) == componentFilter) {
				cachedEntities.insert(arg.entity);
				onGroupChanged.Invoke(Resecs::GroupEntityEventArgs(world->GetEntityHandle(arg.entity), GroupEntityEventArgs::Type::Added));
			}
		}
	}
	else
	{
		if (find != cachedEntities.end()) {
			if ((world->GetActivationTableFor(arg.entity) & componentFilter) != componentFilter) {
				cachedEntities.erase(find);
				onGroupChanged.Invoke(Resecs::GroupEntityEventArgs(world->GetEntityHandle(arg.entity), GroupEntityEventArgs::Type::Removed));
			}
		}
	}
}

void Resecs::Group::Initialize() {
	cachedEntities.clear();
	world->Each(
		[&](Entity entity) {
		if ((world->GetActivationTableFor(entity.entityID) & componentFilter) == componentFilter) {
			cachedEntities.insert(entity.entityID);
		}
	});
}
