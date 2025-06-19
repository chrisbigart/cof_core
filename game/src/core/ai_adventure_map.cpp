#include "core/game.h"
#include "core/adventure_map.h"

#include <thread>

enum hero_role_e {
	ROLE_SCOUT,
	ROLE_MAIN,
	ROLE_SECONDARY
};

//returns a value that indicates how strong the hero is as a fighter/main,
//based on secondary skills + talents
int evaluate_hero_warrior(const hero_t* hero) {
	int score = 0;

	score += hero->get_secondary_skill_level(SKILL_OFFENSE) * 15;
	score += hero->get_secondary_skill_level(SKILL_DEFENSE) * 12;
	score += hero->get_secondary_skill_level(SKILL_ARCHERY) * 10;
	score += hero->get_secondary_skill_level(SKILL_LEADERSHIP) * 10;
	score += hero->get_secondary_skill_level(SKILL_LUCK) * 10;

	return score;
}

//returns a value that indicates how strong the hero is as a scout,
//based on secondary skills + talents
int evaluate_hero_scout(const hero_t* hero) {
	int score = 0;

	score += hero->maximum_daily_movement_points / 10;// 1500 ? 150 pts
	score += hero->get_scouting_radius() * 20;
	score += hero->get_secondary_skill_level(SKILL_LOGISTICS) * 25;
	score += hero->get_secondary_skill_level(SKILL_PATHFINDING) * 20;
	score += hero->get_secondary_skill_level(SKILL_SCOUTING) * 15;

	return score;
}

int get_army_strength_value(const troop_group_t& units) {
	int total = 0;
	for(const auto& u : units) {
		if(u.is_empty())
			continue;

		const auto& creature = game_config::get_creature(u.unit_type);
		total += creature.health * u.stack_size;
	}

	return total;
}

//returns a value indicating how effective a hero will be in a fight,
//which includes stats
int evaluate_fighting_strength(const hero_t* hero) {
	int score = evaluate_hero_warrior(hero);

	score += hero->get_effective_attack() * 10;
	score += hero->get_effective_defense() * 7;
	score += hero->get_effective_power() * 6;
	score += hero->get_effective_knowledge() * 5;

	return score;
}

int get_hero_plus_army_strength(const hero_t* hero) {
	float multiplier = std::clamp(evaluate_fighting_strength(hero) / 100.f, 1.f, 10.f);

	return get_army_strength_value(hero->troops) * multiplier;
}

int calculate_monster_strength(const map_monster_t* monster) {
	const auto& creature = game_config::get_creature(monster->unit_type);

	return monster->quantity * creature.health;
}

bool ai_can_defeat_monster(const hero_t* hero, const map_monster_t* monster_stack) {
	auto hero_strength = get_hero_plus_army_strength(hero);
	auto monster_strength = calculate_monster_strength(monster_stack);

	const float required_advantage_ratio = 1.2f; // AI wants to be 20% stronger

	return hero_strength >= (monster_strength * required_advantage_ratio);
}

hero_role_e classify_hero(const hero_t* hero) {
	return ROLE_MAIN;
////////////////////
	const int war = evaluate_hero_warrior(hero);
	const int scout = evaluate_hero_scout(hero);

	if(war > scout * 3)
		return ROLE_MAIN;

	if(scout > war * 2)
	   return ROLE_SCOUT;
	
	return ROLE_SECONDARY;
}

enum ai_goal_e {
	GOAL_NONE,
	GOAL_RECRUIT_HERO,
	GOAL_RECRUIT_TROOPS,
	GOAL_BUILD_TAVERN,
	GOAL_BUILD_BUILDINGS,
	GOAL_EXPLORE,
	GOAL_CAPTURE_OBJECT,
	GOAL_ATTACK_MONSTER
};

struct ai_goal_t {
	ai_goal_e goal = GOAL_NONE;
	int priority = 0;
	interactable_object_t* target_object = nullptr;
	map_monster_t* target_guard = nullptr;
	coord_t target_coord = { -1, -1 };
};


int get_ai_value_for_object(interactable_object_t* object, const hero_t* hero, const game_t* game) {
	int route_cost = game->map.get_route_cost_to_tile(hero, object->x, object->y, game);
	if(route_cost == -1) //we can't reach this object
		return 0;

	float travel_cost = (route_cost / 100.f);

	switch(object->object_type) {
		case OBJECT_RESOURCE: {
			auto res = (map_resource_t*)object;
			int value = res->resource_type == RESOURCE_GOLD ? 100 : (is_basic_resource(res->resource_type) ? 50 : 65);
			return value / travel_cost;
		}
		case OBJECT_TREASURE_CHEST: {
			return 70 / travel_cost;
		}
		case OBJECT_ARTIFACT: {
			auto art = (map_artifact_t*)object;
			const auto& ainfo = game_config::get_artifact(art->artifact_id);
			return (ainfo.rarity * 50) / travel_cost;
		}
		case OBJECT_MINE: {
			auto mine = (mine_t*)object;
			if(mine->owner == hero->player) //we already have this mine flagged
				return 0;

			int value = mine->mine_type == RESOURCE_GOLD ? 500 : (is_basic_resource(mine->mine_type) ? 200 : 350);
			//if we have no wood/ore income, bump up the value of capturing these mines
			//todo
			return value / travel_cost;
		}
		default:
			return 0;
	}
	
	//return 0;
}


building_e game_t::ai_determine_which_building_to_build(town_t* town, bool is_main_town) {
	if(!town || town->has_built_today)
		return BUILDING_NONE;

	std::vector<building_e> candidates;

	// Pass 1: filter buildings we can actually build right now
	for(building_e b : town->available_buildings) {
		if(town->is_building_built(b) || !town->can_build_building(b, *this))
			continue;

		candidates.push_back(b);
	}

	if(candidates.empty())
		return BUILDING_NONE;

	const auto is_dwelling = [](const building_t& b) {
		return b.weekly_growth > 0;
	};

	const auto is_mage_guild = [](building_e b) {
		return b >= BUILDING_MAGE_GUILD_1 && b <= BUILDING_MAGE_GUILD_5;
	};

	building_e best = BUILDING_NONE;
	int best_score = -999;

	for(building_e b : candidates) {
		const building_t& def = game_config::get_building(b);
		int score = 0;

		// dwellings = high priority in main towns
		if(is_dwelling(def)) {
			score += 50;
			score += def.weekly_growth * 2;
			score += (int)def.generated_creature * 1; // crude tier weighting
			if(!is_main_town)
				score -= 40; // deprioritize in non-main towns
		}

		// mage guilds: escalate
		if(is_mage_guild(b)) {
			int current_level = town->get_mage_guild_level();
			int target_level = b - BUILDING_MAGE_GUILD_1 + 1;
			if(target_level == current_level + 1)
				score += 40 + target_level * 5;
		}

		// economy & utility
		if(b == BUILDING_MARKETPLACE) score += 30;
		if(b == BUILDING_TAVERN)      score += 25;
		if(b == BUILDING_BLACKSMITH)  score += 15;
		if(b == BUILDING_SHIPYARD)    score += 10;

		// special buildings (up to you to define more logic later)
		if(def.subtype >= BUILDING_BASE_TYPE_SPECIAL1)
			score += 10;

		if(score > best_score) {
			best_score = score;
			best = b;
		}
	}

	return best_score > 0 ? best : BUILDING_NONE;
}


void game_t::process_all_ai_turns() {
	auto start_date = get_date();
	for(uint i = 0; i < game_configuration.player_count; i++) {
		auto player_num = (player_e)(i + 1);
		if(get_player(player_num).is_human) {
			if(get_player(player_num).has_completed_turn) {
				continue;
			}
			else {
				return;
			}
		}
		
		process_ai_turn(player_num);
		//std::cout << "calling ai_turn_complete callback [" << (int)player_num << "]\n";
		ai_turn_completion_callback_fn(player_num);
		
	}
	
	//std::cout << "calling ai_turn_complete callback [all players]\n";
	std::cout << "--All AI turns complete for day " << start_date << "--\n";
	ai_turn_completion_callback_fn(PLAYER_NONE);
}

std::vector<ai_goal_t> ai_player_goals;
std::unordered_map<uint16_t, hero_role_e> hero_roles;

struct hero_plan_t {
	hero_t* hero = nullptr;
	interactable_object_t* target = nullptr;
	route_t route;
};



interactable_object_t* find_best_object(const hero_t& hero, const adventure_map_t& map, const game_t& game) {
	interactable_object_t* valuable_object = nullptr;
	//find the highest value object in a NxN radius
	int radius = 7;
	interactable_object_t* valuable_obj = nullptr;
	int highest_value = 0;
		
	for(int y = hero.y - radius; y < hero.y + radius; y++) {
		for(int x = hero.x - radius; x < hero.x + radius; x++) {
			if(!map.tile_valid(x, y))
				continue;
				
			auto obj = map.get_interactable_object_for_tile(x, y);
			if(!obj)
				continue;
				
			auto value = get_ai_value_for_object(obj, &hero, &game);
			if(value > highest_value) {
				highest_value = value;
				valuable_obj = obj;
			}
		}
	}

	return valuable_obj;
}

bool game_t::ai_should_recruit_troops(const hero_t* hero, const town_t* town) const {
	if(!town)
		return false;

	auto player = hero ? hero->player : town->player;

	bool should_buy = false;
	int slot = 0;
	for(auto& tr : town->available_troops) {
		int afford_count = tr.stack_size;
		if(afford_count)
			should_buy = true;
	}

	return should_buy;
}

bool game_t::ai_recruit_troops(hero_t* hero, town_t* town) {
	if(!town)
		return false;

	auto player = hero ? hero->player : town->player;

	int slot = 0;
	for(auto& tr : town->available_troops) {
		int afford_count = tr.stack_size;
		bool did_buy = buy_troops_at_town(player, town, slot, afford_count);
		if(did_buy) {

			std::cout << "[" << magic_enum::enum_name(player) << "] recruited troops at town '"
				<< town->name << "' (" << town->x << ", " << town->y << "): "
				<< tr.get_unit_name() << " - " << afford_count << "x." << std::endl;
		}
		slot++;
	}

	//for(int i = 0; i < game_config::HERO_TROOP_SLOTS; i++) {
	if(hero && town->visiting_hero == hero)
		map.move_all_troops_from_garrison_to_visiting_hero(town);
	//}

	return true;
}

bool game_t::ai_pickup_object(hero_t* hero, interactable_object_t* object) {
	replay_action_t pickup_action;
	pickup_action.tile_x = object->x;
	pickup_action.tile_y = object->y;
	pickup_action.hero = hero;
	pickup_action.action = ACTION_PICKUP_ITEM;
	pickup_action.target_object_id = map.get_object_id(object);

	bool picked_up_item = false;
	auto player_num = hero->player;

	switch(object->object_type) {
		case OBJECT_RESOURCE: {
			auto res = (map_resource_t*)object;
			resource_group_t rg;
			rg.set_value_for_type(res->resource_type, res->min_quantity);
			get_player(hero->player).resources += rg; //fixme
				
			pickup_action.object_type = OBJECT_RESOURCE;
			pickup_action.target_object_type.res_type = res->resource_type;
			//action.target.object = valuable_obj; //problem: we can't reference this as it is about to be deleted
			//remove_interactable_object(valuable_obj);
			map.remove_interactable_object(object);
			picked_up_item = true;

			break;
		}
		case OBJECT_TREASURE_CHEST: {
			auto chest = (treasure_chest_t*)object;
			bool take_gold = (chest->value == 1 || hero_roles[hero->id] == ROLE_SCOUT);
					
			if(take_gold) {
				resource_group_t rg;
				rg.set_value_for_type(RESOURCE_GOLD, chest->value * 500);
				get_player(hero->player).resources += rg; //fixme
			}
			else {
				//todo: give hero xp, make level up choices
			}

			pickup_action.object_type = OBJECT_TREASURE_CHEST;
			//action.target.object = valuable_obj; //problem: we can't reference this as it is about to be deleted
			//remove_interactable_object(valuable_obj);
			map.remove_interactable_object(object);
			picked_up_item = true;

			break;
		}
		case OBJECT_ARTIFACT: {
			auto art = (map_artifact_t*)object;
			hero->pickup_artifact(art->artifact_id);
			pickup_action.object_type = OBJECT_ARTIFACT;
			pickup_action.target_object_type.art_type = art->artifact_id;
			pickup_action.target_object_id = map.get_object_id(art);
			//action.target.object = valuable_obj; //problem: we can't reference this as it is about to be deleted
			//remove_interactable_object(valuable_obj);
			map.remove_interactable_object(object);
			picked_up_item = true;

			break;
		}
	}

	if(picked_up_item) {
		last_turn_actions[player_num].push_back(pickup_action);
	}

	return picked_up_item;
}

bool game_t::ai_move_hero_to_object(hero_t* hero, interactable_object_t* object) {
	if(!hero || !object)
		return false;

	auto player_num = hero->player;
	auto route = map.get_route(hero, object->x, object->y, this);
	
	int starting_mp = hero->movement_points;
	for(auto step = route.begin(); step != route.end(); step++) {
		bool picked_up_item = false;
		if(step->total_cost > starting_mp)
			return false;

		if(route.size() == 1 || (step == route.end() - 1)) {
			picked_up_item = ai_pickup_object(hero, object);
			if(picked_up_item)
				return true;
		}
				
		if(!picked_up_item) {
			replay_action_t movement_action;
			movement_action.action = ACTION_MOVE_TO_TILE;
			movement_action.hero = hero;
			movement_action.tile_x = step->tile.x;
			movement_action.tile_y = step->tile.y;
				
			last_turn_actions[player_num].push_back(movement_action);
			map.move_hero_to_tile(*hero, step->tile.x, step->tile.y, *this);
			auto obj = map.get_interactable_object_for_tile(step->tile.x, step->tile.y);
			//map.interact_with_object(&hero, obj, *this);
		}
	}

	return true;
}

// Find the best *actionable* target for a hero
// Considers object value, guards, and reachability
ai_goal_t find_best_hero_goal(hero_t& hero, const adventure_map_t& map, const game_t& game) {
	ai_goal_t best_goal;
	best_goal.priority = -1;

	//first, are we in a town?  if so, determine if we should recruit troops
	if(hero.in_town) {
		//are we a 'main' hero?
		if(hero_roles.find(hero.id) != hero_roles.end() && hero_roles[hero.id] == ROLE_MAIN) {
			auto town = (town_t*)map.get_interactable_object_for_tile(hero.x, hero.y);
			if(game.ai_should_recruit_troops(&hero, town)) {
				best_goal.goal = GOAL_RECRUIT_TROOPS;
				best_goal.priority = 100;
				best_goal.target_coord.x = hero.x;
				best_goal.target_coord.y = hero.y;
				best_goal.target_object = town;
				//best_goal.target_object = get_town()
				return best_goal;
			}
		}
	}

	int highest_value = -1;
	interactable_object_t* target_obj = nullptr;
	map_monster_t* target_guard = nullptr;

	// Define search radius - could be dynamic based on hero role (scouts search wider?)
	int radius = 6; //hero.get_scouting_radius() > 0 ? hero.get_scouting_radius() : 10; // Example radius
	int search_radius_sq = radius * radius;

	for(auto obj : map.objects) {
		if(!obj) continue;

		//distance check
		int dx = obj->x - hero.x;
		int dy = obj->y - hero.y;
		if(dx * dx + dy * dy > search_radius_sq)
			continue; //object too far away

		// Get object value *for this hero* (considers guard winnability)
		int current_value = get_ai_value_for_object(obj, &hero, &game);

		// Update best target if this object is better
		if(current_value > highest_value)
		{
// Check reachability *before* finalizing (get_route_cost_to_tile handles this)
			auto target_tile = map.get_interactable_coordinate_for_object(obj);
			int cost_to_target = map.get_route_cost_to_tile(&hero, target_tile.x, target_tile.y, &game);
			if(cost_to_target != -1)
			{ // Reachable?
				highest_value = current_value;
				target_obj = obj;
				//target_guard = guard; // Store the guard if present (even if null)
			}
		}
	}

	// --- Construct the Goal ---
	if(target_obj && highest_value > 0)
	{ // Found a valuable, reachable target
		best_goal.priority = highest_value;
		best_goal.target_object = target_obj;
		best_goal.target_guard = target_guard; // Assign the guard (might be null)

		if(target_guard)
		{
// If guarded (and winnable, checked in get_ai_value_for_object), the immediate goal is the guard
			best_goal.goal = GOAL_ATTACK_MONSTER;
			best_goal.target_coord = { target_guard->x, target_guard->y }; // Target the guard's tile
		}
		else
		{
			 // If not guarded, the goal is the object itself
			best_goal.goal = GOAL_CAPTURE_OBJECT; // Use a more generic term than pickup
			best_goal.target_coord = { target_obj->x, target_obj->y }; // Target the object's tile
		}
	}
	else
	{
	 // No valuable objects found, maybe explore?
		best_goal.goal = GOAL_EXPLORE; // Implement exploration logic separately
		best_goal.priority = 5; // Low priority exploration goal
		// Find a nearby unexplored tile as target_coord for exploration
		int startx = hero.x;
		int starty = hero.y;
		int explore_radius = 6;
		int xoff = (rand() % (explore_radius * 2)) - explore_radius;
		int yoff = (rand() % (explore_radius * 2)) - explore_radius;
		int destx = std::clamp(startx + xoff, 0, map.width - 1);
		int desty = std::clamp(starty + yoff, 0, map.height - 1);
		//bug? here if xoff and yoff == 0
		best_goal.target_coord.x = destx;
		best_goal.target_coord.y = desty;

	}

	return best_goal;
}

// Execute a single step towards the hero's current goal
// Returns true if an action was taken (movement, interaction), false otherwise
bool game_t::ai_execute_hero_step(hero_t* hero, ai_goal_t& current_goal) {
	if(!hero || current_goal.goal == GOAL_NONE || current_goal.priority <= 0)
		return false;

	bool is_final_step_for_goal = false;
	step_t next_step;

	if(hero->x != current_goal.target_coord.x || hero->y != current_goal.target_coord.y) {
		route_t route = map.get_route(hero, current_goal.target_coord.x, current_goal.target_coord.y, this);

		if(route.empty()) {	
			current_goal.goal = GOAL_NONE;
			current_goal.priority = -1;
			return false;
		}

		next_step = route.front();

		is_final_step_for_goal = (next_step.tile.x == current_goal.target_coord.x && next_step.tile.y == current_goal.target_coord.y);
		// Check if enough MP for the next step
		if(next_step.total_cost > hero->movement_points) {
			hero->movement_points = 0; // End hero's movement for the turn
			return false;
		}

	}
	else {
		is_final_step_for_goal = true;
	}


	// --- Perform Action ---
	map_action_e action_result = MAP_ACTION_NONE;
	bool action_taken = false;

	// Move the hero to the next step tile
	// move_hero_to_tile should handle combat initiation if moving onto an enemy
	if(!is_final_step_for_goal) {
		action_result = map.move_hero_to_tile(*hero, next_step.tile.x, next_step.tile.y, *this);
		action_taken = true; // Movement is an action

		// Record movement action for replay
		replay_action_t movement_action;

		movement_action.action = ACTION_MOVE_TO_TILE;
		movement_action.hero = hero;
		movement_action.tile_x = next_step.tile.x;
		movement_action.tile_y = next_step.tile.y;
		last_turn_actions[hero->player].push_back(movement_action);
		
		return true;
	}

	// --- Handle Reaching the Goal Coordinate ---
	if(!is_final_step_for_goal)
		return action_taken;

	if(current_goal.goal == GOAL_ATTACK_MONSTER) {
		if(action_result == MAP_ACTION_BATTLE_INITIATED) { // Check if combat actually started
// Check if hero won (this state needs to be accessible after combat resolution)
// Assuming combat resolution removes the guard object if won:
			if(!map.get_monster_guarding_tile(current_goal.target_object->x, current_goal.target_object->y))
			{
// Guard defeated! Update goal to the original object
				current_goal.goal = GOAL_CAPTURE_OBJECT;
				current_goal.target_coord = { current_goal.target_object->x, current_goal.target_object->y };
				current_goal.target_guard = nullptr; // Guard is gone
				// Don't return yet, might have MP left to move onto the object tile in the *next* step/iteration
			}
			else
			{
							// Lost fight or combat didn't happen? Invalidate goal.
				current_goal.goal = GOAL_NONE;
				current_goal.priority = -1;
			}
		}
		else
		{
					// Moved to guard tile but no battle? Maybe blocked? Invalidate.
			current_goal.goal = GOAL_NONE;
			current_goal.priority = -1;
		}

	}
	else if(current_goal.goal == GOAL_RECRUIT_TROOPS) {
		auto town = (town_t*)map.get_interactable_object_for_tile(hero->x, hero->y); //FIXME
		ai_recruit_troops(hero, town);
	}
	else if(current_goal.goal == GOAL_CAPTURE_OBJECT)
	{
		interactable_object_t* obj_at_dest = map.get_interactable_object_for_tile(next_step.tile.x, next_step.tile.y);
		if(obj_at_dest == current_goal.target_object)
		{ // Ensure it's the correct object
			if(interactable_object_t::is_pickupable(obj_at_dest))
				ai_pickup_object(hero, obj_at_dest);
			//action_result = map.interact_with_object(hero, obj_at_dest, *this);
			// Record interaction action (if needed for replay)
			// ... add replay logic for interaction ...

			// Interaction might remove the object (pickup) or change its state (flag mine)
			// Invalidate the goal as it's now completed.
			current_goal.goal = GOAL_NONE;
			current_goal.priority = -1;
		}
		else
		{
					// Reached destination tile, but the target object isn't there? Invalidate goal.
			current_goal.goal = GOAL_NONE;
			current_goal.priority = -1;
		}
	}
	else if(current_goal.goal == GOAL_EXPLORE) {

	}

	// Return true because the hero moved (took an action)
	return action_taken;
}

void print_debug_hero_goal(hero_t* hero, ai_goal_t& goal) {
	std::cout << "[" << magic_enum::enum_name(hero->player).data() << "] ";
	std::cout << "acquired goal for hero '" << hero->name << "' (" << (int)hero->x << ", " << (int)hero->y << "): ";
	std::cout << magic_enum::enum_name(goal.goal).data();
	if(goal.target_object)
		std::cout << " - " << magic_enum::enum_name(goal.target_object->object_type) << " at (" << goal.target_object->x << ", " << goal.target_object->y << ").";

	std::cout << std::endl;
}

void game_t::process_ai_turn(player_e player_num) {
	//for(int i = 1; i < 5; i++) {
		//int fake_delay_ms_REMOVE_ME = 10;//900;
		//std::this_thread::sleep_for(std::chrono::milliseconds(fake_delay_ms_REMOVE_ME));
		//std::cout << "calling ai_progress callback (" << i*20 << ")\n";
		//ai_turn_progress_callback_fn(player_num, i*20);
	//}

	last_turn_actions[player_num].clear();


	if(player_num != PLAYER_2) {
		//end_turn(player_num);
		//return;
	}
	//should we recruit a new hero?

	for(auto obj : map.objects) {
		if(!obj || obj->object_type != OBJECT_MAP_TOWN)
			continue;

		auto town = (town_t*)obj;
		if(town->player != player_num)
			continue;

		auto building_to_build = ai_determine_which_building_to_build(town, true);
		if(building_to_build == BUILDING_NONE)
			continue;

		town->build_building(building_to_build, *this);
		std::cout << "[" << magic_enum::enum_name(player_num).data() << "] built building: " << magic_enum::enum_name(building_to_build).data()
			<< " at town '" << town->name << "' (" << town->x << ", " << town->y << ")." << std::endl;
	}

	for(auto& h : map.heroes) {
		auto& hero = h.second;
		if(hero.player != player_num)
			continue;

		hero_roles[hero.id] = classify_hero(&hero);

		replay_action_t action;
		action.action = ACTION_REPLAY_SET_INITIAL_HERO_POSITION;
		action.hero = &hero;
		action.tile_x = hero.x;
		action.tile_y = hero.y;
		last_turn_actions[player_num].push_back(action);
	}
	

	std::vector<hero_plan_t> hero_plans;
	std::set<uint32_t> reserved_targets;

	std::unordered_map<uint16_t, ai_goal_t> hero_current_goals;

	bool can_any_hero_act = true;
	int max_iterations = 200;
	int i = 0;

	while(can_any_hero_act && i < max_iterations) {
		can_any_hero_act = false;

		for(auto& h : map.heroes) {
			auto& hero = h.second;
			if(hero.player != player_num || hero.movement_points < 50)
				continue;
		
			ai_goal_t& current_goal = hero_current_goals[hero.id];
			if(current_goal.goal == GOAL_NONE || current_goal.priority <= 0) {
				current_goal = find_best_hero_goal(hero, map, *this);
				print_debug_hero_goal(&hero, current_goal);
				if(current_goal.goal == GOAL_NONE || current_goal.priority <= 0)
					continue;
			}

			int s = 0;
			while(s++ < max_iterations) {
				if(!ai_execute_hero_step(&hero, current_goal))
					break;
			}

			current_goal = find_best_hero_goal(hero, map, *this);
			print_debug_hero_goal(&hero, current_goal);
			if(current_goal.goal != GOAL_NONE)
				can_any_hero_act = true;

		}

		i++;
	}

	//for(auto& h : map.heroes) {
	//	auto& hero = h.second;
	//	if(hero.player != player_num)
	//		continue;
	//		
	//	while(hero.movement_points > 50) {
	//		auto valuable_object = find_best_object(hero, map, *this);
	//		//move to and pickup most valuable object
	//		if(!valuable_object)
	//			break;

	//		if(!ai_move_hero_to_object(&hero, valuable_object))
	//			break;

	//		if(i++ > max_iterations)
	//			break;
	//	}
	//}
	
	end_turn(player_num);
}
