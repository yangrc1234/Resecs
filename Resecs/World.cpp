#include "World.h"

Resecs::World::World() :
	m_componentActivationTable(1024),
	m_generation(1024),
	m_possibleAliveEntities(1024)
{

}

Resecs::Entity Resecs::World::Create() {
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

/* Iterate all entities. */
void Resecs::World::Each(typename Identity<std::function<void(Entity)>>::type func) {
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
int Resecs::World::EntityCount() {
	return m_aliveEntityCount;
}

bool Resecs::World::CheckEntityAlive(EntityID toCheck) {
	if (toCheck.index >= m_alive.size()) {
		return false;
	}
	if (m_alive[toCheck.index] && m_generation[toCheck.index] == toCheck.generation) {
		return true;
	}
	return false;
}

Resecs::Entity Resecs::World::GetEntityHandle(EntityID id) {
	return Entity(this, id);
}

void Resecs::World::destroyEntity(EntityID id) {
	for (size_t i = 0; i < m_componentManagers.size(); i++)
	{
		if (HasComponent(id, i))
			RemoveComponent(id, i);
	}
	m_generation[id.index]++;
	m_alive[id.index] = false;
	m_aliveEntityCount--;
}

void Resecs::World::RemoveComponent(EntityID entity, int componentIndex) {
	if (!CheckEntityAlive(entity)) {
		throw std::runtime_error("This entity is already destroyed!");
	}
	if (!HasComponent(entity, componentIndex)) {
		throw std::runtime_error("This entity doesn't have this type of component!");
	}
	auto cm = getComponentManager(componentIndex);
	cm->Release(entity.index);
	getComponentActivationStatus(entity, componentIndex) = false;
	OnComponentChanged.Invoke(ComponentEventArgs(
		ComponentEventType::Removed,
		entity,
		componentIndex
	));
}

bool Resecs::World::HasComponent(EntityID entity, int componentIndex) {
	if (!CheckEntityAlive(entity)) {
		throw std::runtime_error("This entity is already destroyed!");
	}
	return getComponentActivationStatus(entity, componentIndex);
}

Resecs::ComponentActivationBitset & Resecs::World::GetActivationTableFor(EntityID entity) {
	if (!CheckEntityAlive(entity)) {
		throw std::runtime_error("This entity is already destroyed!");
	}
	return m_componentActivationTable[entity.index];
}

//first dim is EntityID, second dim is componentID

Resecs::ComponentActivationBitset::reference Resecs::World::getComponentActivationStatus(EntityID entity, int componentIndex) {
	EnlargeVectorToFit(m_componentActivationTable, entity.index);
	auto& vec = m_componentActivationTable[entity.index];
	return vec[componentIndex];
}

BaseComponentManager * Resecs::World::getComponentManager(int componentIndex) {
	return static_cast<BaseComponentManager*>(m_componentManagers[componentIndex].get());
}
