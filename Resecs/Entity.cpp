#include "Entity.h"
#include "World.h"

using namespace Resecs;

Entity::Entity(World * world, EntityID entityID) :world(world), entityID(entityID) {}

bool Entity::IsAlive() {
	return world->CheckEntityAlive(entityID);
}

void Entity::Destroy() {
	world->destroyEntity(entityID);
}
