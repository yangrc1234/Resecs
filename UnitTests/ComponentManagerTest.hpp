#pragma once
#include <gtest\gtest.h>
#include "Common.hpp"

using namespace Resecs;

static int constructCounter = 0;
static int destructorCounter = 0; 

struct TestComponent : public Component {
	int value;
	TestComponent(int value) : value(value)
	{
		constructCounter++;
	}
	~TestComponent()
	{
		destructorCounter++;
	}
};
TEST(ComponentManagerTest, CreateTest) {
	{
		auto cm = ComponentManager<TestComponent>(100);
		ASSERT_TRUE(cm.GetCurrentFreeSlotCount() == 100);
		for (size_t i = 0; i < 10; i++)
		{
			cm.Create(i, i + 5);
		}
		ASSERT_TRUE(cm.GetCurrentFreeSlotCount() == 90);
		ASSERT_TRUE(constructCounter == 10);
		for (size_t i = 0; i < 10; i++)
		{
			ASSERT_TRUE(cm.Get(i)->value == i+5);
		}
		cm.Release(0);
		ASSERT_TRUE(destructorCounter == 1);
		ASSERT_TRUE(cm.GetCurrentFreeSlotCount() == 91);
	}	
	ASSERT_TRUE(destructorCounter == 10);
}