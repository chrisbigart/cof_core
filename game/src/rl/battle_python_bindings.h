#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct game_t;
struct hero_t;
struct battlefield_t;

enum {
        RL_COMBAT_MAX_ARMY_TROOPS = 16,
};

typedef struct rl_combat_game_handle rl_combat_game_handle;
typedef struct rl_combat_session_handle rl_combat_session_handle;

typedef struct rl_combat_troop_stack_c {
        uint16_t unit_type;
        uint16_t stack_size;
} rl_combat_troop_stack_c;

typedef struct rl_combat_stack_observation_c {
        uint16_t unit_type;
        uint16_t stack_size;
        uint16_t unit_health;
        uint16_t original_stack_size;
        int8_t retaliations_remaining;
        int8_t x;
        int8_t y;
        uint8_t is_attacker;
        uint8_t is_alive;
        uint8_t has_waited;
        uint8_t has_moved;
        uint8_t has_defended;
        uint8_t has_cast_spell;
        uint8_t has_moraled;
        uint8_t is_disabled;
} rl_combat_stack_observation_c;

typedef struct rl_combat_hero_loadout_c {
        uint8_t player;
        uint8_t hero_class;
        uint16_t hero_id;
        const char* name;
        uint8_t level;
        uint64_t experience;
        uint8_t attack;
        uint8_t defense;
        uint8_t power;
        uint8_t knowledge;
        uint16_t mana;
        uint16_t dark_energy;
        uint8_t has_ballista;
        uint8_t human_controlled;
        const rl_combat_troop_stack_c* troops;
        size_t troop_count;
        const uint8_t* talents;
        size_t talent_count;
        const uint16_t* spells;
        size_t spell_count;
        const uint8_t* skills;
        size_t skill_count;
} rl_combat_hero_loadout_c;

typedef struct rl_combat_scenario_c {
        rl_combat_hero_loadout_c attacker;
        rl_combat_hero_loadout_c defender;
        uint8_t is_deathmatch;
        uint8_t environment;
        uint8_t quick_combat;
} rl_combat_scenario_c;

typedef struct rl_combat_observation_c {
        uint16_t round;
        uint8_t attacker_moved_last;
        uint8_t is_siege;
        uint8_t is_quick_combat;
        int8_t active_unit_id;
        uint8_t active_unit_is_attacker;
        int8_t active_unit_x;
        int8_t active_unit_y;
        uint16_t attacker_mana;
        uint16_t defender_mana;
        int16_t attacker_morale;
        int16_t defender_morale;
        int16_t attacker_luck;
        int16_t defender_luck;
        uint8_t attacker_stack_count;
        uint8_t defender_stack_count;
        rl_combat_stack_observation_c attacker_stacks[RL_COMBAT_MAX_ARMY_TROOPS];
        rl_combat_stack_observation_c defender_stacks[RL_COMBAT_MAX_ARMY_TROOPS];
} rl_combat_observation_c;

typedef enum rl_combat_action_type_e {
        RL_COMBAT_ACTION_WAIT = 0,
        RL_COMBAT_ACTION_DEFEND = 1,
        RL_COMBAT_ACTION_AUTO_RESOLVE = 2,
} rl_combat_action_type_e;

typedef struct rl_combat_action_c {
        uint8_t type;
} rl_combat_action_c;

rl_combat_game_handle* rl_combat_create_game();
void rl_combat_destroy_game(rl_combat_game_handle* handle);

rl_combat_session_handle* rl_combat_create_session(rl_combat_game_handle* game_handle);
void rl_combat_destroy_session(rl_combat_session_handle* session_handle);

int rl_combat_session_configure(rl_combat_session_handle* session_handle, const rl_combat_scenario_c* scenario);
int rl_combat_session_reset(rl_combat_session_handle* session_handle, int* out_result);
int rl_combat_session_step(rl_combat_session_handle* session_handle, int* out_result);
int rl_combat_session_apply_action(rl_combat_session_handle* session_handle, const rl_combat_action_c* action);

int rl_combat_session_observation(const rl_combat_session_handle* session_handle, rl_combat_observation_c* out_observation);

const battlefield_t* rl_combat_session_battlefield(const rl_combat_session_handle* session_handle);
const hero_t* rl_combat_session_attacker(const rl_combat_session_handle* session_handle);
const hero_t* rl_combat_session_defender(const rl_combat_session_handle* session_handle);
const game_t* rl_combat_session_game(const rl_combat_session_handle* session_handle);

#ifdef __cplusplus
} // extern "C"
#endif

