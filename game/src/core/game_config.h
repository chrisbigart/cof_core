#pragma once

#include <cctype>
#include <string>
#include <vector>

typedef unsigned int uint;
//make sure the integer type is big enough to store our battlefield coordinates
//static_assert(pow(sizeof(sint) - 1, 2) - 1 > BATTLEFIELD_WIDTH);

//typedef /*unsigned*/ short sint;
typedef signed int sint;
typedef unsigned int uint;

struct creature_t;
struct talent_t;
struct skill_t;
struct artifact_t;
struct building_t;
struct spell_t;
struct hero_t;
struct hero_specialty_t;
struct object_info_t;
enum building_e : uint8_t;
enum talent_e : uint8_t;
enum skill_e : uint8_t;
enum artifact_e : uint16_t;
enum unit_type_e : uint8_t;
enum spell_e : uint16_t;
enum town_type_e : uint8_t;
enum hero_class_e : uint8_t;
enum buff_e : uint8_t;
enum buff_type_e : uint8_t;
enum hero_specialty_e : uint8_t;

enum player_e : uint8_t {
	PLAYER_NONE = 0,
	PLAYER_1,
	PLAYER_2,
	PLAYER_3,
	PLAYER_4,
	PLAYER_5,
	PLAYER_6,
	PLAYER_7,
	PLAYER_8,
	PLAYER_NEUTRAL
};

enum resource_e : uint8_t {
	RESOURCE_RANDOM = 0,
	RESOURCE_GOLD,
	RESOURCE_WOOD,
	RESOURCE_ORE,
	RESOURCE_GEMS,
	RESOURCE_CRYSTALS,
	RESOURCE_MERCURY,
	RESOURCE_SULFUR
	//h2/H3 resources
	//RESOURCE_CRYSTAL_H2,
	//RESOURCE_SULFUR_H2
};

struct buff_info_t {
	buff_e buff_id = (buff_e)0; //BUFF_NONE;
	buff_type_e type = (buff_type_e)0; //BUFF_TYPE_UNKNOWN;
	uint16_t asset_id = 0;
	std::string name;
	std::string description;
};

class game_config {
public:
	static float get_attack_bonus_multiplier();
	static float get_defense_bonus_multiplier();
	static uint get_max_attack_bonus_difference();
	static uint get_max_defense_bonus_difference();

	static int load_game_data(const std::string& path_prefix = std::string());
	
	static int load_object_data(const std::string& path_prefix = std::string());
	static int load_creatures(const std::string& path_prefix = std::string());
	static int load_heroes(const std::string& path_prefix = std::string());
	static int load_talents(const std::string& path_prefix = std::string());
	static int load_skills(const std::string& path_prefix = std::string());
	static int load_specialties(const std::string& path_prefix = std::string());
	static int load_artifacts(const std::string& path_prefix = std::string());
	static int load_spells(const std::string& path_prefix = std::string());
	static int load_buildings(const std::string& path_prefix = std::string());
	static int load_town_names(const std::string& path_prefix = std::string());
	static int load_buffs(const std::string& path_prefix = std::string());
	
	static const object_info_t& get_object_info(int16_t asset_id);
	static const talent_t& get_talent(talent_e talent_id);
	static const creature_t& get_creature(unit_type_e unit_id);
	static const artifact_t& get_artifact(artifact_e artifact_id);
	static const skill_t& get_skill(skill_e skill_id);
	static const hero_specialty_t& get_specialty(hero_specialty_e specialty_id);
	static const building_t& get_building(/*building_base_type_e base_type, */building_e building_id);
	static const spell_t& get_spell(spell_e spell_id);
	static const std::string get_random_town_name(town_type_e town_type);
	static const buff_info_t& get_buff_info(buff_e buff_id);
	
	static const std::vector<hero_t>& get_heroes() { return heroes; }
	static const std::vector<hero_t>& get_heroes(hero_class_e hero_class);
	static const std::vector<talent_t>& get_talents() { return talents; }
	static const std::vector<artifact_t>& get_artifacts() { return artifacts; }
	static const std::vector<skill_t>& get_skills() { return skills; }
	static const std::vector<creature_t>& get_creatures() { return creatures; }
	static const std::vector<spell_t>& get_spells() { return spells; }
	static const std::vector<building_t>& get_buildings() { return buildings; }
	
	static void add_or_update_custom_artifact(const artifact_t& artifact);
	static artifact_e get_next_custom_artifact_id();

	static const uint HERO_ARTIFACT_SLOTS = 15;
	static const uint HERO_BACKPACK_SLOTS = 72;
	static const uint HERO_SKILL_SLOTS = 6;
	static const uint HERO_TALENT_POINTS = 10;
	static const uint HERO_TALENT_SLOTS = 17;
	static const uint HERO_TROOP_SLOTS = 7;
	static const uint CREATURE_TIERS = 6;
	static const uint MAX_UNIT_BUFFS = 16;
	static const uint MAX_INHERENT_BUFFS = 8;
	static constexpr uint BATTLEFIELD_WIDTH = 17;
	static const uint BATTLEFIELD_HEIGHT = 11;
	static const uint MAX_NUMBER_OF_PLAYERS = 8;
	static const uint MAX_NUMBER_OF_HEROES = 8;
	static const uint MAX_TROOP_STACK_SIZE = 65000;
	static const uint MAX_COMBAT_ROUNDS = 100;
	
private:
	static std::vector<object_info_t> objects;
	static std::vector<creature_t> creatures;
	static std::vector<talent_t> talents;
	static std::vector<artifact_t> artifacts;
	static std::vector<skill_t> skills;
	static std::vector<hero_specialty_t> specialties;
	static std::vector<spell_t> spells;
	static std::vector<building_t> buildings;
	static std::vector<hero_t> heroes;
	static std::vector<buff_info_t> buff_info;
	static std::vector<std::pair<town_type_e, std::string>> town_names;
};
