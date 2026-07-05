#pragma once

#include "core/game_config.h"
#include "core/creature.h"
#include "artifact.h"
#include "spell.h"

#include <string>
#include <bitset>

#ifdef __clang__
#pragma clang diagnostic ignored "-Wignored-attributes"
#endif

#include "core/qt_headers.h"

typedef uint16_t asset_id_t;

enum interactable_object_e : uint16_t {
	OBJECT_UNKNOWN,
	OBJECT_CUSTOM,
	OBJECT_MAP_TOWN, //fixme
	OBJECT_WELL,
	OBJECT_BLACKSMITH,
	OBJECT_TREASURE_CHEST,
	OBJECT_RESOURCE,
	OBJECT_WATCHTOWER,
	OBJECT_WITCH_HUT,
	OBJECT_PYRAMID,
	OBJECT_ARENA,
	OBJECT_CRYPT,
	OBJECT_MINE, //fold these into object_mine
	OBJECT_MINE_UNUSED_1,
	OBJECT_MINE_UNUSED_2,
	OBJECT_MINE_UNUSED_3,
	OBJECT_MINE_UNUSED_4,
	OBJECT_MINE_UNUSED_5,
	OBJECT_SIGN,
	OBJECT_JAIL,
	OBJECT_TELEPORTER,
	OBJECT_TELEPORTER_UNUSED_2,
	OBJECT_TELEPORTER_UNUSED_3,
	OBJECT_TELEPORTER_UNUSED_4,
	OBJECT_TELEPORTER_UNUSED_5,
	OBJECT_TELEPORTER_UNUSED_6,
	OBJECT_TELEPORTER_UNUSED_7,
	OBJECT_TELEPORTER_UNUSED_8,
	//H2 objects
	OBJECT_ALCHEMIST_TOWER,
	OBJECT_FOUNDRY,
	OBJECT_MAGIC_GARDEN,
	OBJECT_WINDMILL,
	OBJECT_WATERWHEEL,
	OBJECT_CAMPFIRE,
	OBJECT_DEFENSIVE_FORT,
	OBJECT_MERCENARY_CAMP,
	OBJECT_STANDING_STONES,
	//OBJECT_WITCH_HUT,
	OBJECT_GRAVEYARD,
	OBJECT_MAGIC_LAMP,
	OBJECT_ARCHERS_HOUSE,
	OBJECT_SPHINX,
	OBJECT_STABLES,
	OBJECT_WATERING_HOLE,
	OBJECT_OASIS,
	OBJECT_TRADING_POST,
	OBJECT_FOUNTAIN,
	OBJECT_KEYMASTER_TENT,
	OBJECT_BARRIER,
	OBJECT_LEANTO,
	OBJECT_ORACLE,
	OBJECT_MAGIC_SHRINE,
	OBJECT_MAGIC_SHRINE_LEVEL2_UNUSED, //removeme
	OBJECT_MAGIC_SHRINE_LEVEL3_UNUSED,
	OBJECT_MAGIC_SHRINE_LEVEL4_UNUSED,
	OBJECT_GAZEBO,
	OBJECT_TROLL_BRIDGE,
	OBJECT_OBELISK,
	OBJECT_ARTIFACT,
	//
	OBJECT_MAP_MONSTER,
	//
	OBJECT_SCHOOL_OF_MAGIC,
	OBJECT_SCHOOL_OF_WAR,
	OBJECT_SCHOOL_OF_OFFENSE, //like the above but offers attack/power
	OBJECT_SCHOOL_OF_DEFENSE, //ditto, offers defense/knowledge
	OBJECT_SCHOLAR,
	OBJECT_PANDORAS_BOX,
	
	OBJECT_SKELETON,
	OBJECT_WAGON,
	OBJECT_MONUMENT, //warrior, scholar, heretic
	OBJECT_WARRIORS_TOMB,
	OBJECT_SANCTUARY,
	OBJECT_CRYSTAL_CAVERN,
	
	//water objects
	OBJECT_FLOATSAM, //71
	OBJECT_SEA_CHEST,
	OBJECT_SEA_BARRELS,
	OBJECT_WHIRLPOOL,
	OBJECT_SHIPWRECK,
	OBJECT_SHIP,
	OBJECT_SHIPWRECK_SURVIVOR,
	
	//move later
	OBJECT_REFUGEE_CAMP,
	OBJECT_EYE_OF_THE_MAGI,
	OBJECT_HUT_OF_THE_MAGI,
	OBJECT_GATE,
	OBJECT_DRAGON_UTOPIA,
	OBJECT_MAGE_TOWER,
	OBJECT_GOBLIN_HUT,
	OBJECT_DJINN_LAMP,
	//
	OBJECT_UNUSED
};

//any interactive object drawn on top of terrain (towns, resources, chests, dwellings, etc)
//does not include heroes
struct interactable_object_t {
	virtual ~interactable_object_t() {}
	static interactable_object_t* make_new_object(interactable_object_e object_type);
	static void copy_interactable_object(interactable_object_t* left, const interactable_object_t* right);
	interactable_object_e object_type = OBJECT_UNKNOWN;
	asset_id_t asset_id = 0;
	//x/y coordinates can be negative if in top left/right corner
	int16_t x;
	int16_t y;
	
	static const std::string get_name(interactable_object_e object_type);
	const std::string get_name() { return get_name(object_type); }
	
	static bool is_pickupable(interactable_object_t* object);
	static bool is_pickupable(interactable_object_e object_type);

	virtual inline void read_data(QDataStream&) { }
	virtual inline int read_data_json(QJsonObject) { return 0; }
	virtual inline void write_data(QDataStream&) const { }
	virtual inline QJsonObject write_data_json() const { return QJsonObject(); }
};

QDataStream& operator<<(QDataStream& stream, const interactable_object_t& object);
QDataStream& operator>>(QDataStream& stream, interactable_object_t& object);


//generic flaggable/visitable objects.
struct flaggable_object_t : interactable_object_t {
	player_e owner = PLAYER_NONE;
	uint8_t visited = 0;
	int16_t date_visited = -1;
 
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct graveyard_t : interactable_object_t {
	uint8_t visited = 0;
	uint8_t size = 0;
	artifact_e artifact_reward = ARTIFACT_NONE;
 
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct teleporter_t : interactable_object_t {
	enum teleporter_type_e : uint8_t {
		TELEPORTER_TYPE_TWO_WAY,
		TELEPORTER_TYPE_ONE_WAY_ENTRANCE,
		TELEPORTER_TYPE_ONE_WAY_EXIT
	};
	
	teleporter_type_e teleporter_type = TELEPORTER_TYPE_TWO_WAY;
	uint8_t color = 0;
	uint8_t is_blockable = 1;
	
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct blacksmith_t : interactable_object_t {
	uint8_t visited = 0;
	uint8_t offers_upgrade = 1;
	artifact_e choice1 = ARTIFACT_NONE;
	artifact_e choice2 = ARTIFACT_NONE;
 
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct sign_t : interactable_object_t {
	//std::string title; ?
	std::string text;
	std::bitset<8> visited = 0; //fix to use game_config::max_players

	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
enum monument_type_e : uint8_t {
	MONUMENT_TYPE_KING, //+1 Attack
	MONUMENT_TYPE_NUN, //+1 Knowledge
	MONUMENT_TYPE_HORNED_MAN, //+1 Power
	MONUMENT_TYPE_ANGEL //+1 Defense
};
 
struct monument_t : interactable_object_t {
	monument_type_e monument_type = MONUMENT_TYPE_KING;

	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct pyramid_t : interactable_object_t {
	std::vector<spell_e> available_spells; //std::bitset<16> available_spells;
	spell_e spell_id = SPELL_UNKNOWN; //unknown == random
	uint8_t visited = false;
 
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct map_resource_t : interactable_object_t {
	uint16_t min_quantity = 3;
	uint16_t max_quantity = 7;
	resource_e resource_type = RESOURCE_RANDOM;
 
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
	static std::string get_resource_name(resource_e type) {
		switch(type) {
			case RESOURCE_RANDOM:
				return QObject::tr("Random").toStdString();
			case RESOURCE_GOLD:
				return QObject::tr("Gold").toStdString();
			case RESOURCE_WOOD:
				return QObject::tr("Wood").toStdString();
			case RESOURCE_ORE:
				return QObject::tr("Ore").toStdString();
			case RESOURCE_GEMS:
				return QObject::tr("Gems").toStdString();
			case RESOURCE_CRYSTALS:
				return QObject::tr("Crystals").toStdString();
			case RESOURCE_MERCURY:
				return QObject::tr("Mercury").toStdString();
			case RESOURCE_SULFUR:
				return QObject::tr("Sulfur").toStdString();
		}
		
		return "Unknown";
	}
};
 
struct map_artifact_t : interactable_object_t {
	//allowed artifact tier(s)
	artifact_e artifact_id = ARTIFACT_NONE;
	artifact_rarity_e rarity = RARITY_UNKNOWN;
	uint8_t can_be_highlighted = 1;
	uint8_t show_full_name = 1;
	std::string custom_pickup_message;
	//guards?

	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct map_monster_t : interactable_object_t {
	unit_type_e unit_type = UNIT_UNKNOWN;
	uint16_t quantity = 0; //0 == random quantity
	//bitfield {
	uint8_t grows = 1;
	uint8_t ranged_melee = 0; //0 == any, 1 == ranged only, 2 == melee only
	uint8_t tier = 0;
	std::string message;
	//}
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct mine_t : interactable_object_t {
	resource_e mine_type = RESOURCE_RANDOM;
	player_e owner = PLAYER_NONE;
	
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
	
	std::string get_name() const {
		switch(mine_type) {
			case RESOURCE_GOLD: return QObject::tr("Gold Mine").toStdString();
			case RESOURCE_WOOD: return QObject::tr("Sawmill").toStdString();
			case RESOURCE_ORE: return QObject::tr("Ore Mine").toStdString();
			case RESOURCE_GEMS: return QObject::tr("Gem Mine").toStdString();
			case RESOURCE_CRYSTALS: return QObject::tr("Obsidian Mine").toStdString();
			case RESOURCE_MERCURY: return QObject::tr("Mercury Distillery").toStdString();
			case RESOURCE_SULFUR: return QObject::tr("Sulfur Mine").toStdString();
			default: return QObject::tr("Unknown Mine").toStdString();
		}
	}	
};
 
struct shrine_t : interactable_object_t {
	uint8_t spell_level = 0;
	spell_e spell = SPELL_UNKNOWN;
	std::vector<spell_e> available_spells;
	uint8_t visited_by_player = 0;
	
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct warriors_tomb_t : interactable_object_t {
	uint8_t visited = 0;
	artifact_e artifact = ARTIFACT_NONE;
	uint8_t visited_by_player = 0;
	//std::vector<artifact_e> available_artifacts;
	
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct refugee_camp_t : interactable_object_t {
	troop_t available_troops;
	
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
#include "core/hero.h"
 
struct witch_hut_t : interactable_object_t {
	skill_e skill = SKILL_NONE;
	uint8_t allows_upgrade = 1;
	std::vector<skill_e> available_skills;
	
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct pandoras_box_t : interactable_object_t {
	enum reward_e : uint8_t {
		PANDORAS_BOX_REWARD_NONE,
		PANDORAS_BOX_REWARD_STATS,
		PANDORAS_BOX_REWARD_SKILL,
		PANDORAS_BOX_REWARD_TALENT_POINT,
		PANDORAS_BOX_REWARD_EXPERIENCE,
		PANDORAS_BOX_REWARD_SPELL,
		PANDORAS_BOX_REWARD_CREATURES,
		PANDORAS_BOX_REWARD_ARTIFACT,
		PANDORAS_BOX_REWARD_MORALE,
		PANDORAS_BOX_REWARD_LUCK,
		PANDORAS_BOX_REWARD_MANA,
		PANDORAS_BOX_REWARD_RESOURCES
	};
	
	std::string custom_message;
	troop_group_t guardians;
	
	struct reward_t {
		reward_e type = PANDORAS_BOX_REWARD_NONE;
		uint16_t subtype = 0; //must be big enough to hold union of { stat_e, skill_e, spell_e, unit_type_e, artifact_e }
		uint32_t magnitude = 0;
		
		friend QDataStream& operator>>(QDataStream& stream, reward_t& reward) {
			stream >> reward.type >> reward.subtype >> reward.magnitude;
			return stream;
		}
		friend QDataStream& operator<<(QDataStream& stream, const reward_t& reward) {
			stream << reward.type << reward.subtype << reward.magnitude;
			return stream;
		}
	};
	
	std::vector<reward_t> rewards;
	
	bool has_guardians() const {
		for(auto& tr : guardians) {
			if(!tr.is_empty())
				return true;
		}
		
		return false;
	}
	
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct scholar_t : interactable_object_t {
	enum reward_e : uint8_t {
		SCHOLAR_REWARD_RANDOM,
		SCHOLAR_REWARD_STATS,
		SCHOLAR_REWARD_SKILL,
		SCHOLAR_REWARD_TALENT_POINT,
		SCHOLAR_REWARD_EXPERIENCE,
		SCHOLAR_REWARD_SPELL,
		SCHOLAR_REWARD_NONE
	};
	reward_e reward_type = SCHOLAR_REWARD_RANDOM;
	union reward_subtype_u {
		stat_e stat = STAT_NONE;
		skill_e skill;
		spell_e spell;
	};
	reward_subtype_u reward_subtype;
	union reward_property_u {
		uint8_t magnitude = 0;
		uint8_t available_spell_levels;
	};
	reward_property_u reward_property;
	
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct eye_of_the_magi_t : interactable_object_t {
	uint8_t reveal_radius = 6;
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct keymaster_tent_t : interactable_object_t {
	uint8_t color = 0;
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct gate_t : interactable_object_t {
	uint8_t color = 0;
	uint8_t is_open = 0;
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
//some object values aren't written to the map file, but do need to be
//written to a save game file or sent across the network
struct treasure_chest_t : interactable_object_t {
	uint8_t value = 0;
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct campfire_t : interactable_object_t {
	uint8_t gold_value = 0; //x100 for value
	uint8_t resource_value = 0;
	resource_e resource_type = RESOURCE_RANDOM;
 
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};
 
struct wagon_t : interactable_object_t {
	enum reward_e : uint8_t {
		WAGON_REWARD_RANDOM,
		WAGON_REWARD_GOLD,
		WAGON_REWARD_RESOURCE,
		WAGON_REWARD_ARTIFACT,
		WAGON_REWARD_EMPTY
	};
	reward_e reward_type = WAGON_REWARD_RANDOM;
	uint8_t gold_resource_value = 0; //x100 for gold value
	resource_e resource_type = RESOURCE_RANDOM;
	artifact_e artifact_id = ARTIFACT_NONE;
	uint8_t visited_by_player = 0;
 
	void read_data(QDataStream& stream) override;
	void write_data(QDataStream& stream) const override;
	int read_data_json(QJsonObject json) override ;
	QJsonObject write_data_json() const override ;
};