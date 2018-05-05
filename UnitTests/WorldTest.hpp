#pragma once
#include <gtest\gtest.h>
#include "Common.hpp"

using namespace Resecs;

TEST(WorldTest, EntityCreationTest) {
	World testWorld;

	ASSERT_TRUE(testWorld.EntityCount() == 1);	//should be a singleton there.
	ASSERT_FALSE(testWorld.CheckEntityAlive(EntityID(1, 0)));
	auto entity = testWorld.Create();
	ASSERT_TRUE(testWorld.CheckEntityAlive(entity.entityID));
	ASSERT_TRUE(testWorld.EntityCount() == 2);
	auto entity2 = testWorld.Create();
	ASSERT_TRUE(testWorld.EntityCount() == 3);
	entity.Destroy();
	entity2.Destroy();
	for (size_t i = 0; i < World::MAX_ENTITY_COUNT - 1; i++)
	{
		auto tempEntity = testWorld.Create();
	}
	ASSERT_TRUE(testWorld.EntityCount() == World::MAX_ENTITY_COUNT);

	ASSERT_ANY_THROW(
		auto tempEntity = testWorld.Create();	//should cause a overflow.
	);
}

TEST(WorldTest, EntityDestroyTest) {
	World world;
	auto entity = world.Create();
	entity.Add(PositionComponent(0, 0, 1));
	ASSERT_TRUE(entity.IsAlive());
	entity.Destroy();
	ASSERT_TRUE(world.EntityCount() == 1);
	ASSERT_FALSE(entity.IsAlive());
	ASSERT_ANY_THROW(
		entity.Get<PositionComponent>();
	);
	ASSERT_ANY_THROW(
		entity.Has<PositionComponent>();
	);
}

TEST(WorldTest, ComponentEventTest) {
	World testWorld;
	ComponentEventArgs testArg;
	auto signal = testWorld.OnComponentChanged.Connect(
		[&](ComponentEventArgs arg) {
		testArg = arg;
	});

	auto entity = testWorld.Create();

	entity.Add(PositionComponent(0, 0, 1));
	entity.Add(VelocityComponent(0, 0, 0));
	ASSERT_TRUE(
		testArg.type == ComponentEventType::Added
	);
	ASSERT_TRUE(
		testArg.componentTypeIndex == testWorld.ConvertComponentTypeToIndex<VelocityComponent>()
	);
	ASSERT_TRUE(
		testArg.entity == entity.entityID
	);

	auto entity2 = testWorld.Create();
	entity2.Add(VelocityComponent(0, 0, 0));
	ASSERT_TRUE(
		testArg.type == ComponentEventType::Added
	);
	ASSERT_TRUE(
		testArg.componentTypeIndex == testWorld.ConvertComponentTypeToIndex<VelocityComponent>()
	);
	ASSERT_TRUE(
		testArg.entity == entity2.entityID
	);

	entity.Remove<PositionComponent>();
	ASSERT_TRUE(testArg.type == ComponentEventType::Removed);
	ASSERT_TRUE(
		testArg.componentTypeIndex == testWorld.ConvertComponentTypeToIndex<PositionComponent>()
	);
	ASSERT_TRUE(
		testArg.entity == entity.entityID
	);
}

TEST(WorldTest, SingletonTest) {
	World testWorld;
	auto entity = testWorld.Create();
	ASSERT_TRUE(entity.entityID.index == 1);	//index 0 should be occupied by singleton.

	auto temp = testWorld.Add(SgComponent(1));
	ASSERT_ANY_THROW(
		testWorld.Add(SgComponent(1));
	);

	ASSERT_TRUE(testWorld.Get<SgComponent>() == temp);

	ASSERT_NO_THROW(
		testWorld.Remove<SgComponent>();
		testWorld.Add<SgComponent>(1);
		testWorld.Replace(SgComponent(1));
	);

	testWorld.Remove<SgComponent>();
	ASSERT_TRUE(!testWorld.Has<SgComponent>());

	//Prevent adding singletons to normal entity.
	ASSERT_ANY_THROW(
		entity.Add<SgComponent>();
	);
	ASSERT_ANY_THROW(
		entity.Remove<SgComponent>();
	);
	ASSERT_ANY_THROW(
		entity.Replace<SgComponent>(SgComponent(1));
	);
}
