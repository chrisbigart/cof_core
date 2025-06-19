#include "core/game.h"
#include "core/map_file.h"
#include "core/adventure_map.h"
#include "core/utils.h"

#include <random>
#include <chrono>
#include <set>

#include "core/qt_headers.h"

QDataStream& operator<<(QDataStream& stream, const player_t& player) {
	stream << player.player_number;
	stream << player.is_human;
	stream_write_vector(stream, player.heroes_available_to_recruit);
	stream << player.has_completed_turn;
	stream << player.resources;
	stream << player.tile_visibility;
	stream << player.tile_observability;
	
	return stream;
}

QDataStream& operator>>(QDataStream& stream, player_t& player) {
	stream >> player.player_number;
	stream >> player.is_human;
	stream_read_array(stream, player.heroes_available_to_recruit);
	stream >> player.has_completed_turn;
	stream >> player.resources;
	stream >> player.tile_visibility;
	stream >> player.tile_observability;
	
	return stream;
}


QDataStream& operator<<(QDataStream& stream, const resource_group_t& resource) {
	stream_write_vector(stream, resource.values);
	
	return stream;
}

QDataStream& operator>>(QDataStream& stream, resource_group_t& resource) {
	stream_read_array(stream, resource.values);
	
	return stream;
}


QDataStream& operator<<(QDataStream& stream, const resource_value_t& resource) {
	stream << resource.type;
	stream << resource.value;
	
	return stream;
}

QDataStream& operator>>(QDataStream& stream, resource_value_t& resource) {
	stream >> resource.type;
	stream >> resource.value;
	
	return stream;
}

bool resource_valid(resource_e resource_type) {
	return magic_enum::enum_contains(resource_type);
}

bool is_magic_resource(resource_e resource_type) {
	return (resource_type == RESOURCE_GEMS
			|| resource_type == RESOURCE_CRYSTALS
			|| resource_type == RESOURCE_MERCURY
			|| resource_type == RESOURCE_SULFUR);
}

bool is_basic_resource(resource_e resource_type) {
	return (resource_type == RESOURCE_WOOD || resource_type == RESOURCE_ORE);
}

void game_t::check_for_game_status_updates() {
	for(auto& pl : players) {
		if(!pl.is_alive)
			continue;

		bool is_alive = false;
		for(auto& h : map.heroes) {
			if(h.second.player == pl.player_number) {
				is_alive = true;
				break;
			}
		}
		for(auto obj : map.objects) {
			if(!obj || obj->object_type != OBJECT_MAP_TOWN)
				continue;
			
			auto town = (town_t*)obj;
			if(town->player == pl.player_number) {
				is_alive = true;
				break;
			}
		}

		if(!is_alive) {
			game_event_callback_fn(GAME_STATE_PLAYER_ELIMINATED, pl.player_number, 0);
			pl.is_alive = false;
		}
	}

	auto game_status = get_scenario_status();
	if(game_status == GAME_STATUS_INCOMPLETE)
		return;
	
	
	if(game_status == GAME_STATUS_HUMAN_LOSS)
		game_event_callback_fn(GAME_STATE_LOST_GAME, PLAYER_NONE, 0);
	else if(game_status == GAME_STATUS_HUMAN_VICTORY)
		game_event_callback_fn(GAME_STATE_WON_GAME, PLAYER_NONE, 0);

}

game_status_e game_t::get_scenario_status() {
	bool human_alive = false;
	bool human_allies_alive = false;
	bool cpu_alive = false;

	//todo handle hotseat
	player_e human_player = PLAYER_NONE;
	for(const auto& pl : players) {
		if(pl.is_human)
			human_player = pl.player_number;
	}

	if(map.win_condition == WIN_CONDITION_DEFEAT_ALL_ENEMIES) {
		for(auto& h : map.heroes) {
			const auto& p = get_player(h.second.player);
			if(p.is_human)
				human_alive = true;
			else if(!are_players_allied(p.player_number, human_player))
				cpu_alive = true;
				
		}
		for(auto obj : map.objects) {
			if(!obj || obj->object_type != OBJECT_MAP_TOWN)
				continue;
			
			auto town = (town_t*)obj;
			if(!player_valid(town->player))
				continue;

			const auto& p = get_player(town->player);
			if(p.is_human)
				human_alive = true;
			else if(!are_players_allied(p.player_number, human_player))
				cpu_alive = true;
		}
		
	}
	
	if(human_alive && cpu_alive)
		return GAME_STATUS_INCOMPLETE;
	else if(!human_alive)
		return GAME_STATUS_HUMAN_LOSS;
	
	return GAME_STATUS_HUMAN_VICTORY;
}

std::string game_t::get_date() {
    return (QObject::tr("Month") + " " + QString::number(get_month())
        + ",  " + QObject::tr("Week") + " " + QString::number(get_week())
        + ",  " + QObject::tr("Day") + " " + QString::number(get_day())).toStdString();
}

//todo dedup these functions


bool game_t::player_valid(player_e player) const {
	if(player == PLAYER_NONE || player == PLAYER_NEUTRAL)
		return false;
		
	if((player - 1) >= players.size())
		return false;
	
	return true;
}

player_t dummy_player;

player_t& game_t::get_player(player_e player) {
	if(!player_valid(player))
		return dummy_player;

	return players[((int)player) - 1];
}

const player_t& game_t::get_player(player_e player) const {
	if(!player_valid(player))
		return dummy_player;

	return players[((int)player) - 1];
}

player_e game_t::get_first_human_player() const {
	for(const auto& pc : game_configuration.player_configs) {
		if(pc.is_human)
			return pc.player_number;
	}
	
	return PLAYER_NONE;
}

bool game_t::is_tile_visible(int x, int y, player_e player) const {
	if(!map.tile_valid(x, y) || !player_valid(player))
		return false;
	
	const auto& visibility = get_player(player).tile_visibility;
	int offset = x + y * map.width;
	if(offset >= visibility.size())
		return false;

	return visibility.testBit(offset);
}

bool game_t::is_tile_observable(int x, int y, player_e player) const {
	if(!map.tile_valid(x, y) || !player_valid(player))
		return false;

	const auto& observability = get_player(player).tile_observability;
	int offset = x + y * map.width;
	if(offset >= observability.size())
		return false;
	
	return observability.testBit(offset);
}

bool game_t::is_tile_visible_and_observable(int x, int y, player_e player) const {
	if(!player_valid(player))
		return false;

	return is_tile_visible(x, y, player) && is_tile_observable(x, y, player);
}

void game_t::update_visibility() {
	for(auto& p : players) {
		update_visibility(p.player_number);
	}
}

void game_t::reveal_area(player_e player, int x, int y, int reveal_radius, int observe_radius) {
	if(!player_valid(player))
		return;
	
	auto& tile_visibility = get_player(player).tile_visibility;
	auto& tile_observability = get_player(player).tile_observability;
	
	if(observe_radius < reveal_radius)
		observe_radius = reveal_radius;
	
	for(int ypos = y - (observe_radius + 1); ypos < y + (observe_radius + 1); ypos++) {
		for(int xpos = x - (observe_radius + 1); xpos < x + (observe_radius + 1); xpos++) {
			if(!map.tile_valid(xpos, ypos))
				continue;
			
			bool revealed = utils::dist(x, y, xpos, ypos) < reveal_radius;
			bool observed = utils::dist(x, y, xpos, ypos) < observe_radius;
			
			if(!observed)
				continue;
			
			if(revealed)
				tile_visibility.setBit(xpos + ypos*map.width, true);
			
			if(tile_visibility.testBit(xpos + ypos*map.width))
				tile_observability.setBit(xpos + ypos*map.width, true);
		}
	}
}

player_configuration_t dummy_configuration;

player_configuration_t& game_t::get_player_configuration(player_e player) {
	if(!player_valid(player))
		return dummy_configuration;

	return game_configuration.player_configs[(int)player - 1];
}

const player_configuration_t& game_t::get_player_configuration(player_e player) const {
	if(!player_valid(player))
		return dummy_configuration;

	return game_configuration.player_configs[(int)player - 1];
}

bool game_t::are_players_allied(player_e player1, player_e player2) const {
	if(player1 == PLAYER_NONE || player2 == PLAYER_NONE)
		return false;
	
	if(player1 == player2)
		return true;
	
	if(!player_valid(player1) || !player_valid(player2))
		return false;

	const auto& p1config = get_player_configuration(player1);
	const auto& p2config = get_player_configuration(player2);
	
	return p1config.team == p2config.team;
}

int game_t::get_player_marketplace_count(player_e player_number) {
	if(!player_valid(player_number))
		return 0;
	
	int count = 0;
	
	for(auto obj : map.objects) {
		if(!obj)
			continue;
		
		if(obj->object_type == OBJECT_MAP_TOWN) {
			town_t* t = (town_t*)obj;
			
			if(t->is_building_built(BUILDING_AUCTION_HOUSE))
				count += 2;
			else if(t->is_building_built(BUILDING_MARKETPLACE))
				count += 1;
		}
		//else id(obj->type == TRADING_POST && obj->owner == player
	}
	
	for(auto& h : map.heroes) {
		//if(!are_players_allied(h.second.player, player)) //consider this as a buff to TALENT_TRADER
		if(h.second.player != player_number)
			continue;
		
		if(h.second.has_talent(TALENT_TRADER))
			count++;
	}
	
	return count;
}

static constexpr int gold_to_basic_rate[9] = {2500, 1667, 1250, 1000, 833, 714, 625, 556, 500};
static constexpr int gold_to_magical_rate[9] = {5000, 3333, 2500, 2000, 1667, 1429, 1250, 1111, 1000};
static constexpr int basic_to_gold_rate[9] = {25, 37, 50, 62, 75, 88, 100, 112, 125};
static constexpr int basic_to_basic_rate[9] = {10, 7, 5, 4, 3, 3, 3, 2, 2};
static constexpr int basic_to_magical_rate[9] = {20, 13, 10, 8, 7, 6, 5, 4, 4};
static constexpr int magical_to_gold_rate[9] = {50, 75, 100, 125, 150, 175, 200, 225, 250};
static constexpr int magical_to_basic_rate[9] = {5, 3, 3, 2, 2, 1, 1, 1, 1};
static constexpr int magical_to_magical_rate[9] = {10, 7, 5, 4, 3, 3, 3, 2, 2};

int game_t::get_marketplace_trade_rate(player_e player_number, resource_e resource_type_to_trade, resource_e resource_type_to_receive) {
	if(!player_valid(player_number))
		return 1;
	
	if(!resource_valid(resource_type_to_trade) || !resource_valid(resource_type_to_receive))
		return 1;
	
	int marketplace_count = std::clamp(get_player_marketplace_count(player_number) - 1, 0, 8);
	
	if(resource_type_to_trade == RESOURCE_GOLD && is_basic_resource(resource_type_to_receive))
		return gold_to_basic_rate[marketplace_count];
	else if(resource_type_to_trade == RESOURCE_GOLD && is_magic_resource(resource_type_to_receive))
		return gold_to_magical_rate[marketplace_count];
	else if(is_basic_resource(resource_type_to_trade) && resource_type_to_receive == RESOURCE_GOLD)
		return basic_to_gold_rate[marketplace_count];
	else if(is_magic_resource(resource_type_to_trade) && resource_type_to_receive == RESOURCE_GOLD)
		return magical_to_gold_rate[marketplace_count];
	else if(is_basic_resource(resource_type_to_trade) && is_basic_resource(resource_type_to_receive))
		return basic_to_basic_rate[marketplace_count];
	else if(is_basic_resource(resource_type_to_trade) && is_magic_resource(resource_type_to_receive))
		return basic_to_magical_rate[marketplace_count];
	else if(is_magic_resource(resource_type_to_trade) && is_basic_resource(resource_type_to_receive))
		return magical_to_basic_rate[marketplace_count];
	else if(is_magic_resource(resource_type_to_trade) && is_magic_resource(resource_type_to_receive))
		return magical_to_magical_rate[marketplace_count];

	
	return 1;
}

bool game_t::trade_resources_via_marketplace(player_e player_number, resource_e resource_type_to_trade, uint32_t quantity_to_trade, resource_e resource_type_to_receive) {
	if(!player_valid(player_number))
		return false;
	
	if(!resource_valid(resource_type_to_trade) || !resource_valid(resource_type_to_receive))
		return false;
	
	if(quantity_to_trade > MAX_RESOURCE_VALUE)
		return false;
	
	auto& player = get_player(player_number);
	auto& pres = player.resources;
	
	auto current_quantity = pres.get_value_for_type(resource_type_to_trade);
	
	//can't trade what we don't have
	if(current_quantity < quantity_to_trade)
		return false;
	
	auto trade_rate = get_marketplace_trade_rate(player_number, resource_type_to_trade, resource_type_to_receive);
	
	bool rate_one_to_many = (resource_type_to_receive == RESOURCE_GOLD);
	uint32_t quantity_received = 0;
	
	if(rate_one_to_many) //only for resources to gold trades
		quantity_received = quantity_to_trade * trade_rate;
	else //rate_many_to_one
		quantity_received = quantity_to_trade / trade_rate;
	
	if(quantity_received == 0)
		return false;
	
	pres.sub_value(resource_type_to_trade, quantity_to_trade);
	pres.add_value(resource_type_to_receive, quantity_received);
	
	return true;
}

bool game_t::give_resources_from_player_to_player(player_e from_player_number, player_e to_player_number, resource_e resource_type, uint32_t quantity_to_trade) {
	if(!player_valid(from_player_number) || !player_valid(to_player_number))
		return false;
	
	if(!resource_valid(resource_type))
		return false;
	
	auto& from_player = get_player(from_player_number);
	auto& to_player = get_player(to_player_number);
	
	auto current_quantity = from_player.resources.get_value_for_type(resource_type);
	
	//can't give away more that we have
	if(current_quantity < quantity_to_trade)
		return false;
	
	
	
	return true;
}

bool game_t::give_resource_to_player(player_e player, resource_e resource_type, uint32_t quantity) {
	if(!player_valid(player))
		return false;

	if(!resource_valid(resource_type))
		return false;

	auto& pl = get_player(player);
	pl.resources.add_value(resource_type, quantity);

	return true;
}

void game_t::update_visibility(player_e player) {
	if(!player_valid(player))
		return;

	//auto& pl = get_player(player);
	auto& tile_visibility = get_player(player).tile_visibility;
	auto& tile_observability = get_player(player).tile_observability;
	auto width = map.width;
	auto height = map.height;
	
	//probably move this to load_map function
	if(tile_visibility.size() != width * height) {
		tile_visibility.resize(width * height);
		tile_visibility.fill(false);
		tile_observability.resize(width * height);
		tile_observability.fill(false);
	}
	
	tile_observability.fill(false);	
	
	for(auto obj : map.objects) {
		if(!obj)
			continue;
		
		if(obj->object_type == OBJECT_MAP_TOWN) {
			town_t* t = (town_t*)obj;
			
			if(!are_players_allied(t->player, player))
				continue;
			
			//calculate front-door tile:
			auto pos = map.get_interactable_coordinate_for_object(t);
			reveal_area(player, pos.x, pos.y, t->reveal_area, t->observation_area);
		}
		else if(obj->object_type == OBJECT_MINE) {
			auto mine = (mine_t*)obj;
			if(!are_players_allied(mine->owner, player))
				continue;
			
			auto pos = map.get_interactable_coordinate_for_object(mine);
			reveal_area(player, pos.x, pos.y, 3, 5);
		}
		else if(obj->object_type == OBJECT_WINDMILL || obj->object_type == OBJECT_WATERWHEEL) { //flaggable objects
			auto flaggable = (flaggable_object_t*)obj;
			if(!are_players_allied(flaggable->owner, player))
				continue;
			
			auto pos = map.get_interactable_coordinate_for_object(flaggable);
			reveal_area(player, pos.x, pos.y, 3, 4);
		}
		else if(obj->object_type == OBJECT_WATCHTOWER) {
			auto tower = (flaggable_object_t*)obj;
			if(tower->visited && are_players_allied(tower->owner, player)) {
				auto pos = map.get_interactable_coordinate_for_object(tower);
				auto date_diff = date - tower->date_visited;
				reveal_area(player, pos.x, pos.y, 3, std::clamp(20 - date_diff, 3, 20));
			}
		}
	}	
	
	for(auto& h : map.heroes) {
		if(!are_players_allied(h.second.player, player))
			continue;
		
		int xp = h.second.x;
		int yp = h.second.y;
		int rev_rad = h.second.get_scouting_radius() + 1;
		int obs_rad = h.second.get_observation_radius() + 1;
		for(int y = yp - obs_rad; y < yp + obs_rad; y++) {
			for(int x = xp - obs_rad; x < xp + obs_rad; x++) {
				if(!map.tile_valid(x, y))
					continue;
				
				bool revealed = utils::dist(x, y, xp, yp) < rev_rad;
				bool observed = utils::dist(x, y, xp, yp) < obs_rad;
				
				if(!observed)
					continue;
				
				if(revealed)
					tile_visibility.setBit(x + y*width, true);
				
				if(tile_visibility.testBit(x + y*width))
					tile_observability.setBit(x + y*width, true);
			}
		}
	}
}

bool game_t::end_turn(player_e player) {
	if(!player_valid(player))
		return false;
	
	get_player(player).has_completed_turn = true;

	bool day_has_ended = true;
	for(auto& p : players) {
		if(!p.has_completed_turn) {
			day_has_ended = false;
			break;
		}
	}
	
	if(day_has_ended) {
		next_day();
		
		if(date % 7 == 0) //start of new week
			next_week();
	}
	
	return day_has_ended;
}

void game_t::next_week() {
	for(auto& obj : map.objects) {
		if(!obj)
			continue;
		
		if(obj->object_type == OBJECT_MAP_TOWN) {
			auto town = (town_t*)obj;
			bool freelancer = false;
			bool call_to_arms = false;
			
			for(const auto& h : map.heroes) {
				const auto& hero = h.second;
				if(hero.player != town->player)
					continue;
				
				if(town->town_type == TOWN_BARBARIAN && hero.has_talent(TALENT_FREELANCER))
					freelancer = true;
				if(town->town_type == TOWN_KNIGHT && hero.has_talent(TALENT_CALL_TO_ARMS))
					call_to_arms = true;
				
			}
			
			town->new_week(freelancer, call_to_arms);
		}
		else if(obj->object_type == OBJECT_REFUGEE_CAMP) {
			map.populate_refugee_camp((refugee_camp_t*)obj);
		}
		else if(obj->object_type == OBJECT_WINDMILL) {
			auto mill = (flaggable_object_t*)obj;
			//bool visited = 0;
			auto resource = (resource_e)(utils::rand_range(2, 6));
			auto quantity = utils::rand_range(3, 7);
			
			mill->visited |= (resource << 4);
			mill->visited |= quantity;
		}
	}
	
	for(auto& h : map.heroes) {
		//fixme: update to use artifact properties
		if(h.second.is_artifact_in_effect(ARTIFACT_FOUR_LEAF_GEMSTONE)) {
			resource_group_t res;
			res.set_value_for_type(RESOURCE_GEMS, 1);
			get_player(h.second.player).resources += res;
		}
	}

	generate_new_week_recruitable_heroes();
}

void game_t::next_day() {
    date++;
	
	for(auto& obj : map.objects) {
		if(!obj)
			continue;
		
		if(obj->object_type == OBJECT_MAP_TOWN) {
			auto town = (town_t*)obj;
			town->new_day(date);
			auto income = town->get_daily_income(date);
			auto player = town->player;
			if(player != PLAYER_NONE && player != PLAYER_NEUTRAL)
				get_player(player).resources += income;
		}
		else if(obj->object_type == OBJECT_MINE) {
			auto mine = (mine_t*)obj;
			auto player = mine->owner;
			auto res = mine->mine_type;
			
			resource_group_t income;
			int value = 1;
			if(res == RESOURCE_GOLD)
				value = 1000;
			else if(res == RESOURCE_WOOD || res == RESOURCE_ORE)
				value = 2;
			
			income.set_value_for_type(res, value);
			if(player != PLAYER_NONE && player != PLAYER_NEUTRAL)
				get_player(player).resources += income;
			
		}
	}
    for(auto& h : map.heroes) {
		auto& hero = h.second;
		hero.new_day(date, map.get_tile(hero.x, hero.y).terrain_type);
		
		if(hero.has_talent(TALENT_FIEFDOM) && player_valid(hero.player)) {
			get_player(hero.player).resources += resource_group_t(0, 0, 0, 0, 0, 0, 100 * hero.level);
		}
		if(hero.specialty == SPECIALTY_GOLD && player_valid(hero.player)) {
			get_player(hero.player).resources += resource_group_t(0, 0, 0, 0, 0, 0, 200 + (50 * hero.level));

		}
		if(!hero.garrisoned && hero.has_talent(TALENT_FORAGING) && player_valid(hero.player)) {
			resource_group_t wood_and_ore(map.get_owned_mines_count(RESOURCE_WOOD, hero.player),
										  map.get_owned_mines_count(RESOURCE_ORE, hero.player),
										  0, 0, 0, 0, 0);
			
			get_player(hero.player).resources += wood_and_ore;
		}
//
//		update_lua_hero_table(_lua_state, &hero);
//		for(auto& s : hero.scripts) {
//			if(!s.enabled || s.finished)
//				continue;
//			
//			auto result = luaL_dostring(_lua_state, s.text.c_str());
//			if(result != LUA_OK) {
//				std::string errormsg = lua_tostring(_lua_state, -1);
//				std::cerr << "lua script error: " << errormsg << std::endl;
//			}
//		}
    }
	
	for(auto& p : players) {
		p.has_completed_turn = false;
	}

}

bool game_t::give_hero_xp(hero_t* hero, uint64_t xp) {
	if(!hero)
		return false;
	
	uint64_t xp_to_next_level = hero->get_xp_to_next_level();
	
	if(xp < xp_to_next_level) { //no level up, just add xp and return
		hero->experience += xp;
		for(auto& h : hero_unallocated_xp) {
			if(h.first == hero) {
				h.second = 0;
				break;
			}
		}
		return false;
	}
	else {
		//level up hero...
		bool has_talent_point = hero->has_talent_point_on_level_up();
		hero->level++;
		auto skills = hero->get_level_up_skills();
		auto stat = hero->level_up_stats();
		hero->experience += xp_to_next_level;
		
		xp -= xp_to_next_level;
		
		//...then figure out if we have additional levelups pending
		if(xp >= hero->get_xp_to_next_level()) {
			bool found = false;
			for(auto& h : hero_unallocated_xp) {
				if(h.first == hero) {
					h.second -= xp_to_next_level;
					found = true;
					break;
				}
			}
			if(!found)
				hero_unallocated_xp.push_back(std::make_pair(hero, xp));
			
			show_hero_levelup_callback_fn(hero, skills[0], skills[1], skills[2], stat, has_talent_point);
			return true;
		}
		else {
			for(auto it = hero_unallocated_xp.begin(); it != hero_unallocated_xp.end();) {
				if(it->first == hero)
					it = hero_unallocated_xp.erase(it);
				else
					it++;
			}
			hero->experience += xp;
			show_hero_levelup_callback_fn(hero, skills[0], skills[1], skills[2], stat, has_talent_point);
			return false;
		}
	}
	
	return false;
}

bool game_t::process_hero_unallocated_xp(hero_t* hero) {
	//remove dead entries
	for(auto it = hero_unallocated_xp.begin(); it != hero_unallocated_xp.end();) {
		if(it->second == 0)
			it = hero_unallocated_xp.erase(it);
		else
			it++;
	}
	
	for(auto h : hero_unallocated_xp) {
		if(h.first == hero) {
			give_hero_xp(h.first, h.second);
			return true;
		}
	}
	
	return false;
}

void game_t::apply_battle_necromancy_results(hero_t* hero) {
	if(!hero)
		return;

	//todo: fixme for when troops don't fit
	for(auto& nec_tr : battle.necromancy_raised_troops.troops) {
		if(nec_tr.is_empty())
			continue;

		for(auto& hero_tr : hero->troops) {
			if(hero_tr.unit_type == nec_tr.unit_type) {
				hero_tr.stack_size = std::clamp(hero_tr.stack_size + nec_tr.stack_size, 0, 50000);
				//todo: handle potential leftover troops
				//allocated = true;
				break;
			}
			else if(hero_tr.is_empty()) {
				hero_tr.unit_type = nec_tr.unit_type;
				hero_tr.stack_size = nec_tr.stack_size;//std::clamp(nec_tr.stack_size, 0, 50000);
				break;
			}
		}

	}
}

bool game_t::battle_retreat() {
	return false;
}

void game_t::accept_battle_results() {
	if(battle.result == BATTLE_DEFENDER_VICTORY || battle.result == BATTLE_BOTH_LOSE) {
		if(battle.defending_monster) {
			uint16_t remaining = 0;
			for(auto& tr : battle.get_remaining_troops_defender())
				remaining += tr.stack_size;
			
			battle.defending_monster->quantity = remaining;
			remove_hero_callback_fn(battle.attacking_hero, true);
			map.remove_hero(battle.attacking_hero);
		}
		else if(battle.defending_hero) {
			battle.defending_hero->troops = battle.get_remaining_troops_defender();
			give_hero_xp(battle.defending_hero, battle.get_winning_hero_xp());

			for(const auto& art : battle.captured_artifacts)
				battle.defending_hero->pickup_artifact(art);
			
			remove_hero_callback_fn(battle.attacking_hero, true);
			map.remove_hero(battle.attacking_hero);
		}
	}
	else if(battle.result == BATTLE_ATTACKER_VICTORY) {
		battle.attacking_hero->troops = battle.get_remaining_troops_attacker();
		give_hero_xp(battle.attacking_hero, battle.get_winning_hero_xp());

		for(const auto& art : battle.captured_artifacts)
			battle.attacking_hero->pickup_artifact(art);

		if(battle.attacking_hero->get_secondary_skill_level(SKILL_NECROMANCY))
			apply_battle_necromancy_results(battle.attacking_hero);

		if(battle.defending_map_object) {
			switch(battle.defending_map_object->object_type) {
				case OBJECT_GRAVEYARD: {
					//fixme
					auto graveyard = (graveyard_t*)battle.defending_map_object;
					graveyard->visited = true;
					auto artifact = graveyard->artifact_reward;
					if(artifact == ARTIFACT_NONE) { //select random
						//fixme: should not happen, we need to select a random reward at map creation instead!
						artifact = adventure_map_t::get_random_artifact_of_rarity(RARITY_COMMON);
					}
					battle.attacking_hero->pickup_artifact(artifact);
					auto player = battle.attacking_hero->player;
					if(player != PLAYER_NONE)
						give_resource_to_player(player, RESOURCE_GOLD, graveyard->size * 1000 + 500);

					show_dialog_callback_fn(DIALOG_TYPE_GRAVEYARD_RESULT, battle.defending_map_object, battle.attacking_hero, artifact, 0);
					break;
				}
				case OBJECT_PYRAMID: {
					//fixme: bug here if on hero level-up, hero gets skill to learn pyramid spell,
					//but this skill is selected after we do the check below to see if they can learn it
					auto pyramid = (pyramid_t*)battle.defending_map_object;
					pyramid->visited = true;
					if(battle.attacking_hero->can_learn_spell(pyramid->spell_id))
						battle.attacking_hero->learn_spell(pyramid->spell_id);
			
					//todo: if the hero can't learn the spell, have them write it down
					//on to a spell scroll
					show_dialog_callback_fn(DIALOG_TYPE_PYRAMID_REWARD, pyramid, battle.attacking_hero, 1, pyramid->spell_id);
					break;
					}
			}
		}
		if(battle.defending_monster) {
			remove_interactable_object_callback_fn(battle.defending_monster, false);
			map.remove_interactable_object(battle.defending_monster);
		}
		else if(battle.defending_hero) {
			remove_hero_callback_fn(battle.defending_hero, false);
			map.remove_hero(battle.defending_hero);
		}

		if(battle.defending_town) {
			for(auto& tr : battle.defending_town->garrison_troops)
				tr.clear();

			battle.defending_town->player = battle.attacking_hero->player;
			update_interactable_object_callback_fn(battle.defending_town);
		}
	}

	check_for_game_status_updates();
}

void game_t::replay_battle() {
	battle.reset();
}

map_action_e game_t::interact_with_object(hero_t* hero, interactable_object_t* object) {
	if(!hero || !object)
		return MAP_ACTION_NONE;
	
	if(object->object_type == OBJECT_WARRIORS_TOMB) {
		show_dialog_callback_fn(DIALOG_TYPE_WARRIORS_TOMB_CHOICE, object, hero, 0, 0);
		return MAP_ACTION_NONE;
	}
	else if(object->object_type == OBJECT_REFUGEE_CAMP) {
		show_dialog_callback_fn(DIALOG_TYPE_REFUGEE_CAMP_CHOICE, object, hero, 0, 0);
		return MAP_ACTION_NONE;
	}
	else if(object->object_type == OBJECT_PYRAMID) {
		show_dialog_callback_fn(DIALOG_TYPE_PYRAMID_VISIT, object, hero, 0, 0);
		return MAP_ACTION_NONE;
	}
	else if(object->object_type == OBJECT_STANDING_STONES) {
		bool can_take = !hero->has_object_been_visited(object) && hero->power < 99;
		if(can_take) {
			hero->increase_power(1);
			hero->set_visited_object(object);
		}

		show_dialog_callback_fn(DIALOG_TYPE_STANDING_STONES, object, hero, can_take ? 1 : 0, 0);
		return MAP_ACTION_NONE;
	}
	else if(object->object_type == OBJECT_MAGIC_SHRINE) {
		auto shrine = (shrine_t*)object;
		shrine->set_visited_by_player(hero->player);
		show_dialog_callback_fn(DIALOG_TYPE_MAGIC_SHRINE_VISIT, object, hero, 0, 0);
		return MAP_ACTION_NONE;
	}
	else if(object->object_type == OBJECT_SCHOOL_OF_WAR) {
		show_dialog_callback_fn(DIALOG_TYPE_SCHOOL_OF_WAR_CHOICE, object, hero, 0, 0);
		return MAP_ACTION_NONE;
	}
	else if(object->object_type == OBJECT_SCHOOL_OF_MAGIC) {
		show_dialog_callback_fn(DIALOG_TYPE_SCHOOL_OF_MAGIC_CHOICE, object, hero, 0, 0);
		return MAP_ACTION_NONE;
	}
	else if(object->object_type == OBJECT_GAZEBO) {
		show_dialog_callback_fn(DIALOG_TYPE_GAZEBO_VISIT, object, hero, 0, 0);
		return MAP_ACTION_NONE;
	}
	else if(object->object_type == OBJECT_GRAVEYARD) {
		show_dialog_callback_fn(DIALOG_TYPE_GRAVEYARD_VISIT, object, hero, 0, 0);
		return MAP_ACTION_NONE;
	}
	else if(object->object_type == OBJECT_SANCTUARY) { //todo: change to hut of the magi
		show_dialog_callback_fn(DIALOG_TYPE_HUT_OF_THE_MAGI, object, hero, 0, 0);
		return MAP_ACTION_NONE;
	}
	else if(object->object_type == OBJECT_WITCH_HUT) {
		auto hut = (witch_hut_t*)object;
		const auto& skill = game_config::get_skill(hut->skill);

		if(hero->has_object_been_visited(object)) { //hero has already taken a skill/upgrade from this hut
			show_dialog_callback_fn(DIALOG_TYPE_WITCH_HUT_CHOICE, object, hero, WITCH_HUT_VISIT_RESULT_ALREADY_VISITED, 0);
			return MAP_ACTION_NONE;
		}
		
		if(hero->get_secondary_skill_base_level(hut->skill) != 0) { //hero already has skill
			if(hero->get_secondary_skill_base_level(hut->skill) >= 4) { //hero skill is maxed out
				show_dialog_callback_fn(DIALOG_TYPE_WITCH_HUT_CHOICE, object, hero, WITCH_HUT_VISIT_RESULT_HERO_SKILL_LEVEL_MAXED, 0);
				return MAP_ACTION_NONE;
			}
			else if(!hut->allows_upgrade) { //hero has skill, but hut is set to not allow upgrades
				show_dialog_callback_fn(DIALOG_TYPE_WITCH_HUT_CHOICE, object, hero, WITCH_HUT_VISIT_RESULT_SKILL_NOT_UPGRADABLE, 0);
				return MAP_ACTION_NONE;
			}		
			else { //offer to upgrade the skill
				show_dialog_callback_fn(DIALOG_TYPE_WITCH_HUT_CHOICE, object, hero, WITCH_HUT_VISIT_RESULT_SKILL_UPGRADE_OFFERED, 0);
				return MAP_ACTION_NONE;
			}
		}
		else { //hero does not have the skill already
			if(hero->get_number_of_open_skill_slots() == 0) { //hero has no empty skill slots
				show_dialog_callback_fn(DIALOG_TYPE_WITCH_HUT_CHOICE, object, hero, WITCH_HUT_VISIT_RESULT_NO_EMPTY_SKILL_SLOTS, 0);
				return MAP_ACTION_NONE;
			}
			else if(!hero->can_learn_skill(hut->skill)) { //hero cannot learn skill
				show_dialog_callback_fn(DIALOG_TYPE_WITCH_HUT_CHOICE, object, hero, WITCH_HUT_VISIT_RESULT_HERO_CANNOT_LEARN_SKILL, 0);
				return MAP_ACTION_NONE;
			}	
			else { //hero does not already have skill, can learn the skill, and has an open skill slot
				show_dialog_callback_fn(DIALOG_TYPE_WITCH_HUT_CHOICE, object, hero, WITCH_HUT_VISIT_RESULT_NEW_SKILL_OFFERED, 0);
				return MAP_ACTION_NONE;
			}
		}

		return MAP_ACTION_NONE;
	}
	else if(object->object_type == OBJECT_MAP_TOWN) {
		auto town = (town_t*)object;
		if(are_players_allied(town->player, hero->player)) {
			map.hero_visit_town(hero, town);
		}
		else if(town->is_guarded()) {
			/*army_t defenders;
			for(int i = 0; i < 5; i++) {
				defenders.troops[i].unit_type = UNIT_GHOUL;
				defenders.troops[i].stack_size = 5;
			}

			battle.init_hero_creature_bank_battle(hero, defenders);
			battle.environment_type = BATTLEFIELD_ENVIRONMENT_GRAVEYARD;*/
			return MAP_ACTION_BATTLE_INITIATED;
		}
		else {
			town->player = hero->player;
			map.hero_visit_town(hero, town);
			return MAP_ACTION_OBJECT_UPDATED;
		}

		return MAP_ACTION_NONE;
	}
	else if(object->object_type == OBJECT_MINE) {
		auto mine = (mine_t*)object;
		if(mine->owner != hero->player) { //fixme for allied players
			mine->owner = hero->player;
			show_dialog_callback_fn(DIALOG_TYPE_MINE, object, hero, 0, 0);
			return MAP_ACTION_OBJECT_UPDATED;
		}
		return MAP_ACTION_NONE;
	}	
	else if(object->object_type == OBJECT_WINDMILL) {
		auto mill = (flaggable_object_t*)object;

		bool visited = (mill->visited & 0x80) > 0;
		auto resource = (resource_e)((mill->visited & 0x70) >> 4);
		auto quantity = mill->visited & 0x0F;

		resource_group_t res;
		res.set_value_for_type(resource, quantity);
		get_player(hero->player).resources += res;

		mill->owner = hero->player;
		mill->visited = 0x80;

		int info_val = (resource << 4) | quantity;
		show_dialog_callback_fn(DIALOG_TYPE_WINDMILL, object, hero, visited, info_val);
	}
	else if(object->object_type == OBJECT_TELEPORTER) {
		auto tele = (teleporter_t*)object;
		if(tele->teleporter_type == teleporter_t::TELEPORTER_TYPE_ONE_WAY_EXIT) //nothing to do at an exit-only teleporter
			return MAP_ACTION_NONE;
	
		std::vector<teleporter_t*> destinations;
		for(auto o : map.objects) {
			if(!o || o->object_type != OBJECT_TELEPORTER || o == tele)
				continue;
	
			auto dst = (teleporter_t*)o;
			//check teleporter type/color compatibility
			if(dst->color != tele->color || dst->teleporter_type == teleporter_t::TELEPORTER_TYPE_ONE_WAY_ENTRANCE)
				continue;
				
			//check to see if destination is occupied or not
			auto dest_tile = map.get_interactable_coordinate_for_object(dst);
			bool occupied = false;
			for(auto& h : map.heroes) {
				if(h.second.x == dest_tile.x && h.second.y == dest_tile.y)
					occupied = true;
			}
				
			if(occupied)
				continue;
				
			destinations.push_back(dst);
		}

		if(!destinations.size()) //no valid destinations
			return MAP_ACTION_NONE;

		auto dest = destinations.at(rand() % destinations.size());
		auto teleport_dest_tile = map.get_interactable_coordinate_for_object(dest);
		hero->x = teleport_dest_tile.x;
		hero->y = teleport_dest_tile.y;
		
		return MAP_ACTION_HERO_TELEPORTED;
		}
	else if(object->object_type == OBJECT_WHIRLPOOL) {
		std::vector<interactable_object_t*> destinations;
		for(auto o : map.objects) {
			if(!o || o->object_type != OBJECT_WHIRLPOOL || o == object)
				continue;
	
			//check to see if destination is occupied or not
			auto dest_tile = map.get_interactable_coordinate_for_object(o);
			bool occupied = false;
			for(auto& h : map.heroes) {
				if(h.second.x == dest_tile.x && h.second.y == dest_tile.y)
					occupied = true;
			}
				
			if(occupied)
				continue;
				
			destinations.push_back(o);
		}

		if(!destinations.size()) //no valid destinations
			return MAP_ACTION_NONE;

		auto dest = destinations.at(rand() % destinations.size());
		auto teleport_dest_tile = map.get_interactable_coordinate_for_object(dest);
		hero->x = teleport_dest_tile.x;
		hero->y = teleport_dest_tile.y;
		
		return MAP_ACTION_HERO_TELEPORTED;
	}
	else if(object->object_type == OBJECT_WELL) {
		if(hero->has_object_been_visited(object)) {
			show_dialog_callback_fn(DIALOG_TYPE_WELL, object, hero, WELL_VISIT_RESULT_ALREADY_VISITED, 0);
			return MAP_ACTION_NONE;
		}
		else if(hero->hero_class == HERO_BARBARIAN) {
			int mp = 300;
			hero->movement_points += mp;
			show_dialog_callback_fn(DIALOG_TYPE_WELL, object, hero, WELL_VISIT_RESULT_BARBARIAN_MOVEMENT, mp);
			hero->set_visited_object(object);
			return MAP_ACTION_NONE;
		}
		else {
			//don't mark object as visited if mana is full
			if(hero->mana >= hero->get_maximum_mana()) {
				show_dialog_callback_fn(DIALOG_TYPE_WELL, object, hero, WELL_VISIT_RESULT_FULL_MANA, 0);
			}
			else {
				int mana_before = hero->mana;
				hero->mana = std::min(hero->get_maximum_mana(), (uint16_t)(hero->mana + (hero->get_maximum_mana() / 2)));
				hero->set_visited_object(object);
				show_dialog_callback_fn(DIALOG_TYPE_WELL, object, hero, WELL_VISIT_RESULT_MANA_RESTORED, hero->mana - mana_before);
			}
			return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
		}
	}
	else if(object->object_type == OBJECT_MONUMENT) {
		if(hero->has_object_been_visited(object)) {
			show_dialog_callback_fn(DIALOG_TYPE_MONUMENT, object, hero, 0, 0);
			return MAP_ACTION_NONE;
		}
		else {
			auto type = ((monument_t*)object)->monument_type;
		
			if(type == MONUMENT_TYPE_KING)
				hero->increase_attack(1);
			else if(type == MONUMENT_TYPE_NUN)
				hero->increase_knowledge(1);
			else if(type == MONUMENT_TYPE_HORNED_MAN)
				hero->increase_power(1);
			else if(type == MONUMENT_TYPE_ANGEL)
				hero->increase_defense(1);
			
			hero->set_visited_object(object);
			show_dialog_callback_fn(DIALOG_TYPE_MONUMENT, object, hero, 1, 0);
			return MAP_ACTION_NONE;
		}

	}
	
	return  map.interact_with_object(hero, object, *this);
}

void pick_scholar_reward(hero_t* hero, scholar_t* scholar) {
	if(!hero || !scholar)
		return;
	
	std::vector<scholar_t::reward_e> possible_rewards;
	std::vector<stat_e> possible_stat_rewards;
	if(hero->attack < 99)
		possible_stat_rewards.push_back(STAT_ATTACK);
	if(hero->defense < 99)
		possible_stat_rewards.push_back(STAT_ATTACK);
	if(hero->power < 99)
		possible_stat_rewards.push_back(STAT_ATTACK);
	if(hero->knowledge < 99)
		possible_stat_rewards.push_back(STAT_ATTACK);
	
	if(possible_stat_rewards.size())
		possible_rewards.push_back(scholar_t::SCHOLAR_REWARD_STATS);
	if(hero->talent_points_spent_count() < game_config::HERO_TALENT_POINTS)
		possible_rewards.push_back(scholar_t::SCHOLAR_REWARD_TALENT_POINT);
	if(hero->level < 99)
		possible_rewards.push_back(scholar_t::SCHOLAR_REWARD_EXPERIENCE);
	for(auto& sk : hero->skills) {
		if(sk.skill == SKILL_NONE || (sk.skill != SKILL_NONE && sk.level < 4)) {
			possible_rewards.push_back(scholar_t::SCHOLAR_REWARD_SKILL);
			break;
		}
		//fixme: case where the only enabled skills are already learned
		//else if(sk.skill == SKILL_NONE && utils::contains(hero->enabled_skills))
	}
	std::vector<spell_e> possible_spell_rewards;
	for(auto& sp : game_config::get_spells()) {
		if(hero->can_learn_spell(sp.id) && !utils::contains(hero->spellbook, sp.id))
			possible_spell_rewards.push_back(sp.id);
	}
	if(possible_spell_rewards.size())
		possible_rewards.push_back(scholar_t::SCHOLAR_REWARD_SPELL);
	if(!hero->are_secondary_skills_full_and_max_level())
		possible_rewards.push_back(scholar_t::SCHOLAR_REWARD_SKILL);
		
	//if the scholar reward is random, pick a random applicable reward
	//OR, if the reward is set by the editor, but the reward doesn't apply to the hero,
	//then we pick a random applicable reward
	if(scholar->reward_type == scholar_t::SCHOLAR_REWARD_RANDOM ||
	   !utils::contains(possible_rewards, scholar->reward_type)) {
		//pick a random _applicable_ reward
		if(possible_rewards.size())
			scholar->reward_type = possible_rewards[rand() % possible_rewards.size()];
		else
		   scholar->reward_type = scholar_t::SCHOLAR_REWARD_NONE;
	}
	
	//at this point, the reward _type_ is guaranteed to apply to the hero (or reward type is NONE)
	//but we still have to potentially fixup the reward subtype (e.g. the specific skill, spell, stat, etc)
	if(scholar->reward_type == scholar_t::SCHOLAR_REWARD_SPELL) {
		//we could have the case that the spell is set in the editor but
		//the hero can't learn it / already knows the spell
		if(scholar->reward_subtype.spell == SPELL_UNKNOWN ||
		   !utils::contains(possible_spell_rewards, scholar->reward_subtype.spell)) {
			scholar->reward_subtype.spell = possible_spell_rewards[rand() % possible_spell_rewards.size()];
		}
	}
	if(scholar->reward_type == scholar_t::SCHOLAR_REWARD_EXPERIENCE) {
		if(scholar->reward_property.magnitude == 0)
			scholar->reward_property.magnitude = utils::rand_range(1, 5);
	}
	if(scholar->reward_type == scholar_t::SCHOLAR_REWARD_SKILL) {
		//only pick skills the hero is allowed to learn
		assert(hero->enabled_skills.size());
		std::vector<skill_e> possible_skills;
		if(hero->get_number_of_open_skill_slots() == 0) { //we have to pick an existing skill to upgrade
			for(auto& sk : hero->skills)
				possible_skills.push_back(sk.skill);
		}
		else {
			possible_skills = hero->enabled_skills; //make a copy
		}
		
		//remove the possibility of selecting a maxed out skill
		for(auto& sk : hero->skills) {
			if(sk.level == 4) {
				auto it = std::find(possible_skills.begin(), possible_skills.end(), sk.skill);
				if(it != possible_skills.end())
					possible_skills.erase(it);
			}
		}
		assert(possible_skills.size());
		if(scholar->reward_subtype.skill == SKILL_NONE || !utils::contains(possible_skills, scholar->reward_subtype.skill))
			scholar->reward_subtype.skill = possible_skills[rand() % possible_skills.size()];
	}
	if(scholar->reward_type == scholar_t::SCHOLAR_REWARD_STATS) {
		//again, we have the (quite unlikely) possibility that the scholar reward is
		//set as a stat, but that stat is maxed out for the hero
		if(scholar->reward_subtype.stat == STAT_NONE
		   || !utils::contains(possible_stat_rewards, scholar->reward_subtype.stat)) {
			scholar->reward_subtype.stat = possible_stat_rewards[rand() % possible_stat_rewards.size()];
			scholar->reward_property.magnitude = 1;
		}
	}
}

bool game_t::pickup_object(hero_t* hero, interactable_object_t* object, int x, int y) {
	if(!hero || !object)
		return false;
	
	if(!map.are_tiles_adjacent(x, y, hero->x, hero->y)) //telekenisis?
		return false;
	
	//take movement points from the hero as if they were traveling across the current terrain tile,
	//unless the hero has pathfinding
	int move_cost = hero->get_secondary_skill_level(SKILL_PATHFINDING) ? 0 : map.get_terrain_movement_cost(hero, hero->x, hero->y);
	if(move_cost > hero->movement_points)
		return false;

	hero->movement_points -= move_cost;
	//now we need to check to potentially zero out the remainder of the hero's movement if it is low
	map.zero_hero_movement_points_if_low(hero, *this);

	if(object->object_type == OBJECT_RESOURCE) {
		auto res = (map_resource_t*)object;
		resource_group_t rg;
		rg.set_value_for_type(res->resource_type, res->min_quantity);
		get_player(hero->player).resources += rg; //fixme
		show_dialog_callback_fn(DIALOG_TYPE_PICKUP_RESOURCE, object, hero, 0, 0);
		remove_interactable_object_callback_fn(object, false);
		map.remove_interactable_object(object);
	}
	else if(object->object_type == OBJECT_TREASURE_CHEST) {
		show_dialog_callback_fn(DIALOG_TYPE_TREASURE_CHEST_CHOICE, object, hero, 0, 0);
	}
	else if(object->object_type == OBJECT_CAMPFIRE) {
		auto campfire = (campfire_t*)object;
		int goldval = (int)campfire->gold_value * 100;
		auto resval = campfire->resource_value;
		auto restype = (resource_e)(RESOURCE_WOOD + rand() % 4);
		
		resource_group_t res;
		res.set_value_for_type(RESOURCE_GOLD, goldval);
		res.set_value_for_type(campfire->resource_type, resval);
		get_player(hero->player).resources += res;
		
		show_dialog_callback_fn(DIALOG_TYPE_PICKUP_CAMPFIRE, object, hero, goldval, resval);

		remove_interactable_object_callback_fn(object, false);
		map.remove_interactable_object(object);
	}
	else if(object->object_type == OBJECT_SCHOLAR) {
		pick_scholar_reward(hero, (scholar_t*)object);
		show_dialog_callback_fn(DIALOG_TYPE_SCHOLAR_CHOICE, object, hero, 0, 0);
	}
	else if(object->object_type == OBJECT_PANDORAS_BOX) {
		show_dialog_callback_fn(DIALOG_TYPE_PANDORAS_BOX_CHOICE, object, hero, 0, 0);
	}
	else if(object->object_type == OBJECT_ARTIFACT) {
		auto art = (map_artifact_t*)object;
		
		if(hero->is_inventory_full(art->artifact_id)) {
			show_dialog_callback_fn(DIALOG_TYPE_PICKUP_ARTIFACT, object, hero, -1, 0);
			return false;
		}

		show_dialog_callback_fn(DIALOG_TYPE_PICKUP_ARTIFACT, object, hero, 0, 0);
		hero->pickup_artifact(art->artifact_id);
		
		remove_interactable_object_callback_fn(object, false);
		map.remove_interactable_object(object);
		update_visibility();
	}
	else if(object->object_type == OBJECT_SEA_CHEST) {
		show_dialog_callback_fn(DIALOG_TYPE_PICKUP_SEA_CHEST, object, hero, 0, 0);
		
		remove_interactable_object_callback_fn(object, false);
		map.remove_interactable_object(object);
	}
	else if(object->object_type == OBJECT_SEA_BARRELS) {
		show_dialog_callback_fn(DIALOG_TYPE_PICKUP_SEA_BARRELS, object, hero, 0, 0);
		
		remove_interactable_object_callback_fn(object, false);
		map.remove_interactable_object(object);
	}
	else if(object->object_type == OBJECT_FLOATSAM) {
		show_dialog_callback_fn(DIALOG_TYPE_PICKUP_FLOATSAM, object, hero, 0, 0);
		
		remove_interactable_object_callback_fn(object, false);
		map.remove_interactable_object(object);
	}
	else if(object->object_type == OBJECT_SHIPWRECK_SURVIVOR) {
		show_dialog_callback_fn(DIALOG_TYPE_PICKUP_SHIPWRECK_SURVIVOR, object, hero, 0, 0);
		
		remove_interactable_object_callback_fn(object, false);
		map.remove_interactable_object(object);
	}
	
	return true;
}

map_action_e game_t::make_dialog_choice(hero_t* hero, interactable_object_t* object, dialog_type_e dialog_type, dialog_choice_e choice) {
	if(dialog_type == DIALOG_TYPE_NONE) //no action required
		return MAP_ACTION_NONE;
	
	//dialog_type
	if(dialog_type == DIALOG_TYPE_REFUGEE_CAMP_CHOICE && choice == DIALOG_CHOICE_ACCEPT) {
		show_dialog_callback_fn(DIALOG_TYPE_REFUGEE_CAMP_RECRUIT, object, hero, 1, 0);
		return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
	}
	else if(dialog_type == DIALOG_TYPE_PANDORAS_BOX_CHOICE && choice == DIALOG_CHOICE_ACCEPT) {
		show_dialog_callback_fn(DIALOG_TYPE_PANDORAS_BOX_REWARD, object, hero, 1, 0);
		return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
	}
	else if(dialog_type == DIALOG_TYPE_PANDORAS_BOX_REWARD) {
		if(!hero || !object)
			return MAP_ACTION_NONE;

		auto box = (pandoras_box_t*)object;
		for(auto& reward: box->rewards) {
			switch(reward.type) {
				case pandoras_box_t::PANDORAS_BOX_REWARD_EXPERIENCE: {
					give_hero_xp(hero, reward.magnitude);
					break;
				}
			}
		}
		
		remove_interactable_object_callback_fn(object, false);
		map.remove_interactable_object(object);
	}
	else if(dialog_type == DIALOG_TYPE_MAGIC_SHRINE_VISIT) {
		auto shrine = (shrine_t*)object;
		hero->set_visited_object(object);
		if(hero->can_learn_spell(shrine->spell))
			hero->learn_spell(shrine->spell);
	}
	else if(dialog_type == DIALOG_TYPE_HUT_OF_THE_MAGI) {
		for(const auto obj : map.objects) {
			if(!obj || obj->object_type != OBJECT_EYE_OF_THE_MAGI)
				continue;

			auto eye = (eye_of_the_magi_t*)obj;
			auto coord = map.get_interactable_coordinate_for_object(eye);
			reveal_area(hero->player, coord.x, coord.y, eye->reveal_radius, eye->reveal_radius);
		}

		return MAP_ACTION_SHOW_EYES_OF_THE_MAGI;
	}
	else if(dialog_type == DIALOG_TYPE_WARRIORS_TOMB_CHOICE) {
		if(!hero || !object || object->object_type != OBJECT_WARRIORS_TOMB)
			return MAP_ACTION_NONE;
		
		auto tomb = (warriors_tomb_t*)object;
		//todo: check to see if hero's backpack is full
		hero->pickup_artifact(tomb->artifact);
		hero->add_temporary_morale_effect(MORALE_EFFECT_WARRIORS_TOMB, -3);
		show_dialog_callback_fn(DIALOG_TYPE_WARRIORS_TOMB_REWARD, object, hero, 1, 0);
		tomb->visited = true;
		tomb->artifact = ARTIFACT_NONE;
		return MAP_ACTION_OBJECT_UPDATED;
	}
	if(dialog_type == DIALOG_TYPE_TREASURE_CHEST_CHOICE) {
		auto chest = (treasure_chest_t*)object;
		if(choice == DIALOG_CHOICE_1) { //take the gold
			resource_group_t gold;
			auto gold_value = 500 + chest->value * 500 + ((get_month() - 1) * 2000);
			gold.set_value_for_type(RESOURCE_GOLD, gold_value);
			get_player(hero->player).resources += gold;
		}
		else if(choice == DIALOG_CHOICE_2) { //take the xp
			auto xp_value = chest->value * 500 + (hero->level > 8 ? hero->experience * .1 : 0);
			give_hero_xp(hero, xp_value);
		}
		//auto& tile = map.get_tile_for_interactable_object(object);
		remove_interactable_object_callback_fn(object, false);
		map.remove_interactable_object(object);
	}
//	else if(dialog_type == OBJECT_MAP_MONSTER) {
//		
//	}
	else if(dialog_type == DIALOG_TYPE_PICKUP_CAMPFIRE) {
		remove_interactable_object_callback_fn(object, false);
		map.remove_interactable_object(object);
	}
	else if(dialog_type == DIALOG_TYPE_ARENA_VISIT && !hero->has_object_been_visited(object)) {
		if(choice == 0) { //attack
			hero->attack += 2;
		}
		else { //defense
			hero->defense += 2;
		}
		hero->set_visited_object(object);
	}
	else if(dialog_type == DIALOG_TYPE_GAZEBO_VISIT && !hero->has_object_been_visited(object)) {
		auto xp_value = 1250 + (hero->level > 5 ? hero->level * 50 : 0);
		give_hero_xp(hero, xp_value);
		hero->set_visited_object(object);
	}
	else if((dialog_type == DIALOG_TYPE_SCHOOL_OF_WAR_CHOICE || dialog_type == DIALOG_TYPE_SCHOOL_OF_MAGIC_CHOICE)) {
		if(hero->has_object_been_visited(object)) //hero has already purchased training from this school
			return MAP_ACTION_NONE;
		
		if(get_player(hero->player).resources.get_value_for_type(RESOURCE_GOLD) < 1000)
			return MAP_ACTION_NONE;
		
		resource_group_t res;
		res.set_value_for_type(RESOURCE_GOLD, 1000);
		get_player(hero->player).resources -= res;
		
		assert(choice == DIALOG_CHOICE_1 || choice == DIALOG_CHOICE_2);
		
		if(choice == DIALOG_CHOICE_1) //power/attack
			dialog_type == DIALOG_TYPE_SCHOOL_OF_WAR_CHOICE ? hero->increase_attack(1) : hero->increase_power(1);
		else if(choice == DIALOG_CHOICE_2) //knowledge/defense
			dialog_type == DIALOG_TYPE_SCHOOL_OF_WAR_CHOICE ? hero->increase_defense(1) :hero->increase_knowledge(1);
		
		hero->set_visited_object(object);
	}
	else if(dialog_type == DIALOG_TYPE_PYRAMID_VISIT && choice == DIALOG_CHOICE_ACCEPT) {
		auto pyramid = (pyramid_t*)object;
		if(pyramid->visited) {
			hero->add_temporary_luck_effect(LUCK_EFFECT_NONE, -3);
			show_dialog_callback_fn(DIALOG_TYPE_PYRAMID_REWARD, pyramid, hero, -1, 0);
			return MAP_ACTION_NONE;
		}
		
		army_t defenders;
		for(int i = 0; i < 5; i++) {
			defenders.troops[i].unit_type = UNIT_VAMPIRE;
			defenders.troops[i].stack_size = 5;
		}

		battle.init_hero_creature_bank_battle(hero, defenders, object);

		return MAP_ACTION_BATTLE_INITIATED;
	}
	else if(dialog_type == DIALOG_TYPE_GRAVEYARD_VISIT && choice == DIALOG_CHOICE_ACCEPT) {
		auto graveyard = (graveyard_t*)object;
		if(graveyard->visited) {
			hero->add_temporary_morale_effect(MORALE_EFFECT_WARRIORS_TOMB, -2);
			show_dialog_callback_fn(DIALOG_TYPE_GRAVEYARD_RESULT, graveyard, hero, 0, 0);
			return MAP_ACTION_NONE;
		}

		army_t defenders;
		for(int i = 0; i < 5; i++) {
			defenders.troops[i].unit_type = UNIT_GHOUL;
			defenders.troops[i].stack_size = 5;
		}

		battle.init_hero_creature_bank_battle(hero, defenders, object);
		battle.environment_type = BATTLEFIELD_ENVIRONMENT_GRAVEYARD;

		return MAP_ACTION_BATTLE_INITIATED;
	}
	else if(dialog_type == DIALOG_TYPE_WITCH_HUT_CHOICE && choice == DIALOG_CHOICE_ACCEPT) { //implies player clicked 'ok'/'yes' to witch hut dialog
		auto hut = (witch_hut_t*)object;
		if(hero->has_object_been_visited(object))
			return MAP_ACTION_NONE;
		
		if((hero->get_secondary_skill_base_level(hut->skill) == 0) && hero->get_number_of_open_skill_slots() && hero->can_learn_skill(hut->skill)) {
			//hero does not have skill and has an open slot and can learn skill
			for(uint i = 0; i < game_config::HERO_SKILL_SLOTS; i++) {
				if(hero->skills[i].skill == SKILL_NONE) {
					hero->skills[i].skill = hut->skill;
					hero->skills[i].level = 1;
					hero->move_necromancy_to_top_slot();
					hero->set_visited_object(object);
					return MAP_ACTION_NONE;
				}
			}
		}
		else if((hero->get_secondary_skill_base_level(hut->skill) < 4) && hut->allows_upgrade) { //upgrade
			for(uint i = 0; i < game_config::HERO_SKILL_SLOTS; i++) {
				if(hero->skills[i].skill == hut->skill) {
					hero->skills[i].level++;
					hero->set_visited_object(object);
					return MAP_ACTION_NONE;
				}
			}
		}
	}
	else if(dialog_type == DIALOG_TYPE_SCHOLAR_CHOICE && choice == DIALOG_CHOICE_ACCEPT) {
		auto scholar = (scholar_t*)object;
		//when this is called, the scholar should have an appropriate reward type set.
		auto reward = scholar->reward_type;
		if(reward == scholar_t::SCHOLAR_REWARD_SPELL)
			hero->learn_spell(scholar->reward_subtype.spell);
		if(reward == scholar_t::SCHOLAR_REWARD_TALENT_POINT)
			show_hero_levelup_callback_fn(hero, SKILL_NONE, SKILL_NONE, SKILL_NONE, STAT_NONE, true);
		if(reward == scholar_t::SCHOLAR_REWARD_EXPERIENCE)
			give_hero_xp(hero, scholar->reward_property.magnitude * 500);
		if(reward == scholar_t::SCHOLAR_REWARD_SKILL) {
			bool upgraded = false;
			for(auto& sk : hero->skills) {
				if(sk.skill == scholar->reward_subtype.skill) {
					sk.level++;
					upgraded = true;
					break;
				}
			}
			if(!upgraded) {
				for(auto& sk : hero->skills) {
					if(sk.skill == SKILL_NONE) {
						sk.skill = scholar->reward_subtype.skill;
						sk.level = 1;
						hero->move_necromancy_to_top_slot();
						break;
					}
				}
			}
		}
		if(reward == scholar_t::SCHOLAR_REWARD_STATS) {
			switch(scholar->reward_subtype.stat) {
				case STAT_ATTACK: hero->increase_attack(scholar->reward_property.magnitude); break;
				case STAT_DEFENSE: hero->increase_defense(scholar->reward_property.magnitude); break;
				case STAT_POWER: hero->increase_power(scholar->reward_property.magnitude); break;
				case STAT_KNOWLEDGE: hero->increase_knowledge(scholar->reward_property.magnitude); break;
				default: assert(0);//return MAP_ACTION_NONE;
			}
		}
		
		remove_interactable_object_callback_fn(object, false);
		map.remove_interactable_object(object);
	}
	
	return MAP_ACTION_NONE;
}

bool game_t::make_hero_levelup_selection(hero_t* hero, skill_e skill_selection, talent_e talent_selection) {
	if(!hero)
		return false;
	
	//todo: validation
	
	//if we already have the skill, increment the level
	if(hero->get_secondary_skill_base_level(skill_selection)) {
		for(auto& sk : hero->skills) {
			if(sk.skill == skill_selection) {
				sk.level++;
				break;
			}
		}
	}
	else { //don't have the skill yet
		for(auto& sk : hero->skills) {
			if(sk.skill == SKILL_NONE) {
				sk.skill = skill_selection;
				sk.level = 1;
				break;
			}
		}
	}
	hero->move_necromancy_to_top_slot();
	
	if(talent_selection != TALENT_NONE && hero->is_talent_unlocked(talent_selection))
		hero->give_talent(talent_selection);
	
	process_hero_unallocated_xp(hero);

	//if the hero upgrades their scouting level we need to update the fog
	update_visibility(hero->player);
	
	return true;
}


spell_result_e game_t::cast_adventure_spell(hero_t* caster, spell_e spell_id, coord_t target) {
	if(!caster || spell_id == SPELL_UNKNOWN)
		return SPELL_RESULT_ERROR;
	
	const auto& spell = game_config::get_spell(spell_id);
	
	uint16_t mana_cost = caster->get_spell_cost(spell_id);
	
	if(caster->mana < mana_cost)
		return SPELL_RESULT_INSUFFICIENT_MANA;
	
	if(spell_id == SPELL_CONJURE_SHIP) {
		int dx[] = { 1, 0, -1, 0, 1, 1, -1, -1 };
		int dy[] = { 0, 1, 0, -1, 1, -1, 1, -1 };

		coord_t selected_tile = {-1, -1};
		bool near_water = false;
		
		for (int i = 0; i < 8; i++) {
			int x = caster->x + dx[i];
			int y = caster->y + dy[i];
			
			//tile invalid, impassable, or not water
			if(!map.tile_valid(x, y) || !map.get_tile(x, y).passability || map.get_tile(x, y).terrain_type != TERRAIN_WATER)
				continue;
			
			near_water = true;
			
			//tile is occupied by something
			if(map.get_tile(x, y).is_interactable())
				continue;
			
			//prefer orthogonal tiles to diagonal ones
			if(!map.tile_valid(selected_tile.x, selected_tile.y) || map.are_tiles_diagonal(caster->x, caster->y, selected_tile.x, selected_tile.y)) {
				selected_tile.x = x;
				selected_tile.y = y;
			}
		}
		
		if(!map.tile_valid(selected_tile.x, selected_tile.y)) {
			if(near_water)
				return SPELL_RESULT_NO_VALID_DESTINATION;
			else
				return SPELL_RESULT_NOT_NEAR_WATER;
		}
		
		
		
		
		return SPELL_RESULT_OK;
	}
	if(spell_id == SPELL_RECALL) {
		//we need to check if the starting coordinate is unoccupied
		int dest_x = caster->start_of_day_x;
		int dest_y = caster->start_of_day_y;
		if(!map.tile_valid(dest_x, dest_y))
			return SPELL_RESULT_ERROR;

		auto existing = map.get_hero_at_tile(dest_x, dest_y);
		if(existing && existing != caster)
			return SPELL_RESULT_INVALID_TARGET;

		caster->x = caster->start_of_day_x;
		caster->y = caster->start_of_day_y;
		caster->mana -= mana_cost;

		auto obj = map.get_interactable_object_for_tile(dest_x, dest_y);
		if(obj && obj->object_type == OBJECT_MAP_TOWN)
			map.hero_visit_town(caster, (town_t*)obj);

		return SPELL_RESULT_OK;
	}

	if(spell_id == SPELL_MANA_BATTERY) {
		int extra = spell.multiplier[1].get_value(caster->get_effective_power(), caster->get_spell_effect_multiplier(spell_id));
		extra = std::round(extra / (float)(caster->mana_battery_casts_today + 1));
		caster->movement_points += extra;
		caster->mana_battery_casts_today++;
		caster->mana -= mana_cost;
		return SPELL_RESULT_OK;
	}

	if(spell_id == SPELL_WORMHOLE) {
		int max_range = spell.multiplier[0].get_value(caster->get_effective_power(), caster->get_spell_effect_multiplier(spell_id));
		if(caster->wormhole_casts_today >= spell.multiplier[2].get_value(0))
			return SPELL_RESULT_ERROR;
		
		if(!map.tile_valid(target.x, target.y) || !map.get_tile(target.x, target.y).passability || map.get_tile(target.x, target.y).is_interactable())
			return SPELL_RESULT_INVALID_TARGET;
		
		int dist = abs(target.x - caster->x) + abs(target.y - caster->y);
		if(dist > max_range)
			return SPELL_RESULT_INVALID_TARGET;
		
		auto route = map.get_route_ignoring_blockables(caster, target.x, target.y, nullptr);
		
		if(route.empty())
			return SPELL_RESULT_INVALID_TARGET;
		
		caster->x = target.x;
		caster->y = target.y;
		caster->wormhole_casts_today++;
		caster->mana -= mana_cost;

		return SPELL_RESULT_OK;
	}
	
	return SPELL_RESULT_ERROR;
}

bool game_t::meet_heroes(hero_t* left, hero_t* right) {
	if(!left || !right)
		return false;

	if(!map.are_tiles_adjacent(left->x, left->y, right->x, right->y))
		return false;

	int max_int_level = std::max(left->get_secondary_skill_level(SKILL_INTELLIGENCE), right->get_secondary_skill_level(SKILL_INTELLIGENCE));

	if(max_int_level > 0) {

	}

	return true;
}

resource_group_t get_starting_resources(uint difficulty, bool computer) {
	if(computer) { //ai starting resources
		switch(difficulty) {
			case 0: return resource_group_t(0, 0, 0, 0, 0, 0, 2500); //easy
			case 1: return resource_group_t(15, 15, 5, 5, 5, 5, 10000); //normal
			case 2: return resource_group_t(20, 20, 5, 5, 5, 5, 10000); //hard
			case 3: return resource_group_t(20, 20, 7, 7, 7, 7, 15000); //expert
			case 4: return resource_group_t(30, 30, 10, 10, 10, 10, 15000); //impossible
		}
	}
	else { //human starting resources
		switch(difficulty) {
			case 0: return resource_group_t(30, 30, 10, 10, 10, 10, 15000); //easy
			case 1: return resource_group_t(15, 15, 5, 5, 5, 5, 10000); //normal
			case 2: return resource_group_t(10, 10, 2, 2, 2, 2, 7500); //hard
			case 3: return resource_group_t(5, 5, 0, 0, 0, 0, 5000); //expert
			case 4: return resource_group_t(0, 0, 0, 0, 0, 0, 0); //impossible
		}
	}
	
	return resource_group_t(30, 30, 10, 10, 10, 10, 15000);
}

void game_t::setup(game_configuration_t& game_settings, uint64_t seed) {
	date = 0;
	int num_players = game_settings.player_count; //map.players;
	
	players.clear();
	players.resize(num_players);
	auto heroes = game_config::get_heroes();
	std::shuffle(heroes.begin(), heroes.end(), std::default_random_engine(seed));
	
	uint valid_players = 0;
	for(uint i = 0; i < game_config::MAX_NUMBER_OF_PLAYERS; i++) {
		auto pnum = game_settings.player_configs[i].player_number;
		if(pnum == PLAYER_NONE)
			continue;
		
		
		if(valid_players >= players.size())
			break;
		
		players[valid_players].player_number = pnum;
		players[valid_players].is_human = game_settings.player_configs[i].is_human;
		valid_players++;
	}	
	
	for(auto& p : players) {
		p.resources = get_starting_resources(game_settings.difficulty.difficulty, !p.is_human);
		
		town_t* main_town = nullptr;
		for(const auto obj : map.objects) {
			if(!obj || obj->object_type != OBJECT_MAP_TOWN)
				continue;
			
			auto town = (town_t*)obj;
			if(town->player != p.player_number)
				continue;
			
			main_town = town;
			break;
		}
		
		if(!main_town)
			continue;

		auto& cfg = game_settings.player_configs[p.player_number - 1];
		auto hero_id = cfg.starting_hero_id;
		if(cfg.selected_class == -1) {
			cfg.selected_class = (rand() % 6);
			cfg.was_class_random = true;
		}

		main_town->town_type = town_t::hero_class_to_town_type((hero_class_e)cfg.selected_class);

		if(hero_id == -1) {
			std::vector<uint> pool;
			for(uint i = 0; i < game_config::get_heroes().size(); i++) {
				const auto& h = game_config::get_heroes()[i];
				if(h.hero_class == cfg.selected_class)
					pool.push_back(i);
			}
			
			assert(pool.size());
			hero_id = pool[rand() % pool.size()];
			cfg.was_hero_random = true;
		}
		
		auto hero = game_config::get_heroes()[hero_id];
		hero.player = p.player_number;
		auto pos = map.get_interactable_coordinate_for_object(main_town);
		hero.x = pos.x;
		hero.y = pos.y;
		hero.init_hero(date, map.get_tile(hero.x, hero.y).terrain_type);
		auto id = get_new_hero_id();
		hero.id = id;
		map.heroes[id] = hero;
		map.hero_visit_town(&map.heroes[id], main_town);
		
		cfg.selected_class = cfg.selected_class;
		cfg.starting_hero_id = hero_id;
	}

	generate_new_week_recruitable_heroes();
	
	game_configuration = game_settings;
	
//	game_configuration.difficulty = game_settings.difficulty;
//	game_configuration.player_count = game_settings.player_count;
//
//	for(auto& pc : game_settings.player_configs)
	
	setup_scripting();

	map.initialize_map(game_settings, seed);
}

int16_t game_t::get_new_hero_id() const {
	int16_t id = 0;
	
	std::set<int16_t> existing_ids;
	
	for(const auto& h : map.heroes)
		existing_ids.insert(h.second.id);
	
	for(const auto& pl : players)
		for(const auto& h : pl.heroes_available_to_recruit)
			existing_ids.insert(h.id);
	
	while(existing_ids.count(id))
		id++;
	
	return id;
}

hero_t* game_t::get_hero_by_id(uint16_t hero_id) {
	if(map.heroes.count(hero_id))
		return &map.heroes[hero_id];
	
	for(auto& pl : players)
		for(auto& h : pl.heroes_available_to_recruit)
			if(h.id == hero_id)
				return &h;
	
	return nullptr;
}

bool game_t::setup_scripting() {
	_lua_state = luaL_newstate();
	register_lua_functions(_lua_state, this);
	
	return true;
}

hero_t game_t::generate_new_recruitable_hero(const player_t& player) {
	std::vector<hero_t> heroes;
	for(auto& h : game_config::get_heroes()) {
		bool conflict = false;
		for(auto& existing : map.heroes) {
			if(existing.second.portrait == h.portrait)
				conflict = true;
		}
		for(auto& p : players) {
			for(auto& recruit : p.heroes_available_to_recruit) {
				if(recruit.portrait == h.portrait)
					conflict = true;
			}
		}
		
		if(!conflict)
			heroes.push_back(h);
	}
	
	auto seed = (unsigned)std::chrono::system_clock::now().time_since_epoch().count();
	std::shuffle(heroes.begin(), heroes.end(), std::default_random_engine(seed));
	
	assert(heroes.size()); //could have an 'out of heroes' situation here, fixme
	auto& hero = heroes.back();
	if(map.tile_valid(hero.x, hero.y))
		hero.init_hero(date, map.get_tile(hero.x, hero.y).terrain_type);
	else
		hero.init_hero(date, TERRAIN_UNKNOWN);
	
	hero.experience = 50 + rand() % 250;
	hero.id = get_new_hero_id();
	
	return hero;
}

hero_class_e get_random_adjacent_class(hero_class_e hclass) {
	switch(hclass) {
		case HERO_KNIGHT:
			return (rand() % 2 == 1) ? HERO_SORCERESS : HERO_WIZARD;
		case HERO_SORCERESS:
			return (rand() % 2 == 1) ? HERO_KNIGHT : HERO_WIZARD;
		case HERO_WIZARD:
			return (rand() % 2 == 1) ? HERO_SORCERESS : HERO_KNIGHT;
		case HERO_WARLOCK:
			return (rand() % 2 == 1) ? HERO_NECROMANCER : HERO_BARBARIAN;
		case HERO_NECROMANCER:
			return (rand() % 2 == 1) ? HERO_WARLOCK : HERO_BARBARIAN;
		case HERO_BARBARIAN:
			return (rand() % 2 == 1) ? HERO_WARLOCK : HERO_NECROMANCER;
	}

	return HERO_KNIGHT;
}

void game_t::generate_new_week_recruitable_heroes() {
	std::map<hero_class_e, std::deque<const hero_t*>> heroes_by_class;

	for(const auto& h : game_config::get_heroes()) {
		bool hero_exists_in_map = false;
		for(const auto& existing_hero : map.heroes) {
			if(h.portrait == existing_hero.second.portrait) {
				hero_exists_in_map = true;
				break;
			}
		}
		if(hero_exists_in_map)
			continue;

		heroes_by_class[h.hero_class].push_back(&h);
	}

	for(auto& class_heroes : heroes_by_class)
		std::shuffle(class_heroes.second.begin(), class_heroes.second.end(), std::mt19937_64(time(nullptr)));

	for(auto& pl : players) {
		const auto& pcfg = get_player_configuration(pl.player_number);
		//todo: keep track of the previous week's offered heroes, and make sure we don't select any repeats
		//offer 1-2 heroes of a player's class, one 'adjacently' aligned hero, and a random one (if one remains)
		auto primary_class = (hero_class_e)pcfg.selected_class;
		auto adjacent_class = get_random_adjacent_class(primary_class);
		bool picked_adjacent = false;
		int num_primary = (rand() % 2 == 1) ? 2 : 1;
		for(int i = 0; i < player_t::NUMBER_OF_RECRUITABLE_HEROES; i++) {
			hero_t next_hero;
			if(i < num_primary && heroes_by_class[primary_class].size()) {
				next_hero = *heroes_by_class[primary_class].back();
				heroes_by_class[primary_class].pop_back();
			}
			else if(!picked_adjacent && heroes_by_class[adjacent_class].size()) {
				next_hero = *heroes_by_class[adjacent_class].back();
				heroes_by_class[adjacent_class].pop_back();
				picked_adjacent = true;
			}
			else if(picked_adjacent || !heroes_by_class[adjacent_class].size()) {
				next_hero = generate_new_recruitable_hero(pl);
			}

			next_hero.init_hero(date, TERRAIN_UNKNOWN);
			next_hero.experience = 50 + rand() % 250;
			next_hero.id = get_new_hero_id();
			pl.heroes_available_to_recruit[i] = next_hero;
		}
	}
}

bool game_t::buy_troops_at_object(player_e player_num, interactable_object_t* object, uint16_t count) {
	if(!object)
		return false;
	
	troop_t* troops = nullptr;
	if(object->object_type == OBJECT_REFUGEE_CAMP) {
		troops = &((refugee_camp_t*)object)->available_troops;
	}
	
	if(!troops)
		return false;
	
	if(count > troops->stack_size)
		return false;
	
	auto pos = map.get_interactable_coordinate_for_object(object);
	hero_t* hero = map.get_hero_at_tile(pos.x, pos.y);
	
	if(!hero)
		return false;
	
	auto& player = get_player(player_num);
	auto& creature = game_config::get_creature(troops->unit_type);
	auto troop_cost = creature.cost * count;
	
	if(player.resources < troop_cost)
		return false;
	
	player.resources -= troop_cost;
	troops->stack_size -= count;

	troop_t troops_to_buy(troops->unit_type, count);
	if(map.can_add_troop_to_group(troops_to_buy, hero->troops)) {
		map.add_troop_to_group_combining(troops_to_buy, hero->troops);
	}
	else { //troops won't fit, show distribute creatures window
		//show_distribute_creatures_window(hero, &troops_to_buy);
	}
	
	return true;
}

bool game_t::buy_troops_at_town(player_e player_num, town_t* town, uint slot_num, uint16_t count) {
	if(!town || count == 0 || slot_num >= game_config::HERO_TROOP_SLOTS || town->player != player_num)
		return false;
	
	auto& troop = town->available_troops[slot_num];
	
	if(count > troop.stack_size)
		return false;
	
	auto& player = get_player(player_num);
	auto& creature = game_config::get_creature(troop.unit_type);
	auto troop_cost = creature.cost * count;
	
	if(player.resources < troop_cost)
		return false;
	
	player.resources -= troop_cost;
	
	//todo: refactor this to troop_t::combine_troops or similar
	if(town->garrisoned_hero) {
		bool added = false;
		for(auto& tr : town->garrisoned_hero->troops) {
			if(tr.unit_type == troop.unit_type) {
				tr.stack_size += count;
				troop.stack_size -= count;
				added = true;
				break;
			}
		}
		if(!added) {
			for(auto& tr : town->garrisoned_hero->troops) {
				if(!tr.is_empty())
					continue;
				
				tr.unit_type = troop.unit_type;
				tr.stack_size = count;
				troop.stack_size -= count;
				added = true;
				break;
			}
		}
		return added;
	}
	else {
		//map.add_troops_to_garrison(
		bool added = false;
		for(auto& tr : town->garrison_troops) {
			if(tr.unit_type == troop.unit_type) {
				tr.stack_size += count;
				troop.stack_size -= count;
				added = true;
				break;
			}
		}
		if(!added) {
			for(auto& tr : town->garrison_troops) {
				if(!tr.is_empty())
					continue;
				
				tr.unit_type = troop.unit_type;
				tr.stack_size = count;
				troop.stack_size -= count;
				added = true;
				break;
			}
		}
		return added;
	}
	
	return false;
}

hero_t* game_t::recruit_hero_at_town(player_e player, int selection, town_t* town) {
	if(selection < 0 || selection > 3 || !town || !player_valid(player))
		return nullptr;
	
	if(town->visiting_hero && town->garrisoned_hero)
		return nullptr;
	
	//more checks
	uint count = 0;
	for(auto& h : map.heroes) {
		if(h.second.player == player && !h.second.garrisoned)
			count++;
	}
	
	if(count >= game_config::MAX_NUMBER_OF_HEROES)
		return nullptr;	
	
	auto& pl = get_player(player);
	if(selection >= pl.heroes_available_to_recruit.size())
		return nullptr;

	hero_t new_hero = pl.heroes_available_to_recruit[selection];
	
	uint hero_cost = 2500 + (new_hero.level * 500);
	if(pl.resources.get_value_for_type(RESOURCE_GOLD) < hero_cost)
		return nullptr;
	
	if(map.heroes.count(new_hero.id))
		return nullptr;

	new_hero.player = player;
	map.heroes[new_hero.id] = new_hero;
	auto& hero = map.heroes[new_hero.id];
	auto offset = map.get_interactable_offset_for_object(town);
	hero.x = town->x + offset.x;
	hero.y = town->y + offset.y;
	
	if(town->visiting_hero) {
		//todo merge troops
		//if(!game->map.garrison_hero(&hero, town))
		//   return false;
		town->garrisoned_hero = &hero;
		hero.garrisoned = true;
	}
	else
		town->visiting_hero = &hero;
	
	hero.in_town = true;

	for(auto sp : town->mage_guild_spells)
		hero.learn_spell(sp);

	resource_group_t cost;
	cost.set_value_for_type(RESOURCE_GOLD, hero_cost);
	pl.resources -= cost;
	
	auto new_recruitable_hero = generate_new_recruitable_hero(pl);
	pl.heroes_available_to_recruit[selection] = new_recruitable_hero;
	
	return &hero;
}

uint game_t::read_save_game_header_stream(QDataStream& stream, save_game_header_t& header) {
	header.save_name = stream_read_string(stream);
	header.map_name = stream_read_string(stream);
	stream >> header.started_date;
	stream >> header.last_save_time;
	stream >> header.save_points;
	stream >> header.total_days;
	//stream_read_array(stream, header.save_point_offsets);
	
	return 0;
}

uint game_t::write_save_game_header_stream(QDataStream& stream, const save_game_header_t& header) {
	stream_write_string(stream, header.save_name);
	stream_write_string(stream, header.map_name);
	stream << header.started_date;
	stream << header.last_save_time;
	stream << header.save_points;
	stream << header.total_days;
	//stream_write_vector(stream, header.save_point_offsets);
	
	return 0;
}

uint game_t::read_game_info(QDataStream& stream, game_t* game) {
	stream >> game->date;
	stream >> game->game_configuration.difficulty.difficulty;
	stream >> game->game_configuration.difficulty.challenge;
	stream >> game->game_configuration.difficulty.hardcore;
	stream >> game->game_configuration.difficulty.king_of_the_hill;
	stream >> game->game_configuration.player_count;
	stream_read_array(stream, game->game_configuration.player_configs);
	stream_read_vector(stream, game->players);
	
	return 0;
}

uint game_t::write_game_info(QDataStream& stream, const game_t* game) {
	stream << game->date;
	stream << game->game_configuration.difficulty.difficulty;
	stream << game->game_configuration.difficulty.challenge;
	stream << game->game_configuration.difficulty.hardcore;
	stream << game->game_configuration.difficulty.king_of_the_hill;
	stream << game->game_configuration.player_count;
	stream_write_vector(stream, game->game_configuration.player_configs);
	stream_write_vector(stream, game->players);
	
	return 0;
}

uint game_t::save_game(const std::string& filename) {
	std::string filepath = filename;
	QFile file(QString::fromStdString(filepath));
	
	//first we check to see if an existing save game with this name exists
	if(file.exists()) {

		if(!file.open(QIODevice::ReadWrite))
			return 1;//MAP_COULD_NOT_OPEN_FILE;


		QDataStream stream(&file);

		save_game_header_t header;
		auto result = read_save_game_header_stream(stream, header);
		if(result != 0)
			return -2;

		//update the header info
		header.save_points++;
		header.total_days = date + 1;
		header.last_save_time = QDateTime::currentDateTime();
		
		//seek back to the start of the file, then write the updated header
		auto res = file.seek(0);
		write_save_game_header_stream(stream, header);

		//append the save point info
		res = file.seek(file.size());
		stream << date;
		stream << (uint16_t)map.heroes.size();
		for(const auto& hero : map.heroes) {
			stream << hero.second;
		}
		
		stream << (uint32_t)map.objects.size();
		for(auto obj : map.objects) {
			//when we write an existing game, some map objects can be null(removed, e.g.resources)
			if(!obj) {
				stream << OBJECT_UNKNOWN;
				continue;
			}

			QBuffer buffer;
			buffer.open(QIODevice::WriteOnly);
			QDataStream object_stream(&buffer);

			//write object data to temporary stream
			object_stream << obj->asset_id << obj->x << obj->y << obj->z << obj->width << obj->height;
			obj->write_data(object_stream);

			//write size followed by the buffer contents to main stream
			uint16_t total_size = buffer.size();
			stream << obj->object_type;
			stream << total_size;
			stream.writeRawData(buffer.data().constData(), buffer.size());
		}
		
		//write the visibility data
		stream << (uint8_t)players.size();
		for(auto& p : players) {
			stream << p.tile_visibility;
		}
		
		file.close();
		return 0;
	}
	
	//if we get here, we need to create a new save game file
	if(!file.open(QIODevice::WriteOnly))
		return 1;//MAP_COULD_NOT_OPEN_FILE;
	
	QDataStream stream(&file);
	
	save_game_header_t header;
	header.map_name = map.name;
	header.save_name = "my save";
	header.started_date = QDateTime::currentDateTime();
	header.last_save_time = QDateTime::currentDateTime();
	header.save_points = 0;
	header.total_days = date + 1;
	
	write_save_game_header_stream(stream, header);
	game_t::write_game_info(stream, this);
	write_map_file_stream(stream, map);
	
	return 0; //SUCCESS
}

uint game_t::load_saved_game_info(QDataStream& stream, save_game_header_t& header, game_t* game_info, std::vector<save_checkpoint_t>& checkpoints) {
	//clear existing checkpoint data
	for(auto& check : checkpoints) {
		for(auto obj : check.objects)
			delete obj;
	}
	checkpoints.clear();
	
	game_t::read_save_game_header_stream(stream, header);
	
	read_game_info(stream, game_info);
	
	//fixme: this is likely broken
	map_file_header_t map_file_header;
	auto err = read_map_file_stream(stream, game_info->map, map_file_header);
	
	//we create an initial save-point which contains the initial game data
	save_checkpoint_t checkpoint;
	//checkpoint.heroes.resize(game_info->map.heroes.size())
	for(auto& h : game_info->map.heroes)
		checkpoint.heroes.push_back(h.second);
	
	checkpoint.date = game_info->date;
	checkpoint.objects.reserve(game_info->map.objects.size());
	for(auto obj : game_info->map.objects) {
		if(!obj)
			continue;
		
		auto new_obj = interactable_object_t::make_new_object(obj->object_type);
		interactable_object_t::copy_interactable_object(new_obj, obj);
		checkpoint.objects.push_back(new_obj);
	}
	
	checkpoint.visibility_map.reserve(game_info->players.size());
	for(auto& p : game_info->players) {
		checkpoint.visibility_map.push_back(p.tile_visibility);
	}
	
	checkpoints.push_back(checkpoint);
	
	for(uint i = 0; i < header.save_points; i++) {
		save_checkpoint_t chkpt;
		
		uint16_t day;
		stream >> day;
		chkpt.date = day;
		//read heroes
		uint16_t hero_count = 0;
		stream >> hero_count;
		if(hero_count > MAXIMUM_HERO_COUNT)
			return MAP_TOO_MANY_HEROES;
		
		for(int n = 0; n < hero_count; n++) {
			hero_t hero;
			stream >> hero;
			auto id = hero.id;
			chkpt.heroes.push_back(hero);
		}
		
		//read interactable objects
		uint32_t object_count;
		stream >> object_count;
		if(object_count > MAXIMUM_OBJECTS_COUNT)
			return MAP_TOO_MANY_OBJECTS;

		if(object_count != game_info->map.objects.size())
			return 3;
		
		chkpt.objects.resize(object_count);
		for(uint32_t n = 0; n < object_count; n++) {
			interactable_object_e type;
			stream >> type;
			if(type == OBJECT_UNKNOWN) {
				chkpt.objects[n] = nullptr;
				continue;
			}

			auto obj = interactable_object_t::make_new_object(type);

			uint16_t object_size = 0;
			stream >> object_size;

			//read object data into a temporary buffer first
			QByteArray buffer_data;
			buffer_data.resize(object_size);
			stream.readRawData(buffer_data.data(), object_size);

			QBuffer buffer(&buffer_data);
			buffer.open(QIODevice::ReadOnly);
			QDataStream object_stream(&buffer);

			if(!obj) { //we don't understand this object type, skip past it
				chkpt.objects[n] = nullptr;
				continue;
			}

			//read the base class members from the buffer
			object_stream >> obj->asset_id >> obj->x >> obj->y >> obj->z >> obj->width >> obj->height;

			//read the derived class data
			obj->read_data(object_stream);

			chkpt.objects[n] = obj;
		}
		
		//read the visibility data
		uint8_t players_size = 0;
		stream >> players_size;
		assert(players_size == game_info->players.size());
		chkpt.visibility_map.resize(players_size);
		for(auto& p : chkpt.visibility_map) {
			stream >> p;
		}
		
		checkpoints.push_back(chkpt);
	}
	
	return 0; //SUCCESS
}

QDataStream& operator<<(QDataStream& stream, const player_configuration_t& config) {
	stream << config.player_number;
	stream << config.color;
	stream << config.team;
	stream << config.is_human;
	stream_write_string(stream, config.player_name);
	stream << config.selected_class;
	stream_write_vector(stream, config.allowed_classes);
	stream << config.allowed_player_type;
	stream << config.starting_hero_id;
	
	return stream;
}

QDataStream& operator>>(QDataStream& stream, player_configuration_t& config) {
	stream >> config.player_number;
	stream >> config.color;
	stream >> config.team;
	stream >> config.is_human;
	config.player_name = stream_read_string(stream);
	stream >> config.selected_class;
	stream_read_vector(stream, config.allowed_classes, HERO_KNIGHT, max_enum_val<hero_class_e>());
	stream >> config.allowed_player_type;
	stream >> config.starting_hero_id;
	
	return stream;
}
