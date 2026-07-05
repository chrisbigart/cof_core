#include "core/game.h"
#include "core/game_config.h"

const std::vector<unit_type_e> possible_units = { UNIT_PIXIE, UNIT_BISHOP, UNIT_ARCHER, UNIT_MINOTAUR };
const std::vector<spell_e> known_spells = { SPELL_UNKNOWN, SPELL_BLESS, SPELL_CURSE, SPELL_AIR_SHIELD, SPELL_LIGHTNING_BOLT, SPELL_HASTE};
const int troop_count = 4;
const int evaluation_iterations = 100;

const std::string config_path = "C:\\code\\ConquerorsOfFate\\Source\\ConquerorsOfFate\\homm\\";

int main() {
	game_config::load_game_data(config_path);

	game_t game;
	//setup_callbacks(game);

	int iter = 0;
	while(true) {
		iter++;
		battlefield_t battle;

		hero_t hero = game_config::get_heroes()[0];
		hero.attack = 0;
		hero.defense = 0;
		hero.power = 4;
		hero.knowledge = 4;
		hero.init_hero(0, TERRAIN_GRASS);
		for(const auto& s : known_spells)
			hero.learn_spell(s);

		int total_army_hp = 0;
		for(int i = 0; i < troop_count; i++) {
			auto& tr = hero.troops[i];
			tr.unit_type = utils::rand_item(possible_units);
			tr.stack_size = 1 + rand() % 15;
			const auto& cr = game_config::get_creature(tr.unit_type);
			total_army_hp += (cr.health * tr.stack_size);
		}

		map_monster_t monster;
		monster.unit_type = utils::rand_item(possible_units);
		const auto& cr = game_config::get_creature(monster.unit_type);
		monster.quantity = 1 + (total_army_hp / cr.health);

		for(const auto& selected_spell : known_spells) {
			int hero_victories = 0;
			for(int i = 0; i < evaluation_iterations; i++) {
				auto hero_copy = hero;
				auto monster_copy = monster;
				auto battle_copy = battle;
				srand((iter * 1000) + i);
				battle_copy.init_hero_monster_battle(&hero_copy, &monster_copy);
				battle_copy.is_quick_combat = true;
				//while(!battle_copy.is_attackers_turn())
				//	battle_copy.auto_move_troop();

				if(selected_spell != SPELL_UNKNOWN) {
					battlefield_unit_t* target = nullptr;
					for(auto& atr : battle_copy.attacking_army.troops) {
						if(battle_copy.is_spell_target_valid(&hero_copy, &atr, selected_spell)) {
							assert(selected_spell != SPELL_CURSE);
							target = &atr;
							break;
						}
					}
					if(!target) {
						for(auto& dtr : battle_copy.defending_army.troops) {
							if(battle_copy.is_spell_target_valid(&hero_copy, &dtr, selected_spell)) {
								assert(selected_spell != SPELL_HASTE);
								target = &dtr;
								break;
							}
						}
					}

					battle_copy.cast_spell(&hero_copy, selected_spell, -1, -1, target);
				}
		
				auto result = battle_copy.compute_quick_combat();
				if(result == BATTLE_ATTACKER_VICTORY)// && result != BATTLE_DEFENDER_VICTORY)
					hero_victories++;
			}

			std::printf("win rate using spell[%s]: %.2f\n", magic_enum::enum_name(selected_spell).data(), 100.f * (float)hero_victories / evaluation_iterations);
		}
	}
}