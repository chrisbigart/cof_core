#pragma once

#include "troop.h"
#include "core/interactable_object.h"
#include "resource.h"

#include <vector>
#include <string>

struct game_t;

enum building_e : uint8_t {
	BUILDING_NONE,
	BUILDING_FORT,
	BUILDING_CASTLE,
	BUILDING_CAPTAINS_QUARTERS,
	BUILDING_TURRET_LEFT,
	BUILDING_TURRET_RIGHT,
	BUILDING_MARKETPLACE,
	BUILDING_AUCTION_HOUSE,
	BUILDING_TAVERN,
	BUILDING_SHIPYARD,
	BUILDING_WAR_MACHINE_YARD,
	BUILDING_MAGE_GUILD_1,
	BUILDING_MAGE_GUILD_2,
	BUILDING_MAGE_GUILD_3,
	BUILDING_MAGE_GUILD_4,
	BUILDING_MAGE_GUILD_5,
	BUILDING_LIGHTHOUSE,
	BUILDING_LIBRARY_ANNEX,
	BUILDING_DEN_OF_THIEVES,
	BUILDING_RAINBOW,
	BUILDING_ALEHOUSE,
	BUILDING_BLACKSMITH,
	BUILDING_SENATE_CHAMBERS,
	BUILDING_GRAVEYARD,
	BUILDING_CEMETERY,
	BUILDING_SOUL_TOMB,
	BUILDING_MANOR,
	BUILDING_NECROSPIRE,
	BUILDING_FLESH_CHAMBER,
	BUILDING_BONEYARD,
	BUILDING_GOBLIN_HUT,
	BUILDING_ORC_CAMP,
	BUILDING_WOLF_DEN,
	BUILDING_TROLL_BRIDGE,
	BUILDING_YETI_CAVE,
	BUILDING_BEHEMOTH_CRAG,
	BUILDING_INFANTRY_BARRACKS,
	BUILDING_ARCHERY_RANGE,
	BUILDING_ARMORY,
	BUILDING_SACRED_CONCLAVE,
	BUILDING_STABLES,
	BUILDING_HALLS_OF_VALOR,
	BUILDING_HELLFORGE,
	BUILDING_TEMPTRESS_DEN,
	BUILDING_HIPPOGRIFF_NEST,
	BUILDING_LABYRINTH,
	BUILDING_CHIMERA_LAB,
	BUILDING_DRAGON_LAIR,
	BUILDING_ENCHANTED_OAK,
	BUILDING_DRYAD_SANCTUARY,
	BUILDING_BOWMASTERS_OUTPOST,
	BUILDING_DRUID_GROVE,
	BUILDING_MYSTIC_MEADOW,
	BUILDING_TOWER_OF_FIRE,
	BUILDING_WORKSHOP,
	BUILDING_DWARVEN_HALL,
	BUILDING_ARCANE_FOUNDRY,
	BUILDING_PALACE,
	BUILDING_ACADEMY,
	BUILDING_THUNDERSPIRE
};

enum building_base_type_e : uint8_t {
	BUILDING_BASE_TYPE_NONE = 0,
	BUILDING_BASE_TYPE_FORT,
	BUILDING_BASE_TYPE_MARKETPLACE,
	BUILDING_BASE_TYPE_TAVERN,
	BUILDING_BASE_TYPE_SHIPYARD,
	BUILDING_BASE_TYPE_WAR_MACHINE_YARD,
	BUILDING_BASE_TYPE_MAGE_GUILD,
	BUILDING_BASE_TYPE_SPECIAL1 = 100,
	BUILDING_BASE_TYPE_SPECIAL2,
	BUILDING_BASE_TYPE_SPECIAL3,
	BUILDING_BASE_TYPE_SPECIAL4,
	BUILDING_BASE_TYPE_GENERATOR1,
	BUILDING_BASE_TYPE_GENERATOR2,
	BUILDING_BASE_TYPE_GENERATOR3,
	BUILDING_BASE_TYPE_GENERATOR4,
	BUILDING_BASE_TYPE_GENERATOR5,
	BUILDING_BASE_TYPE_GENERATOR6
};

enum kingdom_building_e {
	//BUILDING_SENATE_CHAMBERS,
	
	BUILDING_CREATURE_UPGRADER_1building_t_CREATURE_UPGRADER_2,
	BUILDING_CREATURE_UPGRADER_3,
	BUILDING_CREATURE_UPGRADER_4,
	BUILDING_CREATURE_UPGRADER_5,
	BUILDING_CREATURE_UPGRADER_6
};

struct building_t {
	building_t() {}
	building_t(building_e building_type) : type(building_type) {}
	uint16_t asset_id;
	uint8_t row; //where the buy button is located
	//uint8_t slot; //visually what window the image goes in
	building_e type = BUILDING_NONE;
	building_base_type_e subtype;
    //building_base_type_e =
	std::string name;
	std::string description;
	resource_group_t cost;
	hero_class_e matching_faction;// = HERO_CLASS_ALL;
	unit_type_e generated_creature = UNIT_UNKNOWN;
	uint weekly_growth = 0;
	//float town_xpos;
	//float town_ypos;
	//uint8_t town_zpos;	
	
	std::vector<building_e> prerequisites;
	
	static std::string get_name(building_e type);
    static std::string get_description(building_e type);
    static resource_value_t get_resource_value(building_e type);
    //static std::vector<building_t> get_prerequisites(building_e type);
	static building_e get_parent_building(building_e type);
	static resource_group_t get_cost(building_e type);
	//bool are_prerequisites_satisfied();
};

QDataStream & operator<<(QDataStream &stream, const building_t& building);
QDataStream & operator>>(QDataStream &stream, building_t& building);

struct hero_t;

enum town_type_e : uint8_t {
	TOWN_UNKNOWN = 0,
	TOWN_KNIGHT,
	TOWN_BARBARIAN,
	TOWN_WIZARD,
	TOWN_WARLOCK,
	TOWN_NECROMANCER,
	TOWN_SORCERESS,
	TOWN_CUSTOM
	//TOWN_COMBINATION,
};

struct town_t : interactable_object_t {
    town_t() { object_type = OBJECT_MAP_TOWN; }

	uint16_t town_id = 0;

    int reveal_area = 6;
    int observation_area = 12;

    town_type_e town_type = TOWN_UNKNOWN;
    player_e player = PLAYER_NONE;

	void populate_available_spells(uint level);
	std::vector<spell_e> allowed_spells;
	std::vector<spell_e> mage_guild_spells;
	bool has_researched_today = false;
	
    //appearance
    //int id = 0x100;
	int get_development_level() const;
	float get_turret_damage_multiplier() const;
	int get_castle_level() const;

    //available buildings
    std::vector<building_e> available_buildings;
    //built buildings
    std::vector<building_e> built_buildings;
	
	bool can_build_building(building_e building_type, const game_t& gamestate) const;
	bool are_building_prerequisites_satisfied(building_e building) const;
	bool is_building_built(building_e building_type) const;
	bool is_building_enabled(building_e building_type) const;
	int get_mage_guild_level() const;
	bool is_guarded() const;
	bool will_visiting_hero_defend_in_castle() const;
	
	bool build_building(building_e building_type, game_t& gamestate); //should move to game_t / client?
	void setup_buildings();
	void setup_default_spells();
	
	static hero_class_e town_type_to_hero_class(town_type_e town_type);
	static town_type_e  hero_class_to_town_type(hero_class_e hero_class);
	
	bool has_built_today = false;
	void new_day(int day);
	void new_week(bool affected_by_freelancer, bool affected_by_call_to_arms);
	resource_group_t get_daily_income(int day);

    std::string name;
    std::string title;
    std::string description;
    std::string town_history;

	uint8_t icon = 0;

    hero_t* garrisoned_hero = nullptr;
	hero_t* visiting_hero = nullptr;
	
	troop_group_t garrison_troops;
	
	troop_t* get_available_troop_by_type(unit_type_e unit_type);
	std::vector<troop_t> available_troops;
    //quests?

    //ai info
	
	void write_data(QDataStream& stream) const override;
	void read_data(QDataStream& stream) override;
	int read_data_json(QJsonObject json) override;
	QJsonObject write_data_json() const override;
};

struct kingdom_t {

};
