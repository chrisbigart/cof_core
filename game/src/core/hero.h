#pragma once

#include "core/utils.h"
#include "core/game_config.h"
#include "core/artifact.h"
#include "core/troop.h"
#include "core/spell.h"
#include "core/script.h"

enum terrain_type_e : uint8_t;

enum split_mode_e {
	SPLIT_ONE,
	SPLIT_EVEN,
	SPLIT_CUSTOM
};

enum stat_e : uint8_t {
	STAT_NONE = 0,
	STAT_ATTACK,
	STAT_DEFENSE,
	STAT_POWER,
	STAT_KNOWLEDGE
	//,STAT_COMMAND, STAT_STAMINA
};

enum skill_e : uint8_t {
	SKILL_NONE = 0,
	SKILL_HOLY_MAGIC,
	SKILL_ARCANE_MAGIC,
	SKILL_DESTRUCTION_MAGIC,
	SKILL_DEATH_MAGIC,
	SKILL_NATURE_MAGIC,
	SKILL_INTELLIGENCE,
	SKILL_SORCERY,
	SKILL_SPELL_PENETRATION,
	SKILL_LOGISTICS,
	SKILL_PATHFINDING,
	SKILL_NECROMANCY,
	SKILL_SCOUTING,
	SKILL_LUCK,
	SKILL_WAR_MACHINES,
	SKILL_RESISTANCE,
	SKILL_LEADERSHIP,
	SKILL_TREACHERY,
	SKILL_SPELL_HASTE,
	SKILL_ARCHERY,
	SKILL_OFFENSE,
	SKILL_DEFENSE,
	SKILL_SAILING,
	///
	SKILL_ALL_SKILLS = 64
};

enum hero_specialty_e : uint8_t {
	SPECIALTY_UNKNOWN,
	//magic schools
	SPECIALTY_ARCANE_MAGIC,
	SPECIALTY_HOLY_MAGIC,
	SPECIALTY_NATURE_MAGIC,
	SPECIALTY_DESTRUCTION_MAGIC,
	SPECIALTY_DEATH_MAGIC,
	//spell damage types
	SPECIALTY_FIRE_MAGIC,
	SPECIALTY_LIGHTNING_MAGIC,
	SPECIALTY_EARTH_MAGIC,
	SPECIALTY_FROST_MAGIC,
	SPECIALTY_CHAOS_MAGIC,
	/*HOLY_MAGIC_DAMAGE*/
	SPECIALTY_INTELLIGENCE,
	SPECIALTY_SORCERY,
	SPECIALTY_SPELL_PENETRATION,
	SPECIALTY_LOGISTICS,
	SPECIALTY_PATHFINDING,
	
	SPECIALTY_ARCHERS,
	SPECIALTY_CRUSADERS,
	SPECIALTY_VAMPIRES,
	SPECIALTY_MAGI,
	SPECIALTY_ORCS,
	SPECIALTY_WAR_MACHINES,
	SPECIALTY_GOLD,
	SPECIALTY_FOOTMEN,
	SPECIALTY_PHOENIX,
	SPECIALTY_UNICORNS,
	SPECIALTY_DRUIDS,
	SPECIALTY_ELVES,
	SPECIALTY_SKELETONS,
	SPECIALTY_LICHES,
	SPECIALTY_RESTORATION,
	SPECIALTY_RESISTANCE,
	SPECIALTY_WOLVES,
	SPECIALTY_BISHOPS,
	SPECIALTY_BLESS,
	SPECIALTY_OFFENSE,
	SPECIALTY_GOBLINS,
	SPECIALTY_DEFENSE,
	SPECIALTY_REANIMATE_DEAD,
	SPECIALTY_DARK_ENERGY,
	SPECIALTY_SHADOW_BOLT,
	SPECIALTY_CRIPPLE,
	SPECIALTY_LIGHTNING_BOLT,
	SPECIALTY_ICE_LANCE,
	SPECIALTY_ELECTROCUTE,
	SPECIALTY_SPELL_HASTE
};

struct specialty_multiplier_t {
	uint16_t fixed = 0;
	//todo: fold these into bitfields
	uint8_t is_power_multiplied = 0;
	uint8_t multi = 0;

	inline int get_value(int level) const {
		return (int)(fixed + (is_power_multiplied ? level : 1.f) * multi);
	}
};

struct hero_specialty_t {
	hero_specialty_e id;
	std::string name;
	std::string description;
	std::string asset_string;
	std::vector<hero_class_e> allowed_classes;
	specialty_multiplier_t multiplier;
};

//todo: change to uint16_t
enum talent_e : uint8_t {
	TALENT_NONE = 0,
	TALENT_KNIGHTS_HONOR,
	TALENT_EAGLE_EYE,
	TALENT_KING_OF_THE_HILL,
	TALENT_UNDERDOG,
	TALENT_BISHOPS_BLESSING,
	TALENT_FAMILIAR_GROUND,
	TALENT_MACHINIST,
	TALENT_COMMANDER,
	TALENT_FIEFDOM,
	TALENT_FORAGING,
	TALENT_PREPARATION,
	TALENT_HOLY_WARD,
	TALENT_ON_GUARD,
	TALENT_CALL_TO_ARMS,
	TALENT_VENGEANCE,
	TALENT_NAVIGATOR,
	TALENT_ARMOURER,
	TALENT_VALOR,
	TALENT_CRUSADE,
	TALENT_REDEMPTION,
	TALENT_FAERIES_FORTUNE,
	TALENT_BLOSSOM_OF_LIFE,
	TALENT_GRASSLANDS_GRACE,
	TALENT_NATURAL_WARD,
	TALENT_REPLENISHMENT,
	TALENT_MENDING,
	TALENT_INSPIRATION,
	TALENT_PATH_OF_THE_MAGI,
	TALENT_PATH_OF_THE_PIOUS,
	TALENT_FORTUNE_FAVORS_THE_BOLD,
	TALENT_THUNDEROUS_SKIES,
	TALENT_LUCKY_FIND,
	TALENT_FROZEN_HEART,
	TALENT_GIFT_OF_THE_GODDESS,
	TALENT_SWIFT_STRIKE,
	TALENT_ENCHANT,
	TALENT_WING_CLIP,
	TALENT_FAERIES_FURY,
	TALENT_REVIVE,
	TALENT_POWER_HUNGRY,
	TALENT_HEXMASTER,
	TALENT_CRIPPLING_FEAR,
	TALENT_ETHER_FEAST,
	TALENT_SUMMONER,
	TALENT_DRAGON_ALLEGIANCE,
	TALENT_ELDRITCH_KNOWLEDGE,
	TALENT_THE_DARK_ARTS,
	TALENT_OBLITERATION,
	TALENT_DEATHBRINGER,
	TALENT_DRAGONBORNE,
	TALENT_DRAGON_MASTER,
	TALENT_STAND_ALONE,
	TALENT_PYROMANIAC,
	TALENT_POWER_SIPHON,
	TALENT_UNLEASH_CHAOS,
	TALENT_INSANITY,
	TALENT_MEGALOMANIA,
	TALENT_UNBRIDLED_POWER,
	TALENT_OFFENSE,
	TALENT_NATURAL_RESISTANCE,
	TALENT_TRADER,
	TALENT_SEIZE_THE_SIEGE,
	TALENT_FREELANCER,
	TALENT_TIRELESS,
	TALENT_CHAIN_BREAKER,
	TALENT_FEROCITY,
	TALENT_SLAYER,
	TALENT_PACK_LEADER,
	TALENT_FRESH_START,
	TALENT_RECKLESS_FORCE,
	TALENT_NATURAL_LEADER,
	TALENT_JUGGERNAUT,
	TALENT_OUTLAW,
	TALENT_FIRST_STRIKE,
	TALENT_QUICKDRAW,
	TALENT_DESOLATOR,
	TALENT_WEAPON_MASTER,
	TALENT_CRITICAL_STRIKE,
	TALENT_RALLYING_CRY,
	TALENT_OVERWHELM,
	TALENT_WIZARDRY,
	TALENT_ESSENCE_STRIP,
	TALENT_OVERDRIVE,
	TALENT_MYSTICISM,
	TALENT_DEBILITATE,
	TALENT_VERSATILITY,
	TALENT_EFFICIENCY,
	TALENT_GENIE_IN_A_BOTTLE,
	TALENT_SORCERERS_SWIFTNESS,
	TALENT_RELATIVITY,
	TALENT_ARCANE_AFFINITY,
	TALENT_HIGH_VOLTAGE,
	TALENT_WISDOM,
	TALENT_INNERVATE,
	TALENT_GROUNDING,
	TALENT_TITANIC,
	TALENT_SPELL_MASTERY,
	TALENT_WANDSLINGER,
	TALENT_NECROMASTERY,
	TALENT_MARCH_OF_THE_DEAD,
	TALENT_UNHOLY_MIGHT,
	TALENT_BONE_COLLECTOR,
	TALENT_SKELETAL_FORTITUDE,
	TALENT_BACKSTAB,
	TALENT_UNDEAD_HORDE,
	TALENT_DESTROYER,
	TALENT_VAMPIRE_LORD,
	TALENT_APOCALYPTIC,
	TALENT_DEATHS_EMBRACE,
	TALENT_ONSLAUGHT,
	TALENT_COLD_BLOODED,
	TALENT_CORPSE_STITCHER,
	TALENT_REANIMATOR,
	TALENT_BONECHILL,
	TALENT_NEGATION,
	TALENT_HOMETOWN_HERO,
	TALENT_SKELEMANCER,
	TALENT_MINDLESS_SUBSERVIENCE
};

enum hero_class_e : uint8_t {
	HERO_KNIGHT,
	HERO_BARBARIAN,
	HERO_NECROMANCER,
	HERO_SORCERESS,
	HERO_WARLOCK,
	HERO_WIZARD,
	HERO_CUSTOM,
	HERO_CLASS_NONE = 16,
	HERO_CLASS_ALL,
	HERO_CLASS_NON_BARBARIAN,
	HERO_CLASS_NON_NECRO
};

enum hero_gender_e : uint8_t {
	HERO_GENDER_MALE,
	HERO_GENDER_FEMALE
};

struct talent_t {
	uint8_t tier;
	hero_class_e hero_class;
	uint8_t asset_id;
	talent_e type;
	std::string name;
	std::string description;
};

struct skill_t {
	uint8_t asset_id;
	skill_e skill_id;
	uint8_t class_learnrate[10] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
	std::string name;
	std::string description;
};

struct secondary_skill_t {
	skill_e skill = SKILL_NONE;
	uint8_t level = 0;
	
	//bool operator==(const secondary_skill_t&) const = default;
};

struct stat_distribution_t {
	uint8_t attack = 25;
	uint8_t defense = 25;
	uint8_t power = 25;
	uint8_t knowledge = 25;
	
	//bool operator==(const stat_distribution_t&) const = default;
};

enum temp_morale_effect_e : uint8_t {
	MORALE_EFFECT_NONE,
	MORALE_EFFECT_WARRIORS_TOMB,
	MORALE_EFFECT_GRAVEYARD,
	MORALE_EFFECT_HOMETOWN_HERO
};

struct temp_morale_effect_t {
	temp_morale_effect_e effect_type = MORALE_EFFECT_NONE;
	int8_t magnitude = 0;
	int8_t duration = -1;
	
	//bool operator==(const temp_morale_effect_t&) const = default;
};

enum temp_luck_effect_e : uint8_t {
	LUCK_EFFECT_NONE,
	LUCK_EFFECT_PYRAMID_VISIT
};

struct temp_luck_effect_t {
	temp_luck_effect_e effect_type = LUCK_EFFECT_NONE;
	int8_t magnitude = 0;
	int8_t duration = -1;
	
	//bool operator==(const temp_luck_effect_t&) const = default;
};

enum hero_class_appearance_e : uint8_t {
	HERO_CLASS_APPEARANCE_KNIGHT,
	HERO_CLASS_APPEARANCE_BARBARIAN,
	HERO_CLASS_APPEARANCE_NECROMANCER,
	HERO_CLASS_APPEARANCE_SORCERESS,
	HERO_CLASS_APPEARANCE_WARLOCK,
	HERO_CLASS_APPEARANCE_WIZARD,
	HERO_CLASS_APPEARANCE_CUSTOM
};

enum hero_body_appearance_e : uint8_t {
	HERO_BODY_HUMAN_M,
	HERO_BODY_HUMAN_F,
	HERO_BODY_ORC_M,
	HERO_BODY_ORC_F,
};

enum hero_face_appearance_e : uint8_t {
	HERO_FACE_APPEARANCE_M_1,
	HERO_FACE_APPEARANCE_M_2,
	HERO_FACE_APPEARANCE_M_3,
	HERO_FACE_APPEARANCE_M_4,
	HERO_FACE_APPEARANCE_M_5,
	HERO_FACE_APPEARANCE_M_6,
	HERO_FACE_APPEARANCE_M_7,
	HERO_FACE_APPEARANCE_M_8,
	HERO_FACE_APPEARANCE_M_9,
	HERO_FACE_APPEARANCE_F_1, //9
	HERO_FACE_APPEARANCE_F_2,
	HERO_FACE_APPEARANCE_F_3,
	HERO_FACE_APPEARANCE_F_4, //12
	HERO_FACE_APPEARANCE_F_5,
	HERO_FACE_APPEARANCE_F_6,
	HERO_FACE_APPEARANCE_F_7, //15
	HERO_FACE_APPEARANCE_F_8,
	HERO_FACE_APPEARANCE_F_9,
	HERO_FACE_APPEARANCE_OM_1
};

enum hero_eyes_appearance_e : uint8_t {
	HERO_EYES_BLUE,
	HERO_EYES_GREEN,
	HERO_EYES_YELLOW,
	HERO_EYES_RED,
	HERO_EYES_BROWN,
	HERO_EYES_BLACK,
	HERO_EYES_GREY,
	HERO_EYES_BLUEGREEN,
	HERO_EYES_BLUE_GLOW,
	HERO_EYES_TEAL_GLOW,
	HERO_EYES_GREEN_GLOW,
	HERO_EYES_YELLOW_GLOW,
	HERO_EYES_ORANGE_GLOW,
	HERO_EYES_RED_GLOW,
	HERO_EYES_LIGHT_RED_GLOW,
	HERO_EYES_BLACKOUT,
	HERO_EYES_WHITEOUT
};

enum hero_beard_appearance_e : uint8_t {
	HERO_BEARD_NONE,
	HERO_BEARD_1,
	HERO_BEARD_2,
	HERO_BEARD_3,
	HERO_BEARD_4,
	HERO_BEARD_5,
	HERO_BEARD_6,
	HERO_BEARD_7
};

enum hero_eyebrow_appearance_e : uint8_t {
	HERO_EYEBROWS_NONE,
	HERO_EYEBROWS_M_1,
	HERO_EYEBROWS_M_2,
	HERO_EYEBROWS_M_3,
	HERO_EYEBROWS_M_4,
	HERO_EYEBROWS_F_1,
	HERO_EYEBROWS_F_2,
	HERO_EYEBROWS_F_3,
	HERO_EYEBROWS_F_4
};

enum hero_ears_appearance_e : uint8_t {
	HERO_EARS_1,
	HERO_EARS_2,
	HERO_EARS_3,
	HERO_EARS_4,
};

enum hero_skin_appearance_e : uint8_t {
	HERO_SKIN_TONE_1,
	HERO_SKIN_TONE_2,
	HERO_SKIN_TONE_3,
	HERO_SKIN_TONE_4,
	HERO_SKIN_TONE_5,
	HERO_SKIN_TONE_6,
	HERO_SKIN_TONE_7,
	HERO_SKIN_TONE_8,
	HERO_SKIN_TONE_9,
};

enum hero_hair_appearance_e : uint8_t {
	HERO_HAIR_BALD,
	HERO_HAIR_H_M_1,
	HERO_HAIR_H_M_2,
	HERO_HAIR_H_M_3,
	HERO_HAIR_H_M_4,
	HERO_HAIR_H_M_5,
	HERO_HAIR_H_F_1, //6
	HERO_HAIR_H_F_2,
	HERO_HAIR_H_F_3,
	HERO_HAIR_H_F_4,
	HERO_HAIR_H_F_5, //10
	HERO_HAIR_H_F_6,
	HERO_HAIR_H_F_7,
	HERO_HAIR_H_F_8,
	HERO_HAIR_H_F_9, //14
	HERO_HAIR_H_F_10,
	HERO_HAIR_H_F_11,
	HERO_HAIR_H_F_12,
	HERO_HAIR_H_F_13
};

enum hero_hair_color_appearance_e : uint8_t {
	HERO_HAIR_COLOR_BALD,
	HERO_HAIR_COLOR_BROWN,
	HERO_HAIR_COLOR_BLACK,
	HERO_HAIR_COLOR_DARK_ORANGE,
	HERO_HAIR_COLOR_WHITE,
	HERO_HAIR_COLOR_GREEN,
	HERO_HAIR_COLOR_ORANGE,
	HERO_HAIR_COLOR_BLONDE,
	HERO_HAIR_COLOR_SILVER,
	HERO_HAIR_COLOR_BLACK_AND_RED,
	HERO_HAIR_COLOR_BLACK_AND_GREEN,
	HERO_HAIR_COLOR_BLACK_AND_BLUE,
	HERO_HAIR_COLOR_PINK_AND_PURPLE,
	HERO_HAIR_COLOR_BLACK_AND_WHITE,
	HERO_HAIR_COLOR_PURPLE,
	HERO_HAIR_COLOR_PURPLE_CHROMATIC,
	HERO_HAIR_COLOR_WHITE_CHROMATIC
};

enum hero_mount_type_e : uint8_t {
	HERO_MOUNT_DEFAULT_HORSE,
	HERO_MOUNT_DINOSAUR,
	HERO_MOUNT_ZEBRA,
	HERO_MOUNT_RHINO,
	HERO_MOUNT_STAG
};

enum hero_pet_type_e : uint8_t {
	HERO_PET_NONE,
	HERO_PET_FOX,
	HERO_PET_PIG,
	HERO_PET_CHICKEN,
	HERO_PET_CROW
};

struct hero_appearance_t {
	hero_class_appearance_e class_appearance = HERO_CLASS_APPEARANCE_KNIGHT;
	hero_body_appearance_e body;
	hero_face_appearance_e face;
	hero_eyes_appearance_e eye_color;
	hero_beard_appearance_e beard;
	hero_eyebrow_appearance_e eyebrows;
	hero_ears_appearance_e ears;
	hero_skin_appearance_e skin;
	hero_hair_appearance_e hair;
	hero_hair_color_appearance_e hair_color;
	hero_mount_type_e mount_type = HERO_MOUNT_DEFAULT_HORSE;
	hero_pet_type_e pet_type = HERO_PET_NONE;
};

std::string get_temp_morale_effect_string(temp_morale_effect_e type, int magnitude);
std::string get_temp_luck_effect_string(temp_luck_effect_e type, int magnitude);

using default_army_t = std::array<std::tuple<unit_type_e, uint8_t, uint8_t>, 3>;

struct hero_t {
	uint16_t id = 0;
	player_e player = PLAYER_NONE;
	//current adventure map position
	uint8_t x = 0;
	uint8_t y = 0;
	int16_t movement_points = 1500;
	int16_t maximum_daily_movement_points = 1500;

	uint8_t start_of_day_x = 0;
	uint8_t start_of_day_y = 0;
	uint8_t wormhole_casts_today = 0;
	uint8_t mana_battery_casts_today = 0;
	
	bool garrisoned = false;
	bool in_town = false;
	bool is_sleeping = false;
	
	bool has_ballista = false;
	std::pair<int, int> get_ballista_damage_range() const;
	int get_ballista_shot_count() const;
	bool get_ballista_manual_control() const;
	
	//todo: fix this so we can serialize boarded_ship
	bool is_on_ship() const { return boarded_ship != nullptr; }
	interactable_object_t* boarded_ship = nullptr;
	
	uint8_t portrait = 0;
	std::string name;
	std::string title;
	std::string biography;
	std::string race;
	hero_gender_e gender = HERO_GENDER_MALE;
	hero_appearance_t appereance;
	const std::string get_title() const;
	const std::string get_class_name() const;
	static const std::string get_class_name(hero_class_e hero_class);
	
	static const std::string get_secondary_skill_level_name(int level);
	
	hero_class_e hero_class = HERO_KNIGHT;
	hero_specialty_e specialty = SPECIALTY_UNKNOWN;
	bool uncontrolled_by_human = false;
	std::array<secondary_skill_t, game_config::HERO_SKILL_SLOTS> skills;
	std::array<artifact_e, game_config::HERO_ARTIFACT_SLOTS> artifacts = { ARTIFACT_NONE };
	std::array<artifact_e, game_config::HERO_BACKPACK_SLOTS> backpack = { ARTIFACT_NONE };
	troop_group_t troops;

	uint8_t level = 1;
	uint64_t experience = 0;
	
	static uint64_t get_xp_to_level(uint level);
	uint64_t get_xp_to_next_level() const;
	std::array<skill_e, 3> get_level_up_skills() const;
	stat_e level_up_stats();
	uint get_number_of_open_skill_slots() const;
	bool can_learn_skill(skill_e skill) const;
	
	uint8_t attack = 0;
	uint8_t defense = 0;
	uint8_t power = 1; //'command' for barbarians
	uint8_t knowledge = 1; //'stamina' for barbarians
	
	uint16_t dark_energy = 0;
	uint16_t get_maximum_dark_energy() const;
	
	void increase_attack(uint8_t amount);
	void increase_defense(uint8_t amount);
	void increase_power(uint8_t amount);
	void increase_knowledge(uint8_t amount);

	uint8_t get_effective_attack(bool ignore_percentage_reductions = false) const;
	uint8_t get_effective_defense(bool ignore_percentage_reductions = false) const;
	uint8_t get_effective_power() const;
	uint8_t get_effective_knowledge() const;
	
	uint get_unit_magic_resistance(unit_type_e unit_type = UNIT_UNKNOWN, magic_damage_e damage_type = MAGIC_DAMAGE_ALL) const;
	uint get_unit_min_damage(unit_type_e unit_type) const;
	uint get_unit_max_damage(unit_type_e unit_type) const;
	uint get_unit_max_hp(unit_type_e unit_type) const;
	uint get_unit_attack(unit_type_e unit_type) const;
	uint get_unit_defense(unit_type_e unit_type) const;
	uint get_unit_speed(unit_type_e unit_type) const;
	uint get_unit_initiative(unit_type_e unit_type) const;
	int get_unit_morale(unit_type_e unit_type) const;

	int get_slowest_unit_speed_in_army() const;
	int get_number_of_troop_alignments() const;
	bool has_undead_in_army() const;
	bool has_no_troops_in_army() const;
	
	int get_artifact_effect_bonus_value(artifact_effect_e artifact_effect) const;
	float get_spell_effect_multiplier(spell_e spell) const;
	float get_spell_haste_percentage() const;

	bool add_temporary_morale_effect(temp_morale_effect_e type, int8_t magnitude, int8_t duration = -1);
	bool add_temporary_luck_effect(temp_luck_effect_e type, int8_t magnitude, int8_t duration = -1);
	std::vector<temp_morale_effect_t> temp_morale_effects;
	std::vector<temp_luck_effect_t> temp_luck_effects;

	int get_luck() const;
	int get_morale() const;
	
	//talents
	std::array<talent_e, game_config::HERO_TALENT_POINTS> talents = { TALENT_NONE };
	std::array<talent_e, game_config::HERO_TALENT_SLOTS> available_talents = { TALENT_NONE };
	
	bool has_talent(talent_e talent) const;
	bool has_talent_available(talent_e talent) const;
	bool is_talent_unlocked(talent_e talent_id) const;
	bool is_talent_position_unlocked(int position) const;
	bool give_talent(talent_e talent);
	uint talent_points_spent_count() const;
	bool has_talent_point_on_level_up() const;
	void init_hero(int day, terrain_type_e starting_terrain);
	void init_talents();
	void init_army();
	
	//ui 'fixup' hack
	void move_necromancy_to_top_slot();
	
	int get_megalomania_artifact_count() const;
	uint8_t outlaw_bonus = 0;
	
	bool is_artifact_equipped(artifact_e artifact) const;
	bool is_artifact_in_effect(artifact_e artifact) const;
	bool move_artifacts_in_backpack(uint from_slot, uint to_slot);
	bool move_artifact_from_backpack_to_slot(uint from_slot, uint to_slot);
	bool move_artifact_to_backpack_from_slot(uint from_slot);
	bool move_artifact_to_backpack_slot_from_slot(uint from_slot, uint to_slot);
	bool move_artifacts_in_paperdoll_slot(uint from_slot, uint to_slot);
	bool pickup_artifact(artifact_e artifact_id);
	bool is_inventory_full(artifact_e artifact_id) const;
	bool does_artifact_fit_in_slot(artifact_slot_e artifact_slot, uint hero_slot) const;
	bool is_backpack_full() const;
	uint16_t calculate_maximum_daily_movement_points(int day, terrain_type_e starting_terrain) const;
	float get_movement_percent_remaining() const { return (float)movement_points / maximum_daily_movement_points; }

	uint get_total_attack_bonus() const;
	uint get_total_defense_bonus() const;

	uint get_total_attack_bonus_multiplier() const;
	uint get_total_defense_bonus_multiplier() const;

	//adjusted spell power boosted by troops in hero's army
	uint get_adjusted_power(); // { return power * troops(); }
	
	int get_secondary_skill_level(skill_e skill) const;
	int get_secondary_skill_base_level(skill_e skill) const;
	bool are_secondary_skills_full_and_max_level() const;
	
	void new_day(int day, terrain_type_e starting_terrain);
	
	const static int base_scouting_radius = 5;
	int get_scouting_radius() const;
	const static int base_observation_radius = 7;
	int get_observation_radius() const;
	

	uint16_t mana = 10; //rage for barbarians
	float get_mana_percentage_remaining() const;
	uint16_t get_maximum_mana() const;
	uint16_t get_daily_mana_regen() const;
	//bool can_learn_spells_outside_class_restrictions = false;
	static std::vector<spell_school_e> get_allowed_schools(hero_class_e hero_class);
	static std::vector<hero_specialty_e> get_allowed_specialties(hero_class_e hero_class);
	float get_specialty_multiplier() const;
	
	std::vector<spell_e> spellbook;
	uint get_spell_count_by_school(spell_school_e school = SCHOOL_ALL, spell_type_e type_filter = SPELL_TYPE_ALL) const;
	bool can_learn_spell(spell_e spell_id) const;
	bool knows_spell(spell_e spell_id) const;
	bool learn_spell(spell_e spell_id);
	bool can_learn_spell_type(int spell_level, spell_school_e school) const;
	uint16_t get_spell_cost(spell_e spell_id) const;
	
	std::vector<talent_e> guaranteed_talents;
	default_army_t starting_army;
	
	stat_distribution_t stat_distribution_pre10;
	stat_distribution_t stat_distribution_post10;
	const stat_distribution_t& get_current_stat_distribution() const;
	static stat_distribution_t get_stat_distribution_for_class(hero_class_e hero_class, int level);
	
	std::vector<script_t> scripts;
	
	void set_visited_object(interactable_object_t* object, bool visited = true);
	bool has_object_been_visited(interactable_object_t* object) const;
	
	std::vector<uint32_t> visited_objects;
	std::vector<skill_e> enabled_skills;
};

//#ifdef BUILD_WITH_QT
#include "core/qt_headers.h"

QDataStream& operator<<(QDataStream& stream, const hero_t& hero);
QDataStream& operator>>(QDataStream& stream, hero_t& hero);

QDataStream& operator<<(QDataStream& stream, const hero_appearance_t& appearance);
QDataStream& operator>>(QDataStream& stream, hero_appearance_t& appearance);

//#endif

inline std::vector<spell_school_e> hero_t::get_allowed_schools(hero_class_e hero_class) {
	std::vector<spell_school_e> result;
	switch (hero_class) {
		case HERO_KNIGHT:
        result.push_back(SCHOOL_HOLY);
		break;
	case HERO_BARBARIAN:
		break;
	case HERO_NECROMANCER:
        result.push_back(SCHOOL_DEATH);
        result.push_back(SCHOOL_DESTRUCTION);
		break;
	case HERO_SORCERESS:
        result.push_back(SCHOOL_NATURE);
        result.push_back(SCHOOL_ARCANE);
        result.push_back(SCHOOL_HOLY);
		break;
	case HERO_WARLOCK:
        result.push_back(SCHOOL_DESTRUCTION);
        result.push_back(SCHOOL_DEATH);
		break;
	case HERO_WIZARD:
        result.push_back(SCHOOL_ARCANE);
        result.push_back(SCHOOL_NATURE);
        result.push_back(SCHOOL_HOLY);
		break;
	case HERO_CUSTOM:
	default:
		break;
	}
	return result;
}

inline std::vector<hero_specialty_e> hero_t::get_allowed_specialties(hero_class_e hero_class) {
	std::vector<hero_specialty_e> result;
	switch(hero_class) {
		case HERO_KNIGHT:
			result.push_back(SPECIALTY_LOGISTICS);
			result.push_back(SPECIALTY_PATHFINDING);
			break;
		case HERO_BARBARIAN:
			result.push_back(SPECIALTY_LOGISTICS);
			result.push_back(SPECIALTY_PATHFINDING);
			break;
		case HERO_NECROMANCER:
			result.push_back(SPECIALTY_INTELLIGENCE);
			result.push_back(SPECIALTY_SORCERY);
			result.push_back(SPECIALTY_SPELL_PENETRATION);
			break;
		case HERO_SORCERESS:
			result.push_back(SPECIALTY_INTELLIGENCE);
			result.push_back(SPECIALTY_SORCERY);
			result.push_back(SPECIALTY_SPELL_PENETRATION);
			result.push_back(SPECIALTY_LOGISTICS);
			result.push_back(SPECIALTY_PATHFINDING);
			break;
		case HERO_WARLOCK:
			result.push_back(SPECIALTY_INTELLIGENCE);
			result.push_back(SPECIALTY_SORCERY);
			result.push_back(SPECIALTY_SPELL_PENETRATION);
			break;
		case HERO_WIZARD:
			result.push_back(SPECIALTY_INTELLIGENCE);
			result.push_back(SPECIALTY_SORCERY);
			result.push_back(SPECIALTY_SPELL_PENETRATION);
			break;
		case HERO_CUSTOM:
		default:
			break;
	}

	return result;
}

inline bool is_default_troop_army_empty(const default_army_t& troops) {
	for(const auto& tr : troops) {
		if(std::get<0>(tr) != UNIT_UNKNOWN)
			return false;
	}
	
	return true;
}
