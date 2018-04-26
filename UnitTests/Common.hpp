#pragma once
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

class PositionComponent : public Component {
public:
	PositionComponent() = default;
	PositionComponent(float x, float y, float z)
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

class FlagComponent {};

class SgComponent : public Component, public ISingletonComponent
{
public:
	SgComponent()
	{

	}
	SgComponent(int val) : val(val)
	{

	}
	int val;
};