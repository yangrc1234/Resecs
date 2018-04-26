#pragma once
#include <gtest\gtest.h>
#include "Common.hpp"

using namespace Resecs;

TEST(GroupTest, EntityCapture) {
	World testWorld;
	auto og = Group::CreateGroup<PositionComponent, VelocityComponent>(&testWorld);
	auto g = og;
	ASSERT_TRUE(g.Count() == 0);

	std::vector<Entity> testEntities;
	for (size_t i = 0; i < 10; i++)
	{
		auto t = testWorld.Create();
		t.Add<PositionComponent>();
		t.Add<VelocityComponent>();
		testEntities.push_back(t);
	}
	ASSERT_TRUE(g.Count() == 10);
	for (auto& t : g) {
		bool flag = false;
		for (size_t i = 0; i < 10; i++)		//It's not guaranteed that group holds all entities in "order"
		{
			if (t.entityID == testEntities[i].entityID)
				flag = true;
		}
		ASSERT_TRUE(flag);
	}

	//Test capture entities created before group.
	auto group2 = Group::CreateGroup<PositionComponent>(&testWorld);
	ASSERT_TRUE(group2.Count() == 10);


	testEntities[0].Remove<VelocityComponent>();
	ASSERT_TRUE(g.Count() == 9);
	ASSERT_TRUE(group2.Count() == 10);
}

TEST(GroupTest, Event) {
	World testWorld;
	auto og = Group::CreateGroup<PositionComponent, VelocityComponent>(&testWorld);
	
	bool triggered = false;
	GroupEntityEventArgs temp;
	auto signal = og.onGroupChanged.Connect(
		[&](GroupEntityEventArgs arg) {
		temp = arg;
		triggered = true;
	});
	auto entity = testWorld.Create();
	
	{
		auto conn = og.onGroupChanged.Connect(
			[&](GroupEntityEventArgs arg) {
			ASSERT_NO_THROW(arg.entity.Get<PositionComponent>());
		});
		entity.Add(PositionComponent(0, 0, 1));
	}
	ASSERT_TRUE(!triggered);
	entity.Add(VelocityComponent(0, 0, 1));
	ASSERT_TRUE(triggered);
	ASSERT_TRUE(temp.type == GroupEntityEventArgs::Type::Added);
	ASSERT_TRUE(temp.entity.entityID == entity.entityID);

	triggered = false;

	{
		auto conn = og.onGroupChanged.Connect(
			[&](GroupEntityEventArgs arg) {
			ASSERT_NO_THROW(arg.entity.Get<PositionComponent>());
		});
		entity.Remove<PositionComponent>();
	}
	ASSERT_TRUE(triggered);
	ASSERT_TRUE(temp.type == GroupEntityEventArgs::Type::Removed);
	ASSERT_TRUE(temp.entity.entityID == entity.entityID);
}