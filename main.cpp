#include <iostream>

#include "core/adventure_map.h"
#include "core/game.h"
#include "core/map_file.h"

void game_state_update(player_e player) {
	// Implementation will go here
}

void ai_turn_completion(player_e player) {
	// Implementation will go here
}

void ai_turn_progress(player_e player, int progress) {
	// Implementation will go here
}

void remove_hero(hero_t* hero, bool permanent) {
	// Implementation will go here
}

void remove_interactable_object(interactable_object_t* obj, bool permanent) {
	// Implementation will go here
}

void update_interactable_object(interactable_object_t* obj) {
	// Implementation will go here
}

void show_hero_levelup(hero_t* hero, skill_e primary, skill_e secondary,
					   skill_e tertiary, stat_e stat, bool can_select) {
	 // Implementation will go here
}

void show_dialog(dialog_type_e type, interactable_object_t* obj,
				 hero_t* hero, int param1, int param2) {
	 // Implementation will go here
}

void combat_action(const battle_action_t& action) {
	 // Implementation will go here
}

// Assigning the callbacks using std::bind
void setup_callbacks(game_t& game) {
	using namespace std::placeholders;

	game.game_state_update_callback_fn = std::bind(&game_state_update, _1);
	game.ai_turn_completion_callback_fn = std::bind(&ai_turn_completion, _1);
	game.ai_turn_progress_callback_fn = std::bind(&ai_turn_progress, _1, _2);
	game.remove_hero_callback_fn = std::bind(&remove_hero, _1, _2);
	game.remove_interactable_object_callback_fn = std::bind(&remove_interactable_object, _1, _2);
	game.update_interactable_object_callback_fn = std::bind(&update_interactable_object, _1);
	game.show_hero_levelup_callback_fn = std::bind(&show_hero_levelup, _1, _2, _3, _4, _5, _6);
	game.show_dialog_callback_fn = std::bind(&show_dialog, _1, _2, _3, _4, _5);
	
	game.battle.fn_emit_combat_action = std::bind(&combat_action, _1);
}

const std::string map_filename = "medium_one.cofmap";

int main() {
	game_config::load_game_data();

	game_t game;
	setup_callbacks(game);

	int iter = 0;
	int max_iters = 1000;

	while(iter++ < max_iters) {
		hero_t hero = game_config::get_heroes()[rand() % game_config::get_heroes().size()];
		hero.init_hero(0, TERRAIN_GRASS);

		int fill_chance = rand() % 100;
		for(int i = 0; i < game_config::HERO_TROOP_SLOTS; i++) {
			auto& tr = hero.troops[i];

			if(!utils::rand_chance(fill_chance)) {
				tr.clear();
				continue;
			}
			
			tr.unit_type = (unit_type_e)((UNIT_UNKNOWN + 1) + (rand() % UNIT_TITAN));
			tr.stack_size = 1 + rand() % 15;
		}

		if(hero.has_no_troops_in_army())
			continue;

		map_monster_t monster;
		monster.unit_type = (unit_type_e)((UNIT_UNKNOWN + 1) + (rand() % UNIT_TITAN));
		monster.quantity = 1 + rand() % 50;

		game.battle.init_hero_monster_battle(&hero, &monster);
		
		auto result = game.battle.compute_quick_combat();
		if(result != BATTLE_ATTACKER_VICTORY && result != BATTLE_DEFENDER_VICTORY)
			continue;
	}
}
