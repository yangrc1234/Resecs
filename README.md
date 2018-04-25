# What's Resecs
Resecs is a ecs library I created for myself to use in my little "game engine".
It contains most features an ecs library should have.

## Example
1. Create components
```C++
#include "Resecs/Resecs.h"	//other files are included in this header.
struct Transform : public Component {
public:
	Transform(Vector3 position = Vector3(0), Vector3 scale = Vector3(1)) {
		this->position = position;
		this->scale = scale;
	}
	Vector3 position;
	Vector3 scale;
};  
struct Velocity : public Component {
public:
	Velocity() = default;
	Velocity(float hor, float vert) {
		this->hor = hor;
		this->vert = vert;
	}
	float hor;
	float vert;
};
```
2. Create an entity, and add components to it.
```C++
World world;	//Component type as template args are not required for World type now.
auto entity = world.CreateEntity();
auto pTrans = entity.Add<Transform>();
pTrans->position = Vector3(0.0f,0.0f,0.0f);
```
3. Iterate through entities using component filter.
```C++
world->Each<Transform,Velocity>([=](ExampleEntity entity,Transform* pTrans,Velocity* pVel)
{
	pTrans->position += Vector3(pVel->hor, pVel->vert, 0) * dt;
});
```

## Features
### Memory layout
The class Entity doesn't actually hold any component. It's just a handle for easy life.  
Each type of component are put together in memory, and managed by World class, which is friendly to cache.

### Group
Using World.Each means iterating through all entities. Besides that, a Group can be used for faster iteration. It will cache all entity that matches component type. e.g.
```C++
Group::CreateGroup<PositionComponent,VelocityComponent>(&world)
for(auto& entity : group)
{
	//...do sth with entity.
}
for(auto& entity : group.GetVectorClone())	//if you're going to do sth that will cause the group to change, use this.
{
	//...do sth with entity.
}
```

### Singleton component
It's essential for an ECS to have the ability to have singleton components. It's pretty easy to do so in Resecs.
```C++
struct Time : public Component, public ISingletonComponent{		//notice we add ISingletonComponent.
	float time;
	float dt;
};
```
To access the singleton component, use the World object.
```C++
world.Get<Time>()->dt;
```

## What about system?
I didn't bother too much about system, since I think that system is all about logic, and there're too many ways to write your logic besides put everything in a class System with Start()/Update() then glue all systems together. Once you stick to the point that systems don't hold data, they don't directly communicate with others, you can find your own way to write system.  
Still, I made a System class(To make sure that this is an ECS library, lol). But it's just an empty class, it even doesn't know anything about World or Entity. And another class Feature, which I use to group systems together. Here's what it looks like.
```C++
namespace Resecs {
	class System {
	public:
		virtual void Start() {}
		virtual void Update() {}
	};

	class Feature : public System {
	public:
		std::vector<std::shared_ptr<System>> systems;
		virtual void Start() {
			for (auto pSys : systems) {
				pSys->Start();
			}
		}
		virtual void Update() {
			for (auto pSys : systems) {
				pSys->Update();
			}
		}
	};
}
```

Here're example code from my course assignment, to show you how I use them.
```C++
//app start.
Feature allSys;
allSys.systems = {
	make_shared<RenderSystem>(&tWorld),
	make_shared<MoveSystem>(&tWorld),
	make_shared<SnakeBodyFollowSystem>(&tWorld),
	make_shared<PlayerControlSystem>(&tWorld),
	make_shared<GameStartSystem>(&tWorld),
	make_shared<SnakeGrowSystem>(&tWorld),
	make_shared<UISystem>(&tWorld)
}; 
allSys.Start();
while(true){
	//render loop...
    allSys.Update();
}
``` 

## Requirements
This project is built in VS2015. I haven't tested on other platform, sorry for that.