#pragma once
#include <gtest\gtest.h>
#include "Common.hpp"

using namespace Resecs;

TEST(ComponentTest, AddComponentTest) {
	World testWorld;
	auto entity = testWorld.Create();
	ASSERT_FALSE(entity.Has<PositionComponent>());
	entity.Add(PositionComponent(0, 0, 1));
	ASSERT_TRUE(entity.Has<PositionComponent>());
	ASSERT_TRUE(entity.Get<PositionComponent>()->val == PositionComponent(0, 0, 1).val);

}

TEST(ComponentTest, RemoveComponentTest) {
	World testWorld;
	auto entity = testWorld.Create();
	entity.Add(PositionComponent(0, 0, 1));

	entity.Remove<PositionComponent>();
	ASSERT_FALSE(entity.Has<PositionComponent>());
	entity.Add(PositionComponent(0, 0, 1));
	entity.Destroy();
}

TEST(ComponentTest, EditComponentTest) {
	World testWorld;
	auto entity = testWorld.Create();
	ComponentEventArgs temp;
	auto connection = testWorld.OnComponentChanged.Connect(
		[&](ComponentEventArgs arg) {
		temp = arg;
	}
	);
	entity.Add(PositionComponent(0, 0, 1));
	ASSERT_TRUE(
		temp.type == ComponentEventType::Added
	);
	ASSERT_TRUE(temp.entity == entity.entityID);
	ASSERT_TRUE(temp.componentTypeIndex == 0);

	entity.Remove<PositionComponent>();
	ASSERT_TRUE(
		temp.type == ComponentEventType::Removed
	);
	ASSERT_TRUE(temp.entity == entity.entityID);
	ASSERT_TRUE(temp.componentTypeIndex == 0);

	entity.Add(VelocityComponent(0, 0, 1));
	ASSERT_TRUE(
		temp.type == ComponentEventType::Added
	);
	ASSERT_TRUE(temp.entity == entity.entityID);
	ASSERT_TRUE(temp.componentTypeIndex == 1);

	entity.Remove<VelocityComponent>();
	ASSERT_TRUE(
		temp.type == ComponentEventType::Removed
	);
	ASSERT_TRUE(temp.entity == entity.entityID);
	ASSERT_TRUE(temp.componentTypeIndex == 1);
}

TEST(ComponentTest, GetComponentTest) {
	World testWorld;
	auto entity = testWorld.Create();
	ASSERT_TRUE(entity.Get<PositionComponent>() == nullptr);
	auto ptrAdd = entity.Add(PositionComponent(0, 0, 5));
	ASSERT_TRUE(entity.Get<PositionComponent>() == ptrAdd);
	ptrAdd->val.x = 1;
	ASSERT_TRUE(entity.Get<PositionComponent>()->val == PositionComponent(1, 0, 5).val);
}
