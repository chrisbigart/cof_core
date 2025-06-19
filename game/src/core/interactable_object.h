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
	uint16_t z;
	uint8_t width = 0;
	uint8_t height = 0;
	
	static const std::string get_name(interactable_object_e object_type);
	const std::string get_name() { return get_name(object_type); }
	
	static bool is_pickupable(interactable_object_t* object);

	virtual inline void read_data(QDataStream&) { }
	virtual inline int read_data_json(QJsonObject) { return 0; }
	virtual inline void write_data(QDataStream&) const { }
	virtual inline QJsonObject write_data_json() const { return QJsonObject(); }
};

QDataStream& operator<<(QDataStream& stream, const interactable_object_t& object);
QDataStream& operator>>(QDataStream& stream, interactable_object_t& object);

#include <bitset>

//utility function to convert bitset to JSON array
template<size_t N>
QJsonArray bitset_to_json_array(const std::bitset<N>& bitset) {
	QJsonArray array;
	for(size_t i = 0; i < N; ++i) {
		array.append(bitset.test(i));
	}
	return array;
}

inline troop_group_t troops_from_json_array(const QJsonArray& troopsArray) {
	troop_group_t troops;
	size_t i = 0;

	for (const auto& troopValue : troopsArray) {
		if(i < troops.size() && troopValue.isObject()) {
			QJsonObject troopJson = troopValue.toObject();
			troop_t troop;
			troop.unit_type = utils::get_enum_value<unit_type_e>(troopJson["unit_type"].toString());
			troop.stack_size = troopJson["stack_size"].toInt();
			troops[i] = troop;
		}
	}
	return troops;
}

inline QJsonArray troops_to_json_array(const troop_group_t& troops) {
	QJsonArray troopsArray;

	for(const auto& t : troops) {
		QJsonObject troopJson;
		troopJson["unit_type"] = utils::name_from_enum(t.unit_type);
		troopJson["stack_size"] = t.stack_size;
		troopsArray.append(troopJson);
	}
	return troopsArray;
}

//helper function for getting the last (highest) value of an enum
template<typename T> T max_enum_val() {
	return (T)(magic_enum::enum_values<T>().back());
}

//template function for reading (up to a specified number of bytes) fields of a derived class of interactable_object_t to a datastream (e.g. map file or network)
template<typename T> void read_clamped(QDataStream& stream, T& field, const T& min_value, const T& max_value) {
	stream >> field;
	field = std::clamp(field, min_value, max_value);
}

//generic flaggable/visitable objects.
struct flaggable_object_t : interactable_object_t {
	player_e owner = PLAYER_NONE;
	uint8_t visited = 0;
	int16_t date_visited = -1;

	inline void read_data(QDataStream& stream) override {
		read_clamped(stream, owner, PLAYER_NONE, max_enum_val<player_e>());
		read_clamped(stream, visited, (uint8_t)0u, (uint8_t)1u);
		read_clamped(stream, date_visited, (int16_t)-1, (int16_t)30000);
	}
	void write_data(QDataStream& stream) const override {
		stream << owner << visited << date_visited;
	}
	inline int read_data_json(QJsonObject json) override {
		owner = utils::get_enum_value<player_e>(json["owner"].toString());
		visited = json["visited"].toInt();
		date_visited = json["date_visited"].toInt();
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["owner"] = utils::name_from_enum(owner);
		json["visited"] = visited;
		json["date_visited"] = date_visited;
		return json;
	}
};

struct graveyard_t : interactable_object_t {
	uint8_t visited = 0;
	uint8_t size = 0;
	artifact_e artifact_reward = ARTIFACT_NONE;

	inline void read_data(QDataStream& stream) override {
		read_clamped(stream, visited, (uint8_t)0u, (uint8_t)1u);
		read_clamped(stream, size, (uint8_t)0u, (uint8_t)4u);
		read_clamped(stream, artifact_reward, ARTIFACT_NONE, max_enum_val<artifact_e>());
	}
	inline void write_data(QDataStream& stream) const override {
		stream << visited << size << artifact_reward;
	}
	inline int read_data_json(QJsonObject json) override {
		visited = json["visited"].toInt();
		size = json["size"].toInt();
		artifact_reward = utils::get_enum_value<artifact_e>(json["artifact_reward"].toString());
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["visited"] = visited;
		json["size"] = size;
		json["artifact_reward"] = artifact_reward;
		return json;
	}
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
	
	inline void read_data(QDataStream& stream) override {
		read_clamped(stream, teleporter_type, TELEPORTER_TYPE_TWO_WAY, max_enum_val<teleporter_type_e>());
		read_clamped(stream, color, (uint8_t)0u, (uint8_t)16u);
		read_clamped(stream, is_blockable, (uint8_t)0u, (uint8_t)1u);
	}
	void write_data(QDataStream& stream) const override {
		stream << teleporter_type << color << is_blockable;
	}
	inline int read_data_json(QJsonObject json) override {
		teleporter_type = utils::get_enum_value<teleporter_type_e>(json["type"].toString());
		color = json["color"].toInt();
		is_blockable = json["is_blockable"].toBool();
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["teleporter_type"] = utils::name_from_enum(teleporter_type);
		json["color"] = color;
		json["is_blockable"] = is_blockable;
		return json;
	}
};

struct blacksmith_t : interactable_object_t {
	uint8_t visited = 0;
	uint8_t offers_upgrade = 1;
	artifact_e choice1 = ARTIFACT_NONE;
	artifact_e choice2 = ARTIFACT_NONE;

	inline void read_data(QDataStream& stream) override {
		read_clamped(stream, visited, (uint8_t)0u, (uint8_t)1u);
		read_clamped(stream, offers_upgrade, (uint8_t)0u, (uint8_t)1u);
		read_clamped(stream, choice1, ARTIFACT_NONE, max_enum_val<artifact_e>());
		read_clamped(stream, choice2, ARTIFACT_NONE, max_enum_val<artifact_e>());
	}
	inline void write_data(QDataStream& stream) const override {
		stream << visited << offers_upgrade << choice1 << choice2;
	}
	inline int read_data_json(QJsonObject json) override {
		visited = json["visited"].toInt();
		offers_upgrade = json["offers_upgrade"].toBool();
		choice1 = utils::get_enum_value<artifact_e>(json["choice1"].toString());
		choice2 = utils::get_enum_value<artifact_e>(json["choice2"].toString());
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["visited"] = visited;
		json["offers_upgrade"] = offers_upgrade;
		json["choice1"] = utils::name_from_enum(choice1);
		json["choice2"] = utils::name_from_enum(choice2);
		return json;
	}
};

struct sign_t : interactable_object_t {
	//std::string title; ?
	std::string text;
	std::bitset<8> visited = 0; //fix to use game_config::max_players
	inline void read_data(QDataStream& stream) override {
		text = stream_read_string(stream);
		quint8 val;
		stream >> val;
		visited = val;
	}
	inline void write_data(QDataStream& stream) const override {
		stream_write_string(stream, text);
		stream << (quint8)visited.to_ulong();
	}
	inline int read_data_json(QJsonObject json) override {
		text = json["text"].toString().toStdString();
		visited = std::bitset<8>(json["visited"].toInt());
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["text"] = QString::fromStdString(text).toStdString().c_str();
		json["visited"] = static_cast<int>(visited.to_ulong());
		return json;
	}
};

enum monument_type_e : uint8_t {
	MONUMENT_TYPE_KING, //+1 Attack
	MONUMENT_TYPE_NUN, //+1 Knowledge
	MONUMENT_TYPE_HORNED_MAN, //+1 Power
	MONUMENT_TYPE_ANGEL //+1 Defense
};

struct monument_t : interactable_object_t {
	monument_type_e monument_type = MONUMENT_TYPE_KING;
	inline void read_data(QDataStream& stream) override {
		stream >> monument_type;
	}
	inline void write_data(QDataStream& stream) const override {
		stream << monument_type;
	}
	inline int read_data_json(QJsonObject json) override {
		monument_type = (monument_type_e)json["monument_type"].toInt();
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["monument_type"] = monument_type;
		return json;
	}
};

struct pyramid_t : interactable_object_t {
	std::vector<spell_e> available_spells; //std::bitset<16> available_spells;
	spell_e spell_id = SPELL_UNKNOWN; //unknown == random
	uint8_t visited = false;

	inline void read_data(QDataStream& stream) override {
		read_clamped(stream, spell_id, SPELL_UNKNOWN, max_enum_val<spell_e>());
		read_clamped(stream, visited, (uint8_t)0u, (uint8_t)1u);
		stream_read_vector(stream, available_spells, SPELL_UNKNOWN, max_enum_val<spell_e>());
	}
	inline void write_data(QDataStream& stream) const override {
		stream << spell_id << visited;
		stream_write_vector(stream, available_spells);
	}
	inline int read_data_json(QJsonObject json) override {
		spell_id = utils::get_enum_value<spell_e>(json["spell_id"].toString());
		visited = json["visited"].toBool();
		
		available_spells.clear();
		QJsonArray spellsArray = json["available_spells"].toArray();
		for(const auto& spellValue : spellsArray) {
			available_spells.push_back(utils::get_enum_value<spell_e>(spellValue.toString()));
		}
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["spell_id"] = utils::name_from_enum(spell_id);
		json["visited"] = visited;
		
		QJsonArray spellsArray;
		for (const auto& spell : available_spells) {
			spellsArray.append(utils::name_from_enum(spell));
		}
		json["available_spells"] = spellsArray;
		
		return json;
	}
};

struct map_resource_t : interactable_object_t {
	uint16_t min_quantity = 3;
	uint16_t max_quantity = 7;
	resource_e resource_type = RESOURCE_RANDOM;

	inline void read_data(QDataStream& stream) override {
		read_clamped(stream, min_quantity, (uint16_t)0u, (uint16_t)10000u);
		read_clamped(stream, max_quantity, (uint16_t)0u, (uint16_t)10000u);
		read_clamped(stream, resource_type, RESOURCE_RANDOM, max_enum_val<resource_e>());
	}
	inline void write_data(QDataStream& stream) const override {
		stream << min_quantity << max_quantity << resource_type;
	}
	inline int read_data_json(QJsonObject json) override {
		min_quantity = json["min_quantity"].toInt();
		max_quantity = json["max_quantity"].toInt();
		resource_type = utils::get_enum_value<resource_e>(json["resource_type"].toString());
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["min_quantity"] = min_quantity;
		json["max_quantity"] = max_quantity;
		json["resource_type"] = utils::name_from_enum(resource_type);
		return json;
	}
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
	inline void read_data(QDataStream& stream) override {
		read_clamped(stream, artifact_id, ARTIFACT_NONE, (artifact_e)0xffff /*max_enum_val<artifact_e>()*/);
		read_clamped(stream, rarity, RARITY_UNKNOWN, max_enum_val<artifact_rarity_e>());
		read_clamped(stream, can_be_highlighted, (uint8_t)0u, (uint8_t)1u);
		read_clamped(stream, show_full_name, (uint8_t)0u, (uint8_t)1u);
		custom_pickup_message = stream_read_string(stream);
	}
	inline void write_data(QDataStream& stream) const override {
		stream << artifact_id << rarity << can_be_highlighted << show_full_name;
		stream_write_string(stream, custom_pickup_message);
	}
	inline int read_data_json(QJsonObject json) override {
		artifact_id = utils::get_enum_value<artifact_e>(json["artifact_id"].toString());
		rarity = utils::get_enum_value<artifact_rarity_e>(json["rarity"].toString());
		can_be_highlighted = json["can_be_highlighted"].toInt();
		show_full_name = json["show_full_name"].toInt();
		custom_pickup_message = json["custom_pickup_message"].toString().toStdString();
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["artifact_id"] = utils::name_from_enum(artifact_id);
		json["rarity"] = utils::name_from_enum(rarity);
		json["can_be_highlighted"] = can_be_highlighted;
		json["show_full_name"] = show_full_name;
		json["custom_pickup_message"] = QString::fromStdString(custom_pickup_message).toStdString().c_str();
		return json;
	}
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
	inline void read_data(QDataStream& stream) override {
		read_clamped(stream, unit_type, UNIT_UNKNOWN, max_enum_val<unit_type_e>());
		read_clamped(stream, quantity, (uint16_t)0u, (uint16_t)50000u);
		read_clamped(stream, grows, (uint8_t)0u, (uint8_t)1u);
		read_clamped(stream, ranged_melee, (uint8_t)0u, (uint8_t)2u);
		read_clamped(stream, tier, (uint8_t)0u, (uint8_t)game_config::CREATURE_TIERS);
		message = stream_read_string(stream, 1024);
	}
	inline void write_data(QDataStream& stream) const override {
		stream << unit_type << quantity << grows << ranged_melee << tier;
		stream_write_string(stream, message);
	}
	inline int read_data_json(QJsonObject json) override {
		unit_type = utils::get_enum_value<unit_type_e>(json["unit_type"].toString());
		quantity = json["quantity"].toInt();
		grows = json["grows"].toBool();
		ranged_melee = json["ranged_melee"].toInt();
		tier = json["tier"].toInt();
		message = json["message"].toString().toStdString();
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["unit_type"] = utils::name_from_enum(unit_type);
		json["quantity"] = quantity;
		json["grows"] = grows;
		json["ranged_melee"] = ranged_melee;
		json["tier"] = tier;
		json["message"] = QString::fromStdString(message).toStdString().c_str();
		return json;
	}
};

struct mine_t : interactable_object_t {
	resource_e mine_type = RESOURCE_RANDOM;
	player_e owner = PLAYER_NONE;
	
	inline void read_data(QDataStream& stream) override {
		read_clamped(stream, mine_type, RESOURCE_RANDOM, max_enum_val<resource_e>());
		read_clamped(stream, owner, PLAYER_NONE, max_enum_val<player_e>());
	}
	inline void write_data(QDataStream& stream) const override {
		stream << mine_type << owner;
	}
	inline int read_data_json(QJsonObject json) override {
		mine_type = utils::get_enum_value<resource_e>(json["mine_type"].toString());
		owner = utils::get_enum_value<player_e>(json["owner"].toString());
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["mine_type"] = utils::name_from_enum(mine_type);
		json["owner"] = utils::name_from_enum(owner);
		return json;
	}
	
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

	void set_visited_by_player(player_e player) {
		if(player == PLAYER_NONE || player == PLAYER_NEUTRAL)
			return;

		int offset = std::clamp((int)player - 1, 0, 7);
		visited_by_player |= (1 << offset);
	}

	bool was_visited_by_player(player_e player) {
		if(player == PLAYER_NONE || player == PLAYER_NEUTRAL)
			return false;

		int offset = std::clamp((int)player - 1, 0, 7);
		return visited_by_player & (1 << offset);
	}
	
	inline void read_data(QDataStream& stream) override {
		read_clamped(stream, spell_level, (uint8_t)0u, (uint8_t)5u);
		read_clamped(stream, spell, SPELL_UNKNOWN, max_enum_val<spell_e>());
		stream_read_vector(stream, available_spells);
	}
	inline void write_data(QDataStream& stream) const override {
		stream << spell_level << spell;
		stream_write_vector(stream, available_spells);
	}
	inline int read_data_json(QJsonObject json) override {
		spell_level = json["spell_level"].toInt();
		spell = utils::get_enum_value<spell_e>(json["spell"].toString());
		
		available_spells.clear();
		QJsonArray spellsArray = json["available_spells"].toArray();
		for (const auto& spellValue : spellsArray) {
			available_spells.push_back(utils::get_enum_value<spell_e>(spellValue.toString()));
		}
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["spell_level"] = spell_level;
		json["spell"] = utils::name_from_enum(spell);
		
		QJsonArray spellsArray;
		for (const auto& sp : available_spells) {
			spellsArray.append(utils::name_from_enum(sp));
		}
		json["available_spells"] = spellsArray;
		
		return json;
	}
};

struct warriors_tomb_t : interactable_object_t {
	uint8_t visited = 0;
	artifact_e artifact = ARTIFACT_NONE;
	//std::vector<artifact_e> available_artifacts;
	
	inline void read_data(QDataStream& stream) override {
		read_clamped(stream, visited, (uint8_t)0u, (uint8_t)1u);
		read_clamped(stream, artifact, ARTIFACT_NONE, max_enum_val<artifact_e>());
	}
	inline void write_data(QDataStream& stream) const override {
		stream << visited << artifact;
	}
	inline int read_data_json(QJsonObject json) override {
		visited = json["visited"].toInt();
		artifact = utils::get_enum_value<artifact_e>(json["artifact"].toString());
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["visited"] = visited;
		json["artifact"] = utils::name_from_enum(artifact);
		return json;
	}
};

struct refugee_camp_t : interactable_object_t {
	troop_t available_troops;
	
	inline void read_data(QDataStream& stream) override {
		stream >> available_troops;
	}
	inline void write_data(QDataStream& stream) const override {
		stream << available_troops;
	}
	inline int read_data_json(QJsonObject json) override {
		QJsonObject troopJson = json["available_troops"].toObject();
		available_troops.unit_type = utils::get_enum_value<unit_type_e>(troopJson["unit_type"].toString());
		available_troops.stack_size = troopJson["stack_size"].toInt();
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject troopJson;
		troopJson["unit_type"] = utils::name_from_enum(available_troops.unit_type);
		troopJson["stack_size"] = available_troops.stack_size;
		QJsonObject json;
		json["available_troops"] = troopJson;
		return json;
	}
};

#include "core/hero.h"

struct witch_hut_t : interactable_object_t {
	skill_e skill = SKILL_NONE;
	uint8_t allows_upgrade = 1;
	std::vector<skill_e> available_skills;
	
	inline void read_data(QDataStream& stream) override {
		read_clamped(stream, skill, SKILL_NONE, max_enum_val<skill_e>());
		read_clamped(stream, allows_upgrade, (uint8_t)0u, (uint8_t)1u);
		stream_read_vector(stream, available_skills, SKILL_NONE, max_enum_val<skill_e>());
	}
	inline void write_data(QDataStream& stream) const override {
		stream << skill << allows_upgrade;
		stream_write_vector(stream, available_skills);
	}
	inline int read_data_json(QJsonObject json) override {
		skill = utils::get_enum_value<skill_e>(json["skill"].toString());
		allows_upgrade = json["allows_upgrade"].toBool();
		
		available_skills.clear();
		QJsonArray skillsArray = json["available_skills"].toArray();
		for (const auto& skillValue : skillsArray) {
			available_skills.push_back(utils::get_enum_value<skill_e>(skillValue.toString()));
		}
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["skill"] = utils::name_from_enum(skill);
		json["allows_upgrade"] = allows_upgrade;
		
		QJsonArray skillsArray;
		for (const auto& sk : available_skills) {
			skillsArray.append(utils::name_from_enum(sk));
		}
		json["available_skills"] = skillsArray;
		
		return json;
	}
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
	
	inline void read_data(QDataStream& stream) override {
		custom_message = stream_read_string(stream);
		stream_read_array(stream, guardians);
		stream_read_vector(stream, rewards);
	}
	void write_data(QDataStream& stream) const override {
		stream_write_string(stream, custom_message);
		stream_write_vector(stream, guardians);
		stream_write_vector(stream, rewards);
	}
	inline int read_data_json(QJsonObject json) override {
		custom_message = json["custom_message"].toString().toStdString();
		guardians = troops_from_json_array(json["guardians"].toArray());
		
		rewards.clear();
		QJsonArray rewardsArray = json["rewards"].toArray();
		for (const auto& rewardValue : rewardsArray) {
			QJsonObject rewardJson = rewardValue.toObject();
			reward_t reward;
			reward.type = utils::get_enum_value<reward_e>(rewardJson["type"].toString());
			reward.subtype = rewardJson["subtype"].toInt();
			reward.magnitude = rewardJson["magnitude"].toInt();
			rewards.push_back(reward);
		}
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["custom_message"] = QString::fromStdString(custom_message).toStdString().c_str();
		json["guardians"] = troops_to_json_array(guardians);
		
		QJsonArray rewardsArray;
		for(const auto& reward : rewards) {
			QJsonObject rewardJson;
			rewardJson["type"] = utils::name_from_enum(reward.type);
			rewardJson["subtype"] = reward.subtype;
			rewardJson["magnitude"] = (int)reward.magnitude;
			rewardsArray.append(rewardJson);
		}
		json["rewards"] = rewardsArray;
		
		return json;
	}
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
	
	inline void read_data(QDataStream& stream) override {
		stream >> reward_type >> reward_subtype.stat >> reward_property.magnitude;
	}
	inline void write_data(QDataStream& stream) const override {
		stream << reward_type << reward_subtype.stat << reward_property.magnitude;
	}
	inline int read_data_json(QJsonObject json) override {
		reward_type = utils::get_enum_value<reward_e>(json["reward_type"].toString());
		reward_subtype.stat = utils::get_enum_value<stat_e>(json["reward_subtype"].toString());
		reward_property.magnitude = json["reward_property"].toInt();
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["reward_type"] = utils::name_from_enum(reward_type);
		json["reward_subtype"] = utils::name_from_enum(reward_subtype.stat); // Assuming stat is the default active member
		json["reward_property"] = reward_property.magnitude; // Adjust based on active member
		return json;
	}
};

struct eye_of_the_magi_t : interactable_object_t {
	uint8_t reveal_radius = 6;
	inline void read_data(QDataStream& stream) override {
		stream >> reveal_radius;
	}
	inline void write_data(QDataStream& stream) const override {
		stream << reveal_radius;
	}
	inline int read_data_json(QJsonObject json) override {
		reveal_radius = json["reveal_radius"].toInt();
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["reveal_radius"] = reveal_radius;
		return json;
	}
};

//some object values aren't written to the map file, but do need to be
//written to a save game file or sent across the network
struct treasure_chest_t : interactable_object_t {
	uint8_t value = 0;
	inline void read_data(QDataStream& stream) override {
		stream >> value;
	}
	inline void write_data(QDataStream& stream) const override {
		stream << value;
	}
	inline int read_data_json(QJsonObject json) override {
		value = json["value"].toInt();
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["value"] = value;
		return json;
	}
};

struct campfire_t : interactable_object_t {
	uint8_t gold_value = 0; //x100 for value
	uint8_t resource_value = 0;
	resource_e resource_type = RESOURCE_RANDOM;

	inline void read_data(QDataStream& stream) override {
		stream >> gold_value >> resource_value >> resource_type;
	}
	inline void write_data(QDataStream& stream) const override {
		stream << gold_value << resource_value << resource_type;
	}
	inline int read_data_json(QJsonObject json) override {
		gold_value = json["gold_value"].toInt();
		resource_value = json["resource_value"].toInt();
		resource_type = utils::get_enum_value<resource_e>(json["resource_type"].toString());
		return 0;
	}
	inline QJsonObject write_data_json() const override {
		QJsonObject json;
		json["gold_value"] = gold_value;
		json["resource_value"] = resource_value;
		json["reward_type"] = utils::name_from_enum(resource_type);
		return json;
	}
};