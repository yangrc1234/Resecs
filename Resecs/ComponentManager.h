#pragma once
#include <vector>
#include <queue>
#include "Utils\Common.hpp"

class BaseComponentManager
{
public:
	virtual void Release(int id) = 0;
	virtual ~BaseComponentManager()
	{

	}
};

template <typename TComp>
class ComponentManager : public BaseComponentManager {
public:
	ComponentManager(size_t initialSize = 1024):
		m_componentIndex(initialSize)
	{
		Reallocate(initialSize);
	}
	~ComponentManager()
	{
		std::vector<char> availableSlots(m_maxIndex,true);
		while (!m_releasedIndex.empty())
		{
			availableSlots[m_releasedIndex.front()] = false;
			m_releasedIndex.pop();
		}
		for (size_t i = 0; i < m_maxIndex; i++)
		{
			if (availableSlots[i])
				m_componentPool[i].~TComp();
		}
		Reallocate(0);
	}

	void Reallocate(size_t hintSize) {
		auto newLocation = allocator.allocate(hintSize);
		if (m_componentPool != nullptr) {
			if (hintSize != 0)	//TODO: throw when hintSize is smaller than current size.
				std::memcpy(newLocation, m_componentPool, m_poolSize * sizeof(TComp));
			allocator.deallocate(m_componentPool, m_poolSize);
		}
		m_poolSize = hintSize;
		m_componentPool = newLocation;
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
		m_componentPool[memoryIndex].~TComp();
		m_releasedIndex.push(memoryIndex);
	}
	
	//create a component for id.
	template<typename... TArgs>
	void Create(int id, TArgs&&... args) {
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
			if (memoryIndex > m_poolSize) {
				Reallocate(memoryIndex * 1.5f);
			}
		}
		
		//Construct. Dont use m_componentPool[memoryIndex] = TComp(args), it will cause destruction on position.
		new (m_componentPool + memoryIndex) TComp(std::forward<TArgs>(args)...);
		
		//enlarge index pool.
		EnlargeVectorToFit(m_componentIndex, memoryIndex);
		
		//map index to correct memory position.
		m_componentIndex[id] = memoryIndex;
	}

	int GetCurrentPoolSize() {
		return m_poolSize;
	}

	int GetCurrentFreeSlotCount() {
		return m_poolSize - m_maxIndex + m_releasedIndex.size();
	}
private:
	//For actual component memory, we use raw memory, but not std::vector<TComp>
	//Using vector makes you have to provide a default constructor,
	//Also makes it hard to use constructor or destructor to do sth useful.(like releasing resource)

	std::allocator<TComp> allocator;	//Used for memory allocation. I believe stl implementation is better than mine.
	TComp* m_componentPool = nullptr;
	int m_poolSize = 0;
	size_t m_maxIndex = 0;
	std::vector<int> m_componentIndex;	//map entity ID to actual component id.
	std::queue<int> m_releasedIndex; //indexes that were released but not used yet.
};