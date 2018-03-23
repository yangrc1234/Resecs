#include <Resecs\Resecs.h>
#include <gtest\gtest.h>
#include "Resecs\Resecs.h"

using namespace Resecs;

class Vector3 {
public:
	float x;
	float y;
	float z;
	bool operator==(const Vector3& ano) const {
		return x == ano.x && y == ano.y && z == ano.z;
	}
};

class PositionComponent {
public:
	PositionComponent()
	{
			
	}
	PositionComponent(float x,float y,float z)
	{
		val.x = x;
		val.y = y;
		val.z = z;
	}
	Vector3 val;
};

class VelocityComponent {
public:
	VelocityComponent()
	{

	}
	VelocityComponent(float x, float y, float z)
	{
		val.x = x;
		val.y = y;
		val.z = z;
	}
	Vector3 val;
};

class FlagComponent{};

TEST(WorldTest, EntityCreationTest) {
	
	World testWorld;
	
	ASSERT_TRUE(testWorld.EntityCount() == 0);
	ASSERT_FALSE(testWorld.CheckEntityAlive(EntityID(0, 0)));
	auto entity = testWorld.Create();
	ASSERT_TRUE(testWorld.CheckEntityAlive(entity.entityID));
	ASSERT_TRUE(testWorld.EntityCount() == 1);
	auto entity2 = testWorld.Create();
	ASSERT_TRUE(testWorld.EntityCount() == 2);
	entity.Destroy();
	entity2.Destroy();
	for (size_t i = 0; i < World::MAX_ENTITY_COUNT; i++)
	{
		auto tempEntity = testWorld.Create();
		tempEntity.Add(PositionComponent(0, 0, 0));
	}
	ASSERT_TRUE(testWorld.EntityCount() == World::MAX_ENTITY_COUNT);

	ASSERT_ANY_THROW(
		auto tempEntity = testWorld.Create();	//should cause a overflow.
	);
}

TEST(WorldTest, AddComponentTest) {
	World testWorld;
	auto entity = testWorld.Create();
	ASSERT_FALSE(entity.Has<PositionComponent>());
	entity.Add(PositionComponent(0,0,1));
	ASSERT_TRUE(entity.Has<PositionComponent>());
	ASSERT_TRUE(entity.Get<PositionComponent>()->val == PositionComponent(0, 0, 1).val);

	for (size_t i = 0; i < 5; i++)
	{
		auto ent = testWorld.Create();
		ent.Add(PositionComponent(i,0,0));
		ent.Add(VelocityComponent(0, 1, 0));
		if (i != 3) {
			ent.Add(FlagComponent());
		}
	}
}

TEST(WorldTest, EntityDestroyTest) {
	World world;
	auto entity = world.Create();
	entity.Add(PositionComponent(0, 0, 1));
	ASSERT_TRUE(entity.IsAlive());
	entity.Destroy();
	ASSERT_TRUE(world.EntityCount() == 0);
	ASSERT_FALSE(entity.IsAlive());
	ASSERT_ANY_THROW(
		entity.Get<PositionComponent>();
		);

	ASSERT_ANY_THROW(
		entity.Has<PositionComponent>();
	);
}

