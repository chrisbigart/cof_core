#include "core/interactable_object.h"
#include "core/adventure_map.h"
#include "core/magic_enum.hpp"

#include "core/qt_headers.h"

QDataStream& operator<<(QDataStream& stream, const interactable_object_t& object) {
	stream << object.asset_id << object.object_type << object.x << object.y << object.z << object.width << object.height;
	return stream;
}

QDataStream& operator>>(QDataStream& stream, interactable_object_t& object) {
	stream >> object.asset_id >> object.object_type >> object.x >> object.y >> object.z >> object.width >> object.height;
	return stream;
}

bool interactable_object_t::is_pickupable(interactable_object_t* object) {
	if(!object)
		return false;
	
	return (object->object_type == OBJECT_RESOURCE
			|| object->object_type == OBJECT_TREASURE_CHEST
			|| object->object_type == OBJECT_ARTIFACT
			|| object->object_type == OBJECT_CAMPFIRE
			|| object->object_type == OBJECT_SCHOLAR
			|| object->object_type == OBJECT_SCHOLAR
			|| object->object_type == OBJECT_PANDORAS_BOX
			|| object->object_type == OBJECT_FLOATSAM
			|| object->object_type == OBJECT_SEA_CHEST
			|| object->object_type == OBJECT_SHIPWRECK_SURVIVOR
			|| object->object_type == OBJECT_SEA_BARRELS);
}

void interactable_object_t::copy_interactable_object(interactable_object_t* left, const interactable_object_t* right) {
	assert(left->object_type == right->object_type);
	switch(left->object_type) {
		case OBJECT_MAP_TOWN: *((town_t*)left) = *((town_t*)right); break;
		case OBJECT_SIGN: *((sign_t*)left) = *((sign_t*)right); break;
		case OBJECT_PYRAMID: *((pyramid_t*)left) = *((pyramid_t*)right); break;
		case OBJECT_RESOURCE: *((map_resource_t*)left) = *((map_resource_t*)right); break;
		case OBJECT_ARTIFACT: *((map_artifact_t*)left) = *((map_artifact_t*)right); break;
		case OBJECT_MAP_MONSTER: *((map_monster_t*)left) = *((map_monster_t*)right); break;
		case OBJECT_MINE: *((mine_t*)left) = *((mine_t*)right); break;
		case OBJECT_TREASURE_CHEST: *((treasure_chest_t*)left) = *((treasure_chest_t*)right); break;
		case OBJECT_MAGIC_SHRINE: *((shrine_t*)left) = *((shrine_t*)right); break;
		case OBJECT_WARRIORS_TOMB: *((warriors_tomb_t*)left) = *((warriors_tomb_t*)right); break;
		case OBJECT_REFUGEE_CAMP: *((refugee_camp_t*)left) = *((refugee_camp_t*)right); break;
		case OBJECT_WITCH_HUT: *((witch_hut_t*)left) = *((witch_hut_t*)right); break;
		case OBJECT_BLACKSMITH: *((blacksmith_t*)left) = *((blacksmith_t*)right); break;
		case OBJECT_TELEPORTER: *((teleporter_t*)left) = *((teleporter_t*)right); break;
		case OBJECT_PANDORAS_BOX: *((pandoras_box_t*)left) = *((pandoras_box_t*)right); break;
		case OBJECT_SCHOLAR: *((scholar_t*)left) = *((scholar_t*)right); break;
		case OBJECT_GRAVEYARD: *((graveyard_t*)left) = *((graveyard_t*)right); break;
		case OBJECT_MONUMENT: *((monument_t*)left) = *((monument_t*)right); break;
		case OBJECT_CAMPFIRE: *((campfire_t*)left) = *((campfire_t*)right); break;
		case OBJECT_EYE_OF_THE_MAGI: *((eye_of_the_magi_t*)left) = *((eye_of_the_magi_t*)right); break;
		case OBJECT_WATERWHEEL:
		case OBJECT_WINDMILL: *((flaggable_object_t*)left) = *((flaggable_object_t*)right); break;
		default:
			*left = *right;
	}
}

interactable_object_t* _make_object(interactable_object_e object_type) {
	switch(object_type) {
		case OBJECT_MAP_TOWN:
			return new town_t;
		case OBJECT_SIGN:
			return new sign_t;
		case OBJECT_PYRAMID:
			return new pyramid_t;
		case OBJECT_RESOURCE:
			return new map_resource_t;
		case OBJECT_ARTIFACT:
			return new map_artifact_t;
		case OBJECT_MAP_MONSTER:
			return new map_monster_t;
		case OBJECT_MINE:
			return new mine_t;
		case OBJECT_TREASURE_CHEST:
			return new treasure_chest_t;
		case OBJECT_MAGIC_SHRINE:
			return new shrine_t;
		case OBJECT_WITCH_HUT:
			return new witch_hut_t;
		case OBJECT_BLACKSMITH:
			return new blacksmith_t;
		case OBJECT_TELEPORTER:
			return new teleporter_t;
		case OBJECT_PANDORAS_BOX:
			return new pandoras_box_t;
		case OBJECT_SCHOLAR:
			return new scholar_t;
		case OBJECT_WARRIORS_TOMB:
			return new warriors_tomb_t;
		case OBJECT_REFUGEE_CAMP:
			return new refugee_camp_t;
		case OBJECT_GRAVEYARD:
			return new graveyard_t;
		case OBJECT_MONUMENT:
			return new monument_t;
		case OBJECT_CAMPFIRE:
			return new campfire_t;
		case OBJECT_EYE_OF_THE_MAGI:
			return new eye_of_the_magi_t;
		case OBJECT_WATERWHEEL:
		case OBJECT_WINDMILL:
		case OBJECT_WATCHTOWER:
			return new flaggable_object_t;
		default:
			return new interactable_object_t;

	}

	return nullptr;
}

interactable_object_t* interactable_object_t::make_new_object(interactable_object_e object_type) {
	if(object_type > max_enum_val<interactable_object_e>())
		return nullptr;

	auto obj = _make_object(object_type);
	if(obj)
		obj->object_type = object_type;
	
	return obj;
}

const std::string interactable_object_t::get_name(interactable_object_e type) {
	return magic_enum::enum_name(type).data();
}

