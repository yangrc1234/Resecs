#pragma once
#include <vector>
#include <queue>
#include "Utils\Common.hpp"

class BaseComponentManager
{
public:
	virtual void Release(int id) = 0;
	virtual void Create(int id) = 0;
	virtual ~BaseComponentManager()
	{

	}
};

template <typename TComp>
class ComponentManager : public BaseComponentManager {
public:
	ComponentManager(size_t initialSize = 1024):
		m_componentIndex(initialSize),
		m_componentPool(initialSize)
	{

	}

	//GetComponent a component for id.
	TComp* Get(int id) {
		if (id >= m_componentIndex.size())
			return nullptr;
		auto memoryIndex = m_componentIndex[id];
		return &m_componentPool[memoryIndex];
	}
	
	//release a component for id.
	virtual void Release(int id) override {
		auto memoryIndex = m_componentIndex[id];
		m_releasedIndex.push(memoryIndex);
	}

	//create a component for id.
	virtual void Create(int id) override {
		int memoryIndex;
		if (m_releasedIndex.size() > 0) {
			//use released index.
			memoryIndex = m_releasedIndex.front();
			m_releasedIndex.pop();
		}
		else
		{
			//use a new slot.
			memoryIndex = m_maxIndex++;
			//enlarge pool.
			EnlargeVectorToFit(m_componentPool, memoryIndex);
		}

		//enlarge index pool.
		EnlargeVectorToFit(m_componentIndex, memoryIndex);

		//map index to correct memory position.
		m_componentIndex[id] = memoryIndex;
	}

private:
	int m_maxIndex = 0;
	std::vector<TComp> m_componentPool;	//map 
	std::vector<int> m_componentIndex;	//map entity ID to actual component id.
	std::queue<int> m_releasedIndex; //indexes that were released but not used yet.
};