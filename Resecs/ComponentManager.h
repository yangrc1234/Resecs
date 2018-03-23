#pragma once
#include <vector>
#include <queue>

class BaseComponentManager
{
public:
	virtual ~BaseComponentManager()
	{

	}
};

template <typename TComp>
class ComponentManager : public BaseComponentManager {
public:
	ComponentManager(){

	}

	//GetComponent a component for id.
	TComp* Get(int id) {
		if (id >= m_componentIndex.size())
			return nullptr;
		auto memoryIndex = m_componentIndex[id];
		return &m_componentPool[memoryIndex];
	}
	
	//release a component for id.
	void Release(int id) {
		auto memoryIndex = m_componentIndex[id];
		m_releasedIndex.push(memoryIndex);
	}

	//create a component for id.
	void Create(int id) {
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
			if (m_componentPool.size() <= memoryIndex) {
				m_componentPool.resize((memoryIndex + 1) * 1.5f);
			}
		}

		//enlarge index pool.
		if (m_componentIndex.size() <= id) {
			m_componentIndex.resize((id + 1) * 1.5f);
		}

		//map index to correct memory position.
		m_componentIndex[id] = memoryIndex;
	}

private:
	int m_maxIndex = 0;
	std::vector<TComp> m_componentPool;	//map 
	std::vector<int> m_componentIndex;	//map entity ID to actual component id.
	std::queue<int> m_releasedIndex; //indexes that were released but not used yet.
};