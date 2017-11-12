#pragma once
#include <unordered_set>

namespace Resecs {

	/* The type used to index entity in memory.*/
	using EntityIndex_t = uint32_t;

	/* Entity ID. used to identify an unique entity. 
	Even two entities are created with the same memory index, they don't share the same EntityID.
	*/
	struct EntityID {
	public:
		EntityID() = default;
		EntityID(EntityIndex_t id, int generation) {
			this->index = id;
			this->generation = generation;
		}
		/* The memory index */
		EntityIndex_t index; 
		/* The generation. see comment in World.h for more info. */
		int generation;

		bool operator==(const EntityID& other) const {
			return this->index == other.index && this->generation == other.generation;
		}
	};
}

/* Implement hash function for EntityID.
So EntityID could be used as key of unordered_set
*/
namespace std {
	template <>
	struct std::hash<Resecs::EntityID> {
		size_t operator()(const Resecs::EntityID& k) const {
			// Compute individual hash values for first, second and third
			// http://stackoverflow.com/a/1646913/126995
			size_t res = 17;
			res = res * 31 + std::hash<Resecs::EntityIndex_t>()(k.index);
			res = res * 31 + std::hash<int>()(k.generation);
			return res;
		}
	};
}