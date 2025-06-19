#include "core/battlefield.h"
#include "core/hero.h"
#include "core/game.h"
#include "core/interactable_object.h"

#include <deque>
#include <algorithm>
#include <random>
#include <queue>

hex_location_t unit_to_hex(battlefield_unit_t& unit) {
	return std::make_pair((int8_t)unit.x, (int8_t)unit.y);
}

template<typename T> hex_location_t make_hex_location(const T& val_x, const T& val_y) {
	return std::make_pair((int8_t)val_x, (int8_t)val_y);
}

QDataStream& operator<<(QDataStream& stream, const battle_action_t& action) {
	stream << action.action;
	stream << action.source_hex;
	stream << action.target_hex;
	stream << action.acting_unit;
	stream << (quint16)action.affected_units.size();
	for(const auto& t : action.affected_units) {
		stream << t.target_unit;
		stream << (quint64)t.damage;
		stream << t.kills;
		stream << t.was_fatal;
		stream << t.is_healing;
	}
	stream << action.spell_id;
	stream << action.buff_id;
	stream << action.applicable_talent;

	stream << (uint8_t)std::min((size_t)255, action.route.size());
	for(const auto& st : action.route) {
		stream << (int8_t)st.tile.x << (int8_t)st.tile.y;
	}
	
	return stream;
}

QDataStream& operator>>(QDataStream& stream, battle_action_t& action) {
	stream >> action.action;
	stream >> action.source_hex;
	stream >> action.target_hex;
	stream >> action.acting_unit;
	quint16 len = 0;
	stream >> len;
	action.affected_units.resize(len);
	for(uint i = 0; i < len; i++) {
		battle_action_t::target_t t;
		stream >> t.target_unit;
		stream >> (quint64&)t.damage;
		stream >> t.kills;
		stream >> t.was_fatal;
		stream >> t.is_healing;
		action.affected_units[i] = t;
	}
	stream >> action.spell_id;
	stream >> action.buff_id;
	stream >> action.applicable_talent;
	
	uint8_t route_length = 0;
	stream >> route_length;
	for(int i = 0; i < route_length; i++) {
		int8_t x, y;
		stream >> x >> y;
		action.route.push_back({x, y});
	}

	return stream;
}

QDataStream& operator<<(QDataStream& stream, const battlefield_unit_t& unit) {
	operator<<(stream, (const troop_t&)unit);
	stream << unit.troop_id;
	stream << unit.x;
	stream << unit.y;
	stream << unit.original_stack_size;
	stream << unit.unit_health;
	stream << unit.was_summoned;
	stream << unit.has_waited;
	stream << unit.has_moved;
	stream << unit.has_moraled;
	stream << unit.has_cast_spell;
	stream << unit.has_defended;
	stream << unit.retaliations_remaining;
	stream << unit.is_attacker;
	stream_write_vector(stream, unit.buffs);
	
	return stream;
}

QDataStream& operator>>(QDataStream& stream, battlefield_unit_t& unit) {
	operator>>(stream, (troop_t&)unit);
	
	stream >> unit.troop_id;
	stream >> unit.x;
	stream >> unit.y;
	stream >> unit.original_stack_size;
	stream >> unit.unit_health;
	stream >> unit.was_summoned;
	stream >> unit.has_waited;
	stream >> unit.has_moved;
	stream >> unit.has_moraled;
	stream >> unit.has_cast_spell;
	stream >> unit.has_defended;
	stream >> unit.retaliations_remaining;
	stream >> unit.is_attacker;
	stream_read_array(stream, unit.buffs);
	
	return stream;
}

QDataStream& operator<<(QDataStream& stream, const army_t& army) {
	stream_write_vector(stream, army.troops);
	stream << (int16_t)(army.hero ? army.hero->id : -1);

	return stream;
}

QDataStream& operator>>(QDataStream& stream, army_t& army) {
	stream_read_array(stream, army.troops);
	int16_t hero_id = -1;
	stream >> hero_id;

	if(hero_id == -1)
		army.hero = nullptr;
	else {
		//fixme, we don't have a good way to get heroes by id here
	}

	return stream;
}

QDataStream& operator<<(QDataStream& stream, const battlefield_t& battlefield) {
	stream << battlefield.attacking_army;
	stream << battlefield.defending_army;

	stream << battlefield.environment_type;

	stream << battlefield.is_deathmatch;
	stream << battlefield.is_creature_bank_battle;

	stream << battlefield.round;
	stream << battlefield.unit_actions_this_round;
	stream << battlefield.attacker_moved_last;
	
	stream << battlefield.time_dilation_in_effect;
	stream << battlefield.time_dilation_is_attacker_effect;
	stream << battlefield.time_dilation_rounds_remaining;
	stream << battlefield.time_dilation_speed_increase;
	stream << battlefield.time_dilation_initiative_increase;

	for(uint y = 0; y < game_config::BATTLEFIELD_HEIGHT; y++) {
		for(uint x = 0; x < game_config::BATTLEFIELD_WIDTH; x++) {
			int8_t unit_id = (battlefield.hex_grid.hexes[x][y].unit ? battlefield.hex_grid.hexes[x][y].unit->troop_id : -1);
			stream << unit_id;
		}
	}

	stream << (uint8_t)std::min((size_t)255, battlefield.unit_move_queue_with_markers.size());
	for(const auto& m : battlefield.unit_move_queue_with_markers) {
		stream << (int8_t)m.attacker_can_cast << (int8_t)m.defender_can_cast << (int8_t)m.round_marker;
		stream << (int8_t)(m.unit ? m.unit->troop_id : -1);
	}

	return stream;
}

QDataStream& operator>>(QDataStream& stream, battlefield_t& battlefield) {
	//we need to read in the armies before the hex grid, as the hex grid references units in the respective armies
	stream >> battlefield.attacking_army;
	stream >> battlefield.defending_army;

	stream >> battlefield.environment_type;

	stream >> battlefield.is_deathmatch;
	stream >> battlefield.is_creature_bank_battle;

	stream >> battlefield.round;
	stream >> battlefield.unit_actions_this_round;
	stream >> battlefield.attacker_moved_last;
	
	stream >> battlefield.time_dilation_in_effect;
	stream >> battlefield.time_dilation_is_attacker_effect;
	stream >> battlefield.time_dilation_rounds_remaining;
	stream >> battlefield.time_dilation_speed_increase;
	stream >> battlefield.time_dilation_initiative_increase;

	for(uint y = 0; y < game_config::BATTLEFIELD_HEIGHT; y++) {
		for(uint x = 0; x < game_config::BATTLEFIELD_WIDTH; x++) {
			int8_t unit_id;
			stream >> unit_id;
			battlefield.hex_grid.hexes[x][y].unit = battlefield.get_unit_by_id(unit_id); //-1 id returns nullptr
		}
	}
	
	uint8_t queue_with_markers_sz = 0; 
	stream >> queue_with_markers_sz;
	battlefield.unit_move_queue.clear();
	battlefield.unit_move_queue_with_markers.clear();

	for(int i = 0; i < queue_with_markers_sz; i++) {
		int8_t attacker_can_cast, defender_can_cast, round_marker;
		int8_t troop_id = -1;
		stream >> attacker_can_cast >> defender_can_cast >> round_marker;
		stream >> troop_id;

		move_queue_slot_t slot;
		slot.attacker_can_cast = attacker_can_cast;
		slot.defender_can_cast = defender_can_cast;
		slot.round_marker = round_marker;
		slot.unit = battlefield.get_unit_by_id(troop_id);
		battlefield.unit_move_queue_with_markers.push_back(slot);

		if(slot.unit)
			battlefield.unit_move_queue.push_back(slot.unit);
	}

	return stream;
}

bool write_battlefield_to_stream(QDataStream& stream, const battlefield_t& battlefield, const game_t& game) {
	stream << battlefield;

	//write pointers as ids
	stream << (int16_t)(battlefield.attacking_hero ? battlefield.attacking_hero->id : -1);
	stream << (int16_t)(battlefield.defending_hero ? battlefield.defending_hero->id : -1);


	//todo: handle deathmatch

	return true;
}

bool read_battlefield_from_stream(QDataStream& stream, battlefield_t& battlefield, game_t& game) {
	stream >> battlefield;

	//read hero ptrs as ids
	int16_t attacker_hero_id = -1;
	int16_t defender_hero_id = -1;
	stream >> attacker_hero_id;
	stream >> defender_hero_id;

	if(attacker_hero_id == -1)
		battlefield.attacking_hero = nullptr;
	else {
		if(game.map.heroes.count(attacker_hero_id)) {
			auto hero = &(game.map.heroes[attacker_hero_id]);
			battlefield.attacking_hero = hero;
			battlefield.attacking_army.hero = battlefield.attacking_hero;
		}
	}

	if(defender_hero_id == -1)
		battlefield.attacking_hero = nullptr;
	else {
		if(game.map.heroes.count(defender_hero_id)) {
			auto hero = &(game.map.heroes[defender_hero_id]);
			battlefield.defending_hero = hero;
			battlefield.defending_army.hero = battlefield.defending_hero;
		}
	}

	//todo: handle deathmatch

	return true;
}

const std::string stringify_combat_action(const battle_action_t& action, const std::function<QString(const char*, int)>& tr) {
	QString message;
	int count = action.acting_unit.stack_size;
	QString unit_name = tr(action.acting_unit.get_unit_name().c_str(), count);

	switch(action.action) {
		case ACTION_ROUND_ENDED: {
			message = tr("End of round %1.", count)
				.arg(action.effect_value);

			break;
		}
		case ACTION_WAIT: {
			message = tr("The %1 %2 for a better time to act.", count)
				.arg(unit_name)
				.arg(tr("waits", count));

			break;
		}
		case ACTION_DEFEND: {
			if(action.acting_unit.is_turret_or_war_machine()) {
				message = tr("The %1 skips its turn.", count).arg(unit_name);
			}
			else {
				message = tr("The %1 %2 a defensive stance.", count)
					.arg(unit_name)
					.arg(tr("takes", count));
			}

			break;
		}
		case ACTION_MORALED: {
			message = tr("High morale allows the %1 to take another action.", count)
				.arg(unit_name);

			break;
		}
		case ACTION_MISMORALED: {
			message = tr("Low morale causes the %1 to freeze in panic.", count)
				.arg(unit_name);

			break;
		}
		case ACTION_WALK_TO_HEX:
		case ACTION_FLY_TO_HEX: {
			message = tr("The %1 %2 from (%3, %4) to (%5, %6).", count)
				.arg(unit_name)
				.arg(tr("moves", count))
				.arg((int)action.source_hex.first)
				.arg((int)action.source_hex.second)
				.arg((int)action.target_hex.first)
				.arg((int)action.target_hex.second);

			break;
		}
		case ACTION_MELEE_ATTACK:
		case ACTION_RANGED_ATTACK:
		case ACTION_RANGED_AOE_ATTACK:
		case ACTION_RETALIATION: {
			uint64_t damage = 0;
			uint32_t kills = 0;
			for(const auto& u : action.affected_units) {
				damage += u.damage;
				kills += u.kills;
			}
			
			//QString defender_name = action.affected_units.size() ? (tr(action.affected_units.front().target_unit.get_unit_name(), count)) : "Unknown";
			QString defender_name;
			if(action.affected_units.size())
				defender_name = tr(action.affected_units.front().target_unit.get_unit_name().c_str(), count);

			if(kills > 0) {
				message = tr("The %1 %2 the %3 for %4 damage, killing %5.", count)
					.arg(unit_name)
					.arg(tr(action.action == ACTION_RETALIATION ? "retaliates against" : "attacks", count))
					.arg(defender_name)
					.arg(damage)
					.arg(kills);
			}
			else {
				message = tr("The %1 %2 the %3 for %4 damage.", count)
					.arg(unit_name)
					.arg(tr(action.action == ACTION_RETALIATION ? "retaliates against" : "attacks", count))
					.arg(defender_name)
					.arg(damage);
			}

			break;
		}

		case ACTION_ATTACKER_SPELLCAST:
		case ACTION_DEFENDER_SPELLCAST:
		case ACTION_UNIT_SPELLCAST: {
			uint64_t damage = 0;
			uint32_t kills = 0;
			for(const auto& u : action.affected_units) {
				damage += u.damage;
				kills += u.kills;
			}
			const auto& spell = game_config::get_spell(action.spell_id);

			QString target_name;
			if(action.affected_units.size() == 1)
				target_name = QObject::tr(action.affected_units[0].target_unit.get_unit_name().c_str());

			auto spell_name = QObject::tr(spell.name.c_str());
			QString caster_name;
			if(action.applicable_artifact != ARTIFACT_NONE) {
				const auto& art = game_config::get_artifact(action.applicable_artifact);
				caster_name = QString("The %1").arg(QObject::tr(art.name.c_str()));
			}
			else {
				if(action.action == ACTION_UNIT_SPELLCAST)
					caster_name = QString("The %1").arg(unit_name);
				else if(action.action == ACTION_ATTACKER_SPELLCAST || action.action == ACTION_DEFENDER_SPELLCAST)
					caster_name = QObject::tr(action.applicable_hero->name.c_str());
			}

			if(damage > 0) {
				if(kills > 0) {
					if(spell.is_healing_spell())
						message = tr("%1's %2 heals the %3 for %4 health, reviving %5.", count)
							.arg(caster_name)
							.arg(spell_name)
							.arg(target_name)
							.arg(damage)
							.arg(kills);
					else
						message = tr("%1's %2 deals %3 damage to the %4, killing %5.", count)
							.arg(caster_name)
							.arg(spell_name)
							.arg(damage)
							.arg(target_name)
							.arg(kills);
				}
				else {
					if(spell.is_healing_spell())
						message = tr("%1's %2 heals the %3 for %4 health.", count)
						.arg(caster_name)
						.arg(spell_name)
						.arg(target_name)
						.arg(damage);
					else
						message = tr("%1's %2 deals %3 damage to the %4.", count)
							.arg(caster_name)
							.arg(spell_name)
							.arg(damage)
							.arg(target_name);
				}
			}
			else {
				if(target_name.isEmpty()) {
					message = tr("%1 casts %2.", count)
						.arg(caster_name)
						.arg(spell_name);
				}
				else {
					message = tr("%1 casts %2 on the %3.", count)
						.arg(caster_name)
						.arg(spell_name)
						.arg(target_name);
				}

			}
			break;
		}
		
		case ACTION_UNIT_LIFESTEAL: {
			auto life_stolen = action.affected_units.front().damage;
			auto units_revived = action.affected_units.front().kills;
			message = tr("The %1 %2 %3 life.", count)
				.arg(unit_name)
				.arg(tr("steals", count))
				.arg(life_stolen);

			break;
		}
	}

	return message.toStdString();
}

std::set<battlefield_hex_t*> battlefield_hex_grid_t::get_movement_range_flyer(const battlefield_unit_t& unit, sint radius) {
	std::set<battlefield_hex_t*> movement_hexes;
		
	bool two_hex = unit.is_two_hex();
	int tail_direction = (unit.is_attacker ? -1 : 1);

	for(uint y = 0; y < game_config::BATTLEFIELD_HEIGHT; y++) {
		for(uint x = 0; x < game_config::BATTLEFIELD_WIDTH; x++) {
			if(!is_in_radius_of(unit.x, unit.y, x, y, radius))
				continue;
			
			auto hex = get_hex(x, y);
			if(!hex || !hex->passable || hex->unit)
				continue;

			if(two_hex) {
				auto tail_hex = get_hex(x + tail_direction, y);
				if(!tail_hex || !tail_hex->passable || (tail_hex->unit && tail_hex->unit != &unit)) {
					//try again, moving to hex in "front"
					auto front_hex = get_hex(x + (tail_direction * -1), y);
					if(!front_hex || !front_hex->passable || !is_in_radius_of(unit.x, unit.y, front_hex->x, front_hex->y, radius) || front_hex->unit)
						continue;

					//the repositioned "tail hex" is now the original "hex" from above, our actual
					//move location will be in "front" of the original "hex"
					//hex = front_hex;
				}
			}

			movement_hexes.insert(hex);
		}
	}
		
	return movement_hexes;
}

std::set<battlefield_hex_t*> battlefield_hex_grid_t::get_movement_range(const battlefield_unit_t& unit, sint radius, bool flyer) {
	std::set<battlefield_hex_t*> movement_hexes;
		
	if(flyer)
		return get_movement_range_flyer(unit, radius);

	////temporary fix for two-hex units
	//if(unit.is_two_hex()) {
	//	for(int y = 0; y < game_config::BATTLEFIELD_HEIGHT; y++) {
	//		for(int x = 0; x < game_config::BATTLEFIELD_WIDTH; x++) {
	//		}
	//	}
	//}

	bool two_hex = unit.is_two_hex();
	int tail_direction = (unit.is_attacker ? -1 : 1);

	movement_hexes.insert(get_hex(unit.x, unit.y));
	std::vector<std::vector<battlefield_hex_t*>> fringes;
	std::vector<battlefield_hex_t*> start;
	start.push_back(get_hex(unit.x, unit.y));
	fringes.push_back(start);
		
	for(auto i = 1; i < radius + 1; i++) {
		fringes.push_back(std::vector<battlefield_hex_t*>());
		for(size_t j = 0; j < fringes[i - 1].size(); j++) {
			auto h = fringes[i - 1][j];

			if(!h)
				continue;

			for(auto k = 0; k < 6; k++) {
				auto h2 = get_adjacent_hex(h->x, h->y, battlefield_direction_e((int)battlefield_direction_e::TOPLEFT + k));
				if(!h2 || movement_hexes.count(h2) || !h2->passable || (h2->unit && h2->unit != &unit))
					continue;

				if(two_hex) {
					auto tail_hex = get_hex(h2->x + tail_direction, h2->y);
					if(!tail_hex || !tail_hex->passable || (tail_hex->unit && tail_hex->unit != &unit)) {
						//continue;
						//try again, moving to hex in "front"
						auto front_hex = get_hex(h2->x + (tail_direction * -1), h2->y);
						
						//we have to take into account that we might not be able to move to this hex
						//if(i != radius)
						//	continue;

						if(!front_hex || !front_hex->passable || !is_in_radius_of(unit.x, unit.y, front_hex->x, front_hex->y, radius) || front_hex->unit)
							continue;


						movement_hexes.insert(h2);
						continue;
						//the repositioned "tail hex" is now the original "hex" from above, our actual
						//move location will be in "front" of the original "hex"
						//hex = front_hex;
					}

				}
				
				movement_hexes.insert(h2);
				fringes[i].push_back(h2);
			}
		}
	}
		
	return movement_hexes;
}

std::set<battlefield_hex_t*> battlefield_t::get_movement_range(battlefield_unit_t& unit, sint radius, bool flyer) {
	std::set<battlefield_hex_t*> movement_hexes;
		
	if(unit.is_turret() || unit.unit_type == UNIT_CATAPULT || unit.unit_type == UNIT_BALLISTA)
		return movement_hexes;

	if(!unit.is_two_hex())
		return hex_grid.get_movement_range(unit, radius, flyer);

	//temporary fix for two-hex units
	if(unit.is_two_hex()) {
		for(int y = 0; y < game_config::BATTLEFIELD_HEIGHT; y++) {
			for(int x = 0; x < game_config::BATTLEFIELD_WIDTH; x++) {
				if(!is_move_valid(unit, x, y))
					continue;

				movement_hexes.insert(hex_grid.get_hex(x, y));
				int tail_direction = (unit.is_attacker ? -1 : 1);
				auto tail_hex = hex_grid.get_hex(x +  tail_direction, y);
				if(tail_hex && tail_hex->passable && (!tail_hex->unit || (tail_hex->unit == &unit)))
					movement_hexes.insert(tail_hex);
			}
		}
	}

	return movement_hexes;
	//return hex_grid.get_movement_range(unit, radius, flyer);
	}

uint battlefield_t::get_two_hex_effective_x(const battlefield_unit_t& unit, uint target_x, uint target_y) {
	if(!unit.is_two_hex())
		return target_x;

	int tail_direction = (unit.is_attacker ? -1 : 1);
	int tail_x = (int)target_x + tail_direction;

	//for two-hex units, if the tail hex is invalid, impassable, or occupied, we shift the effective x position
	//left or right
	if(!hex_grid.get_hex(tail_x, target_y) 
	   || !hex_grid.get_hex(tail_x, target_y)->passable
	   || (get_unit_on_hex(tail_x, target_y) && (get_unit_on_hex(tail_x, target_y)->troop_id != unit.troop_id)))
	{
		return (int)target_x - tail_direction;
	}

	return target_x;
}

bool battlefield_t::is_move_valid(battlefield_unit_t &unit, uint target_x, uint target_y) {
	bool two_hex = unit.is_two_hex();
	int tail_direction = (unit.is_attacker ? -1 : 1);
	int tail_x = (int)target_x + tail_direction;
			
	auto effective_target_x = target_x;

	if(two_hex) {
		effective_target_x = get_two_hex_effective_x(unit, target_x, target_y);
		tail_x = effective_target_x + tail_direction;
	}

	//is the target hex valid?
	if(!hex_grid.get_hex(effective_target_x, target_y) || (two_hex && !hex_grid.get_hex(tail_x, target_y)))
		return false;

	//moving into an impassable hex is never allowed
	if(!hex_grid.get_hex(effective_target_x, target_y)->passable || (two_hex && !hex_grid.get_hex(tail_x, target_y)->passable))
		return false;

	//in a siege, the gate (if it is intact) is impassable to attackers, but can be passed by defenders
	if(is_siege() && unit.is_attacker && (target_x == 11 && target_y == 5) && gate_hp != 0)
		return false;

	//only calculate route if unit is moving to a different hex than it currently occupies
	if(unit.x == effective_target_x && unit.y == target_y)
		return true;

	//if the target hex is occupied by a unit that isn't the acting unit, bail out
	if(get_unit_on_hex(effective_target_x, target_y) && (get_unit_on_hex(effective_target_x, target_y)->troop_id != unit.troop_id))
		return false;

	//check the tail hex for two-hex units
	if(two_hex && get_unit_on_hex(tail_x, target_y) && (get_unit_on_hex(tail_x, target_y)->troop_id != unit.troop_id))
		return false;
	
	//if the unit is a flyer no need to figure out the route, they can just fly to the hex (if it's in range)
	if(unit.is_flyer()) {
		if(hex_grid.distance(unit.x, unit.y, effective_target_x, target_y) <= get_unit_adjusted_speed(unit))
			return true;
		else
			return false;
	}
	
	//make sure the walking unit has a route to the target hex
	auto route = get_unit_route(unit, effective_target_x, target_y);
	if(route.size() == 0)
		return false;
	
	if(route.size() > get_unit_adjusted_speed(unit))
		return false;
	
	return true;
}

route_t battlefield_t::get_unit_route(const battlefield_unit_t &unit, uint target_x, uint target_y) {
	return get_unit_route_xy(unit, unit.x, unit.y, target_x, target_y);
}

route_t battlefield_t::get_unit_route_xy(const battlefield_unit_t& moving_unit, uint source_x, uint source_y, uint target_x, uint target_y) {
	route_t route;
	std::queue<coord_t> q;
	std::map<coord_t, coord_t> came_from;
	int x1 = source_x;
	int y1 = source_y;
	coord_t coord1{x1, y1};
	q.push(coord1);

	bool two_hex = moving_unit.is_two_hex();
	int tail_direction = (moving_unit.is_attacker ? -1 : 1);

	int x2 = target_x;
	int y2 = target_y;

	while(!q.empty()) {
		auto p = q.front();
		q.pop();

		if(p.x == x2 && p.y == y2) {
			while(p != coord1) {
				route.push_back({p, 0});
				p = came_from[p];
			}
			break;
		}

		for(int i = 0; i < 6; i++) {
			auto direction = (battlefield_direction_e)(TOPLEFT + i);
			auto next_hex = hex_grid.get_adjacent_hex(p.x, p.y, direction);
			if(!next_hex)
				continue;

			auto tail_hex = hex_grid.get_hex(next_hex->x + tail_direction, next_hex->y);
			if(two_hex && !tail_hex)
				continue;
			
			//in a siege, the gate (if it is intact) is impassable to attackers, but can be passed by defenders
			if(is_siege() && moving_unit.is_attacker && (next_hex->x == 11 && next_hex->y == 5) && gate_hp != 0)
				continue;

			//check that the next hex in the path is passable and unoccupied
			if((!next_hex->passable || (next_hex->unit && next_hex->unit->troop_id != moving_unit.troop_id)))// && !(next_hex->x == x2 && next_hex->y == y2))
				continue;

			//we can have the situation where a two-hex unit wants to "pass through" itself
			if(two_hex && (!tail_hex->passable || (tail_hex->unit && tail_hex->unit->troop_id != moving_unit.troop_id)))
				continue;
			
			step_t next_step{{next_hex->x, next_hex->y}, 0};
			if(came_from.find(next_step.tile) == came_from.end()) {
				q.push(next_step.tile);
				came_from[next_step.tile] = p;
			}
		}
	}

	std::reverse(route.begin(), route.end());
	return route;
}

bool battlefield_t::troops_remain() {
	int attacker_alive_troops = 0;
	for(uint i = 0; i < attacking_army.MAX_BATTLEFIELD_TROOPS; i++) {
		if(attacking_army.troops[i].is_turret_or_war_machine())
			continue;

		if(attacking_army.troops[i].stack_size != 0) {
			attacker_alive_troops++;
		}
	}

	int defender_alive_troops = 0;
	for(uint i = 0; i < defending_army.MAX_BATTLEFIELD_TROOPS; i++) {
		if(defending_army.troops[i].is_turret_or_war_machine())
			continue;

		if(defending_army.troops[i].stack_size != 0) {
			defender_alive_troops++;
		}
	}
	
	//fixme: why is this here?
	unit_actions_per_round = attacker_alive_troops + defender_alive_troops;
	
	if(attacking_hero)
		attacking_hero_cast_interval = std::ceil(attacking_hero->get_spell_haste_percentage() * unit_actions_per_round);
	if(defending_hero)
		defending_hero_cast_interval = defending_hero->get_spell_haste_percentage() * unit_actions_per_round;
	
	return (attacker_alive_troops && defender_alive_troops);
}

std::pair<uint32_t, uint32_t> battlefield_t::get_attack_damage_range(battlefield_unit_t& attacker, battlefield_unit_t& defender, bool is_ranged_attack, battlefield_hex_t* attack_from_hex, battlefield_hex_t* source_movement_hex) {
	assert(!attacker.is_empty() && !defender.is_empty());
	
	auto base_range = get_unit_adjusted_damage_range(attacker);
	auto base_min = base_range.first * attacker.stack_size;
	auto base_max = base_range.second * attacker.stack_size;
	
	uint32_t min_damage = apply_damage_adjustments(base_min, attacker, defender, is_ranged_attack, false, attack_from_hex, source_movement_hex);
	uint32_t max_damage = apply_damage_adjustments(base_max, attacker, defender, is_ranged_attack, false, attack_from_hex, source_movement_hex);
	
	return std::make_pair(min_damage, max_damage);
}

uint16_t battlefield_t::get_kills(uint32_t damage, battlefield_unit_t& defender) {
	if(damage < defender.unit_health)
		return 0;
	
	auto kills = 1;
	damage -= defender.unit_health;
	auto adjusted_hp = get_unit_adjusted_hp(defender);
	if(adjusted_hp == 0) //should not happen
		return 0;

	kills += damage / adjusted_hp;

	return kills;
}

std::pair<uint32_t, uint32_t> battlefield_t::get_kill_range(battlefield_unit_t& attacker, battlefield_unit_t& defender, bool is_ranged_attack, battlefield_hex_t* attack_from_hex, battlefield_hex_t* source_movement_hex) {
	auto range = get_attack_damage_range(attacker, defender, is_ranged_attack, attack_from_hex, source_movement_hex);
	auto min = range.first;
	auto max = range.second;
	return std::make_pair(std::min(get_kills(min, defender), defender.stack_size), std::min(get_kills(max, defender), defender.stack_size));
}

uint32_t battlefield_t::apply_damage_adjustments(uint32_t base_damage, battlefield_unit_t& attacker, battlefield_unit_t& defender, bool is_ranged_attack, bool is_retaliation, battlefield_hex_t* attack_from_hex, battlefield_hex_t* source_movement_hex) {
	auto& army_of_attacker = attacker.is_attacker ? attacking_army : defending_army;
	auto& army_of_defender = defender.is_attacker ? attacking_army : defending_army;
	
	auto multiplier = get_damage_multiplier(attacker, defender, is_retaliation);
	auto damage = uint32_t(base_damage * multiplier);

	if(!is_ranged_attack && army_of_attacker.is_affected_by_skill(SKILL_OFFENSE))
		damage *= 1.05f + (army_of_attacker.hero->get_secondary_skill_level(SKILL_OFFENSE) * 0.05f);

	if(is_ranged_attack && army_of_attacker.is_affected_by_skill(SKILL_ARCHERY))
		damage *= 1.05f + (army_of_attacker.hero->get_secondary_skill_level(SKILL_ARCHERY) * 0.05f);

	if(army_of_defender.is_affected_by_skill(SKILL_DEFENSE))
		damage *= 1.0f - (army_of_defender.hero->get_secondary_skill_level(SKILL_DEFENSE) * 0.05f);

	bool units_adjacent = are_units_adjacent(&attacker, &defender);
	
	if(is_ranged_attack) {
		damage = apply_ranged_attack_penalty(damage, attacker, defender);
		if(defender.has_buff(BUFF_REDUCED_DAMAGE_FROM_RANGED))
			damage *= 1.0f -(.01f * defender.get_buff(BUFF_REDUCED_DAMAGE_FROM_RANGED).magnitude);

		if(attacker.has_buff(BUFF_SHOOTS_ARROWS) && defender.has_buff(BUFF_REDUCED_DAMAGE_FROM_ARROWS))
			damage /= 2;
	}
	
	if((units_adjacent || !is_ranged_attack) && attacker.is_shooter() && !attacker.has_buff(BUFF_NO_MELEE_PENALTY))
		damage = apply_shooter_melee_penalty(damage, attacker, defender);

	if(attacker.has_buff(BUFF_JOUSTING_BONUS) && source_movement_hex && attack_from_hex) {
		auto route = get_unit_route_xy(attacker, source_movement_hex->x, source_movement_hex->y, attack_from_hex->x, attack_from_hex->y);
		//for two-hex units, if we can't get to the target hex, we adjust the target hex to the left or right
		//and then try again
		if(attacker.is_two_hex() && !route.size()) {
			int offset = attacker.is_attacker ? 1 : -1;
			route = get_unit_route_xy(attacker, source_movement_hex->x, source_movement_hex->y, (int)(attack_from_hex->x) + offset, attack_from_hex->y);
			
			if(!route.size()) {
				offset *= -1;
				route = get_unit_route_xy(attacker, source_movement_hex->x, source_movement_hex->y, (int)(attack_from_hex->x) + offset, attack_from_hex->y);
			}
		}
		damage *= 1 + (.05 * route.size());
	}

	if(army_of_attacker.is_affected_by_talent(TALENT_BACKSTAB) && attack_from_hex) {
		auto direction = hex_grid.get_adjacent_hex_direction(attack_from_hex->x, attack_from_hex->y, defender.x, defender.y);
		bool is_backstab = false;
		if(defender.is_attacker)
			is_backstab = (direction == LEFT || direction == TOPLEFT || direction == BOTTOMLEFT);
		else
			is_backstab = (direction == RIGHT || direction == TOPRIGHT || direction == BOTTOMRIGHT);

		if(is_backstab)
			damage *= 1.15;
	}

	if(army_of_attacker.hero && army_of_attacker.hero->is_artifact_in_effect(ARTIFACT_HOLY_WATER))
		damage *= 1.25f;

	if(attacker.has_buff(BUFF_CRIPPLED)) {
		damage *= (1.0f - ((float)attacker.get_buff(BUFF_CRIPPLED).magnitude / 100.f));
	}
	
	if(damage == 0)
		damage = 1;
	
	return damage;
}

uint32_t battlefield_t::calculate_damage(battlefield_unit_t& attacker, battlefield_unit_t& defender, bool is_ranged_attack, bool is_retaliation, battlefield_hex_t* attack_from_hex, battlefield_hex_t* source_movement_hex) {
	auto& army_of_attacker = attacker.is_attacker ? attacking_army : defending_army;
	auto dmg_range = get_unit_adjusted_damage_range(attacker);
	auto min_damage = dmg_range.first;
	auto max_damage = dmg_range.second;
	
	uint32_t base_dmg = utils::rand_range(min_damage, max_damage) * attacker.stack_size;

	auto damage = apply_damage_adjustments(base_dmg, attacker, defender, is_ranged_attack, is_retaliation, attack_from_hex, source_movement_hex);

	if(army_of_attacker.is_affected_by_talent(TALENT_CRITICAL_STRIKE) && utils::rand_chance(20))
		damage *= 1.5;

	return damage;
}

uint battlefield_t::get_unit_adjusted_attack(battlefield_unit_t& unit) {
	auto& army_of_unit = unit.is_attacker ? attacking_army : defending_army;
	auto attack = army_of_unit.hero ? army_of_unit.hero->get_unit_attack(unit.unit_type) : unit.get_base_attack();
	
	auto bonus = 0;
	
	if(unit.has_buff(BUFF_INCREASED_ATTACK))
		bonus += unit.get_buff(BUFF_INCREASED_ATTACK).magnitude;
	
	return attack + bonus;
}

uint battlefield_t::get_unit_adjusted_defense(battlefield_unit_t& unit) {
	auto& army_of_unit = unit.is_attacker ? attacking_army : defending_army;
	auto defense = army_of_unit.hero ? army_of_unit.hero->get_unit_defense(unit.unit_type) : unit.get_base_defense();

	auto bonus = 0;

	if(unit.has_buff(BUFF_INCREASED_DEFENSE))
		bonus += unit.get_buff(BUFF_INCREASED_DEFENSE).magnitude;

	if(unit.has_buff(BUFF_INFESTED))
		bonus -= unit.get_buff(BUFF_INFESTED).magnitude;

	return std::round((defense + bonus) * (unit.has_defended ? 1.2f : 1.f));
}

uint battlefield_t::get_unit_adjusted_hp(const battlefield_unit_t& unit) {
	auto& army_of_unit = unit.is_attacker ? attacking_army : defending_army;
	auto hp = army_of_unit.hero ? army_of_unit.hero->get_unit_max_hp(unit.unit_type) : unit.get_base_max_hitpoints();

	if(unit.has_buff(BUFF_SUMMONED) && army_of_unit.is_affected_by_talent(TALENT_SUMMONER))
		hp *= (1.4f * hp);

	if(unit.has_buff(BUFF_INCREASED_HEALTH))
		hp *= (1.0f + ((float)unit.get_buff(BUFF_INCREASED_HEALTH).magnitude / 100.f));

	if(hp == 0) //this should never happen, but if it does, it will cause a divide by 0 crash
		return 1;
		
	return hp;
}

std::pair<uint, uint> battlefield_t::get_unit_adjusted_damage_range(battlefield_unit_t& unit) {
	auto& army_of_unit = unit.is_attacker ? attacking_army : defending_army;
	
	uint min_damage = army_of_unit.hero ? army_of_unit.hero->get_unit_min_damage(unit.unit_type) : unit.get_base_min_damage();
	uint max_damage = army_of_unit.hero ? army_of_unit.hero->get_unit_max_damage(unit.unit_type) : unit.get_base_max_damage();

	if(unit.is_turret() && defending_town) {
		float multi = defending_town->get_turret_damage_multiplier();
		min_damage = (min_damage * multi);
		max_damage = (max_damage * multi);
	}
	
	if(unit.unit_type == UNIT_SKELETON && army_of_unit.is_affected_by_talent(TALENT_UNHOLY_MIGHT))
		min_damage++;
	
	if(army_of_unit.is_affected_by_talent(TALENT_RECKLESS_FORCE)) {
		min_damage = std::round(.8f * min_damage);
		max_damage = std::round(1.2f * max_damage);
	}

	if(unit.has_buff(BUFF_CURSED))
		max_damage = min_damage;
	
	if(unit.has_buff(BUFF_BLESSED)) {
		if(army_of_unit.is_affected_by_specialty(SPECIALTY_BLESS)) {
			const auto& unit_info = game_config::get_creature(unit.unit_type);
			max_damage += unit_info.tier;
		}

		min_damage = max_damage;
	}
	
	return std::make_pair(min_damage, max_damage);
}

int battlefield_t::get_unit_adjusted_luck(battlefield_unit_t& unit) {
	auto& army_of_unit = unit.is_attacker ? attacking_army : defending_army;
	auto& army_of_enemy = unit.is_attacker ? defending_army : attacking_army;
	auto luck = army_of_unit.get_luck_value();
	
	if(unit.has_buff(BUFF_INCREASED_LUCK))
		luck += unit.get_buff(BUFF_INCREASED_LUCK).magnitude;

	if(army_of_unit.is_affected_by_talent(TALENT_FORTUNE_FAVORS_THE_BOLD)) {
		if(army_of_unit.hero == attacking_hero)
			luck += 2;
		else
			luck -= 1;
	}
	
	if(army_of_enemy.hero && army_of_enemy.hero->get_secondary_skill_level(SKILL_TREACHERY) != 0) {
		auto treachery_level = army_of_enemy.hero->get_secondary_skill_level(SKILL_TREACHERY);
		const int reduced_luck_amount[5] = { 1, 1, 2, 3, 4 };
		luck -= reduced_luck_amount[treachery_level];
	}
	
	return luck;
}

int battlefield_t::get_unit_adjusted_morale(const battlefield_unit_t& unit) const {
	auto& army_of_unit = unit.is_attacker ? attacking_army : defending_army;
	auto& army_of_enemy = unit.is_attacker ? defending_army : attacking_army;
	auto morale = army_of_unit.get_morale_value();
	
	if(unit.has_buff(BUFF_UNDEAD) || army_of_unit.is_affected_by_talent(TALENT_MINDLESS_SUBSERVIENCE))
		return 0;
	
	if(morale < 0 && unit.has_buff(BUFF_POSITIVE_MORALE))
		return 1;
	
	if(unit.has_buff(BUFF_INCREASED_MORALE))
		morale += unit.get_buff(BUFF_INCREASED_MORALE).magnitude;

	if(army_of_enemy.hero && army_of_enemy.hero->has_talent(TALENT_UNDEAD_HORDE)) {
		bool all_undead = true;
		for(auto& tr : army_of_enemy.troops) {
			if(tr.unit_type != UNIT_UNKNOWN && tr.stack_size != 0 && !tr.has_buff(BUFF_UNDEAD)) {
				all_undead = false;
				break;
			}
		}

		morale += all_undead ? -1 : 0;
	}
	
	if(army_of_enemy.hero && army_of_enemy.hero->get_secondary_skill_level(SKILL_TREACHERY) != 0) {
		auto treachery_level = army_of_enemy.hero->get_secondary_skill_level(SKILL_TREACHERY);
		const int reduced_morale_amount[5] = { 1, 1, 1, 2, 3 };
		morale -= reduced_morale_amount[treachery_level];
	}
	
	return morale;
}

int battlefield_t::get_unit_adjusted_resistance(battlefield_unit_t& unit, magic_damage_e damage_type) {
	auto& army_of_unit = unit.is_attacker ? attacking_army : defending_army;
	auto& army_of_enemy = unit.is_attacker ? defending_army : attacking_army;
	int resistance = unit.get_base_resistance();
	
	if(army_of_unit.hero)
		resistance = army_of_unit.hero->get_unit_magic_resistance(unit.unit_type, damage_type);
	
	if(army_of_enemy.hero && army_of_enemy.hero->has_talent(TALENT_ESSENCE_STRIP))
		resistance = std::max(0, resistance - 10);

	if(army_of_enemy.hero && army_of_enemy.hero->get_secondary_skill_level(SKILL_SPELL_PENETRATION) != 0)
		resistance = std::max(0, resistance - get_skill_value(10, 5, army_of_enemy.hero->get_secondary_skill_level(SKILL_SPELL_PENETRATION)));

	if(unit.has_buff(BUFF_ELECTROCUTED)) {
		int reduction = unit.get_buff(BUFF_ELECTROCUTED).magnitude;
		resistance = std::max(0, resistance - reduction);
	}

	if(unit.has_buff(BUFF_DRAGON_MAGIC_DAMPER)) {
		int base = 80;
		if(army_of_unit.is_affected_by_talent(TALENT_DRAGON_MASTER))
			base = 40;

		int remainder = 100 - base;
		int bonus = std::round(remainder * ((float)resistance / 100));
		resistance = std::clamp(base + bonus, 0, 100);
	}

	return resistance;
}

int battlefield_t::get_unit_adjusted_speed(battlefield_unit_t& unit) const {
	if(unit.has_buff(BUFF_ROOTED))
		return 0;

	if(unit.is_turret() || unit.unit_type == UNIT_CATAPULT || unit.unit_type == UNIT_BALLISTA)
		return 0;

	auto& army_of_unit = unit.is_attacker ? attacking_army : defending_army;
	int speed = army_of_unit.hero ? army_of_unit.hero->get_unit_speed(unit.unit_type) : unit.get_base_speed();
	
	if(unit.has_buff(BUFF_HASTENED))
		speed += unit.get_buff(BUFF_HASTENED).magnitude;

	if(unit.has_buff(BUFF_INCREASED_SPEED))
		speed += unit.get_buff(BUFF_INCREASED_SPEED).magnitude;

	if(unit.has_buff(BUFF_SLOWED))
		speed -= unit.get_buff(BUFF_SLOWED).magnitude;

	if(unit.has_buff(BUFF_CHILLED))
		speed -= unit.get_buff(BUFF_CHILLED).magnitude;

	if(unit.has_buff(BUFF_FIRST_STRIKE))
		speed += 2;

	if(unit.has_buff(BUFF_BONECHILLED))
		speed -= 2;

	if(unit.has_buff(BUFF_WING_CLIPPED))
		speed -= 2;

	if(unit.has_buff(BUFF_SWIFT_STRIKE))
		speed += 2;
		
	if(time_dilation_in_effect) {
		if(unit.is_attacker == time_dilation_is_attacker_effect)
			speed += time_dilation_speed_increase;
		else
			speed -= time_dilation_speed_increase;
	}

	//native terrain
	return std::max(1, speed); //speed cannot be reduced below 1
}

int battlefield_t::get_unit_adjusted_initiative(battlefield_unit_t& unit) {
	auto& army_of_unit = unit.is_attacker ? attacking_army : defending_army;
	int initiative = army_of_unit.hero ? army_of_unit.hero->get_unit_initiative(unit.unit_type) : unit.get_base_initiative();

	if(army_of_unit.hero && environment_type == BATTLEFIELD_ENVIRONMENT_GRASS && army_of_unit.hero->has_talent(TALENT_GRASSLANDS_GRACE)) {
		//only applies to enchantress troops
		const auto& cr = game_config::get_creature(unit.unit_type);
		if(cr.faction == HERO_SORCERESS)
			initiative += 1;
	}

	if(unit.has_buff(BUFF_INCREASED_INITIATIVE))
		initiative += unit.get_buff(BUFF_INCREASED_INITIATIVE).magnitude;

	if(unit.has_buff(BUFF_FIRST_STRIKE))
		initiative += 1;

	if(unit.has_buff(BUFF_BONECHILLED))
		initiative -= 1;

	if(time_dilation_in_effect) {
		if(unit.is_attacker == time_dilation_is_attacker_effect)
			initiative += time_dilation_initiative_increase;
		else
			initiative -= time_dilation_initiative_increase;
	}

	return std::max(1, initiative); //initiative cannot be reduced below 1
}

float battlefield_t::get_damage_multiplier(battlefield_unit_t& attacker, battlefield_unit_t& defender, bool is_retaliation) {
	auto& army_of_attacker = attacker.is_attacker ? attacking_army : defending_army;
	auto& army_of_defender = defender.is_attacker ? attacking_army : defending_army;
	
	auto attack = get_unit_adjusted_attack(attacker);
	auto defense = get_unit_adjusted_defense(defender);

	//todo: switch to a fixed-point like calculation
	
	if(defender.has_buff(BUFF_NIX_SHIELD))
		attack *= .4;
	
	float ignore_defense_amount = 1.f;
	if(attacker.has_buff(BUFF_BEHEMOTH_CLAWS))
		ignore_defense_amount = .4f;
	if(utils::rand_chance(50) && army_of_attacker.is_affected_by_talent(TALENT_DESOLATOR))
		ignore_defense_amount -= .2f;
	
	defense *= ignore_defense_amount;
	
	int difference = (int)attack - (int)defense;
	double multiplier = 1.0;
	if(difference > 0) { //attack greater than defense
		difference = std::min(difference, (int)game_config::get_max_attack_bonus_difference());
		multiplier = 1.0 + (difference * game_config::get_attack_bonus_multiplier());
	}
	else if(difference < 0) {
		difference = std::max(difference, -(int)game_config::get_max_defense_bonus_difference());
		multiplier = 1.0 + (difference * game_config::get_defense_bonus_multiplier());
	}

	if(attacker.has_buff(BUFF_EXTRA_DAMAGE_TO_UNDEAD) && defender.has_buff(BUFF_UNDEAD))
		multiplier *= 1.5f;
	
	//knight talents
	if(army_of_attacker.is_affected_by_talent(TALENT_UNDERDOG) && attacker.get_base_max_hitpoints() < defender.get_base_max_hitpoints())
		multiplier *= 1.15;
	
	if(army_of_attacker.is_affected_by_talent(TALENT_VALOR)) //if true, army->hero must be valid ptr
		multiplier *= 1.0 + 0.05 * std::max(0, army_of_attacker.hero->get_morale());
	
	if(army_of_defender.is_affected_by_talent(TALENT_PREPARATION) && is_retaliation)
		multiplier *= .8;
	
	if(defender.has_buff(BUFF_CRUSADE_DEBUFF_STACK))
		multiplier *= 1.0 + (defender.has_buff(BUFF_UNDEAD) ? .10 : .05) * (float)(std::min((uint16_t)10, defender.get_buff(BUFF_CRUSADE_DEBUFF_STACK).magnitude));
	
	if(defender.has_buff(BUFF_ON_GUARD))
		multiplier *= .8;
	
	//warlock talents
	if(army_of_attacker.is_affected_by_talent(TALENT_SUMMONER) && attacker.has_buff(BUFF_SUMMONED))
		multiplier *= 1.2;
	
	if(army_of_attacker.is_affected_by_talent(TALENT_MEGALOMANIA))
		multiplier *= 1 + .02 * army_of_attacker.hero->get_megalomania_artifact_count();
	
	if(army_of_defender.is_affected_by_talent(TALENT_MEGALOMANIA))
		multiplier *= 1 - .02 * army_of_defender.hero->get_megalomania_artifact_count();
	
	//barbarian talents
	if(army_of_defender.is_affected_by_talent(TALENT_SEIZE_THE_SIEGE) && attacker.is_turret())
		multiplier *= .8;
	
	if(army_of_attacker.is_affected_by_talent(TALENT_SLAYER) && game_config::get_creature(defender.unit_type).tier >= 6)
		multiplier *= 1.25;

	if(defender.has_buff(BUFF_OVERWHELMED))
		multiplier *= (1. + (.05 * defender.get_buff(BUFF_OVERWHELMED).magnitude));

	if(attacker.has_buff(BUFF_FEROCITY))
		multiplier *= 1.4;
	
	return multiplier;
}

//todo: probably move this to the spells config file
buff_e get_buff_for_spell(spell_e spell_id) {
	switch(spell_id) {
		case SPELL_HASTE:
		case SPELL_MASS_HASTE: return BUFF_HASTENED;
		
		case SPELL_FROST_RAY: return BUFF_CHILLED;

		case SPELL_MASS_SLOW:
		case SPELL_SLOW: return BUFF_SLOWED;

		case SPELL_MASS_BLESS:
		case SPELL_BLESS: return BUFF_BLESSED;

		case SPELL_FIRE_BALL: return BUFF_BURNING;

		case SPELL_MASS_BARKSKIN:
		case SPELL_BARKSKIN:
		case SPELL_MASS_DIAMOND_SKIN:
		case SPELL_DIAMOND_SKIN:
		case SPELL_STONESKIN:
		case SPELL_MASS_STONESKIN:
		case SPELL_MASS_STEELSKIN:
		case SPELL_STEELSKIN: return BUFF_INCREASED_DEFENSE;
		
		case SPELL_MASS_AIR_SHIELD:
		case SPELL_AIR_SHIELD: return BUFF_REDUCED_DAMAGE_FROM_RANGED;
		
		case SPELL_MANA_SHIELD: return BUFF_MANA_SHIELD;
		case SPELL_FIRE_SHIELD: return BUFF_FIRE_SHIELD;

		case SPELL_BLOODLUST:
		case SPELL_FRENZY:
		case SPELL_FORTITUDE: return BUFF_INCREASED_HEALTH;

		case SPELL_THORNS: return BUFF_THORNS;

		case SPELL_SHIELD:
		case SPELL_MASS_SHIELD: return BUFF_REDUCED_DAMAGE_FROM_MELEE;

		case SPELL_MIRTH: return BUFF_INCREASED_MORALE;
		case SPELL_SWIFTNESS: return BUFF_INCREASED_SPEED;
		case SPELL_CATS_REFLEXES: return BUFF_CATS_SWIFTNESS;
		case SPELL_NATURES_FORTUNE: return BUFF_INCREASED_LUCK;
		//case SPELL_ANGELS_WINGS: return BUFF_ANGELS_WINGS;
		case SPELL_ENTANGLING_ROOTS: return BUFF_ROOTED;
		case SPELL_PACIFY: return BUFF_PACIFIED;
		case SPELL_PARALYZE: return BUFF_PARALYZED;
		case SPELL_BLIND: return BUFF_BLINDED;
		
		case SPELL_MASS_CURSE:
		case SPELL_CURSE: return BUFF_CURSED;

		case SPELL_MASS_CRIPPLE:
		case SPELL_CRIPPLE: return BUFF_CRIPPLED;

		case SPELL_SORROW: return BUFF_INCREASED_MORALE;
		case SPELL_MISFORTUNE: return BUFF_INCREASED_LUCK;
		case SPELL_FEAR: return BUFF_FEARED;
		case SPELL_INFEST: return BUFF_INFESTED;
		case SPELL_BERSERK: return BUFF_BERSERK;
		case SPELL_ICE_LANCE: return BUFF_FROZEN;
		case SPELL_FIRE_BLAST: return BUFF_STUNNED;
		case SPELL_ELECTROCUTE: return BUFF_ELECTROCUTED;
		case SPELL_DIVINE_INSPIRATION: return BUFF_DIVINE_INSPIRATION;
			
		default:
			return BUFF_NONE;
	}
}

int battlefield_t::get_hero_adjusted_power(hero_t* hero) {
	if(!hero)
		return 0;

	int power = hero->get_effective_power();

	//this shouldn't happen
	if(hero != attacking_hero && hero != defending_hero)
		return power;

	//if both heroes have power siphon, the effect gets cancelled out
	if(attacking_hero && attacking_hero->has_talent(TALENT_POWER_SIPHON) && defending_hero && defending_hero->has_talent(TALENT_POWER_SIPHON))
		return power;

	auto opposing_hero = (hero == attacking_hero) ? defending_hero : attacking_hero;
	if(hero->has_talent(TALENT_POWER_SIPHON) && opposing_hero && opposing_hero->hero_class != HERO_BARBARIAN) {
		int siphoned_power = std::max(1, (int)std::round(opposing_hero->get_effective_power() * .1f)); //10%, with a minimum of 1
		power += siphoned_power;
	}
	else if(opposing_hero && opposing_hero->has_talent(TALENT_POWER_SIPHON) && hero->hero_class != HERO_BARBARIAN) {
		int siphoned_power = std::max(1, (int)std::round(opposing_hero->get_effective_power() * .1f)); //10%, with a minimum of 1
		power = std::max(1, power - siphoned_power); //cannot reduce power below 1
	}

	return power;
}

bool battlefield_t::restore_hero_mana(hero_t* hero, int mana_to_restore, talent_e talent) {
	if(!hero)
		return false;

	//we could have a situation where a hero's current mana exceeds their maximum,
	//in which case we would not add additional mana
	if(hero->mana >= hero->get_maximum_mana())
		return false;

	int old_mana = hero->mana;
	hero->mana = std::min(hero->mana + mana_to_restore, (int)hero->get_maximum_mana());
	int mana_restored = hero->mana - old_mana;

	if(!is_quick_combat) {
		battle_action_t action;
		action.action = ACTION_MANA_RESTORED;
		action.applicable_hero = hero;
		action.applicable_talent = talent;
		action.effect_value = mana_restored;
		fn_emit_combat_action(action);
	}

	total_stats.total_mana_restored += mana_restored;
	if(hero == attacking_hero)
		attacker_stats.total_mana_restored += mana_restored;
	else if(hero == defending_hero)
		defender_stats.total_mana_restored += mana_restored;

	return true;
}

std::vector<battlefield_unit_t*> battlefield_t::get_chain_lightning_targets(battlefield_unit_t* initial_target, int jump_count) {
	std::vector<battlefield_unit_t*> targets;
	std::set<battlefield_unit_t*> hit_units;

	if(!initial_target)
		return targets;

	auto current_target = initial_target;
	targets.push_back(current_target);
	hit_units.insert(current_target);

	int max_target_count = jump_count + 1;

	while(targets.size() < max_target_count) {
		battlefield_unit_t* next_target = nullptr;
		int closest_distance = 255;

		//check all units on the battlefield
		for(auto& unit : attacking_army.troops) {
			if(!unit.is_empty() && hit_units.find(&unit) == hit_units.end()) {
				int distance = hex_grid.distance(current_target->x, current_target->y, unit.x, unit.y);
				if(distance < closest_distance) {
					next_target = &unit;
					closest_distance = distance;
				}
			}
		}
		for(auto& unit : defending_army.troops) {
			if(!unit.is_empty() && hit_units.find(&unit) == hit_units.end()) {
				int distance = hex_grid.distance(current_target->x, current_target->y, unit.x, unit.y);
				if(distance < closest_distance && distance) {
					next_target = &unit;
					closest_distance = distance;
				}
			}
		}
		
		if(next_target) {
			targets.push_back(next_target);
			hit_units.insert(next_target);
			current_target = next_target;
		}
		else {
			//no more valid targets found
			break;
		}
	}

	return targets;
}

std::vector<battlefield_unit_t*> battlefield_t::cast_spell_on_random_troops(army_t::battlefield_unit_group_t& troops, spell_e spell_id, int duration, int number_of_troops_affected) {
	std::vector<battlefield_unit_t*> potential_targets;
	std::vector<battlefield_unit_t*> targets;

	for(auto& tr : troops) {
		if(!tr.is_empty() && !is_unit_immune_to_spell(&tr, spell_id))
			potential_targets.push_back(&tr);
	}

	if(!potential_targets.size())
		return targets;

	std::shuffle(potential_targets.begin(), potential_targets.end(), std::default_random_engine{});

	int picked = 0;
	while(picked < number_of_troops_affected && picked < potential_targets.size()) {
		auto target = potential_targets[picked++];
		auto buff = get_buff_for_spell(spell_id);
		target->add_buff(buff, duration);
		targets.push_back(target);
	}
	
	return targets;
}

uint battlefield_t::cast_spell(hero_t* caster, spell_e spell_id, int target_x, int target_y, battlefield_unit_t* target_unit) {
	//if(not current hero's turn)
	if(!caster || !caster->knows_spell(spell_id))
		return 1; //ERROR_INVALID_PARAMETERS
	
	const auto& spell = game_config::get_spell(spell_id);
	
	uint16_t mana_cost = caster->get_spell_cost(spell_id);
	
	if(caster->mana < mana_cost)
		return 2;//ERROR_INSUFFICIENT_MANA;
	
	//verify valid targets
	//if(spell.target == TARGET_ALL_ALLIED || spell.target == TARGET_ALL_ENEMY || spell.target == TARGET_ALL_UNITS || spell.target == TARGET_SUMMON)
	
	auto target = target_unit;
	if(!target)
		target = get_unit_on_hex(target_x, target_y);

	if(!target && (spell_id == SPELL_RESURRECTION || spell_id == SPELL_REANIMATE_DEAD || spell_id == SPELL_ARCANE_REANIMATION))
		target = get_dead_unit_on_hex(target_x, target_y);

	if(target && !is_spell_target_valid(caster, target, spell_id))
		return 3;
//	if(caster == attacking_hero && attacking_hero_used_cast)
//		return 3;
//	else if(caster == defending_hero && defending_hero_used_cast)
//		return 3;
	
	auto& caster_troops = (caster == attacking_hero ? attacking_army.troops : defending_army.troops);
	auto& enemy_troops = (caster == attacking_hero ? defending_army.troops : attacking_army.troops);
	
	std::vector<battlefield_unit_t*> units_to_update;
	
	uint32_t damage = 0;
	uint32_t total_damage = 0;
	uint16_t total_kills = 0;
	
	battle_action_t action;
	action.action = (caster == attacking_hero ? ACTION_ATTACKER_SPELLCAST : ACTION_DEFENDER_SPELLCAST);
	action.spell_id = spell_id;
	action.target_hex = make_hex_location(target_x, target_y);
	action.applicable_hero = caster;
	//action.affected_units.push_back target;
	
	switch(spell_id) {
		//MARK: nukes
		case SPELL_IMPLOSION:
		case SPELL_ARCANE_BOLT:
		case SPELL_SHADOW_BOLT:
		case SPELL_LIGHTNING_BOLT:
		case SPELL_FIRE_BALL:
		case SPELL_LUNAR_ARROW:
		case SPELL_FIRE_BLAST:
		case SPELL_FROST_RAY:
		case SPELL_ELECTROCUTE:
		case SPELL_REAPERS_SCYTHE:
		case SPELL_ICE_LANCE: {
			if(!target)
				return 1;

			damage = spell.multiplier[0].get_value(get_hero_adjusted_power(caster), caster->get_spell_effect_multiplier(spell_id));
			damage = calculate_magic_damage_to_stack(damage, *target, spell.damage_type);
			total_kills = deal_magic_damage_to_stack(damage, *target);

			if(spell_id == SPELL_REAPERS_SCYTHE && total_kills > 0 && target->stack_size > 0) {
				int additional_units_to_kill = spell.multiplier[1].get_value(0, caster->get_spell_effect_multiplier(spell_id));
				deal_damage_to_stack(target->unit_health, *target);
				total_kills++;
				additional_units_to_kill--;
				if(additional_units_to_kill > 0) {
					total_kills += deal_damage_to_stack(get_unit_adjusted_hp(*target) * additional_units_to_kill, *target);
				}
			}

			auto buff = get_buff_for_spell(spell_id);
			if(buff != BUFF_NONE && target->stack_size > 0) {
				auto duration = spell.multiplier[1].get_value(caster->get_effective_power(), caster->get_spell_effect_multiplier(spell_id));
				bool use_magnitude = (spell_id == SPELL_ELECTROCUTE || spell_id == SPELL_FROST_RAY);
				int magnitude = (use_magnitude ?
								 spell.multiplier[2].get_value(caster->get_effective_power(), caster->get_spell_effect_multiplier(spell_id))
								 : 0);

				target->add_buff(buff, duration, magnitude);
				action.buff_id = buff;
			}

			action.affected_units.push_back({*target, damage, total_kills, (target->stack_size == 0)});
		}
		break;
			
		//MARK: aoe nukes
		case SPELL_NOVA:
		case SPELL_INFERNO:
		case SPELL_METEOR_SHOWER: {
			std::vector<battlefield_unit_t*> hit_units;

			int radius = (spell.target == TARGET_HEX_RADIUS_1 ? 1 : (spell.target == TARGET_HEX_RADIUS_2 ? 2 : 3));
			auto affected_hexes = hex_grid.get_neighbors(target_x, target_y, radius, spell_id != SPELL_NOVA);

			for(auto hex : affected_hexes) {
				auto hit_unit = hex->unit; //need temporary here because deal_damage_to_stack may set hex->unit to nullptr if the damage is fatal
				if(!hex || !hit_unit || hit_unit->is_empty() || utils::contains(hit_units, hit_unit))
					continue;

				damage = spell.multiplier[0].get_value(get_hero_adjusted_power(caster), caster->get_spell_effect_multiplier(spell_id));
				damage = calculate_magic_damage_to_stack(damage, *hit_unit, spell.damage_type);
				auto kills = deal_magic_damage_to_stack(damage, *hit_unit);
				action.affected_units.push_back({ *hit_unit, damage, kills, (hit_unit->stack_size == 0) });
				hit_units.push_back(hit_unit);
				total_kills += kills;
				total_damage += damage;
			}
		}
		break;

		case SPELL_DECAY:
		case SPELL_DEATH_COIL:
		break;

		case SPELL_HOLY_SHOUT: {
			auto apply_damage = [this, &caster, spell_id, &damage, &action, &spell, &total_kills, &total_damage](battlefield_unit_t& unit) {
				if(unit.is_empty() || !unit.has_buff(BUFF_UNDEAD))
					return;

				damage = spell.multiplier[0].get_value(get_hero_adjusted_power(caster), caster->get_spell_effect_multiplier(spell_id));
				damage = calculate_magic_damage_to_stack(damage, unit, spell.damage_type);
				auto kills = deal_magic_damage_to_stack(damage, unit);
				action.affected_units.push_back({ unit, damage, kills, (unit.stack_size == 0) });
				total_kills += kills;
				total_damage += damage;
			};

			for(auto& u : caster_troops)
				apply_damage(u);

			for(auto& u : enemy_troops)
				apply_damage(u);
		}

		break;

		case SPELL_CHAIN_LIGHTNING: {
			int target_count = spell.multiplier[1].get_value(get_hero_adjusted_power(caster), caster->get_spell_effect_multiplier(spell_id));
			auto targets = get_chain_lightning_targets(target, target_count);

			auto initial_damage = spell.multiplier[0].get_value(get_hero_adjusted_power(caster), caster->get_spell_effect_multiplier(spell_id));
			auto jump_damage = initial_damage;
			for(auto current_target : targets) {
				damage = calculate_magic_damage_to_stack(jump_damage, *current_target, spell.damage_type);
				auto kills = deal_magic_damage_to_stack(damage, *current_target);
				action.affected_units.push_back({ *current_target, damage, kills, (current_target->stack_size == 0) });
				total_kills += kills;
				total_damage += damage;
				jump_damage = std::max(1, (int)std::round((float)jump_damage * (caster->has_talent(TALENT_HIGH_VOLTAGE) ? .65f : .5f)));
			}
		}
		break;

		case SPELL_ARCANE_MISSILES: {
			auto hexes = hex_grid.get_neighbors(target_x, target_y, 2);
			int max_targets = caster->get_spell_effect_multiplier(spell_id) * spell.multiplier[1].get_value(get_hero_adjusted_power(caster));

			auto caster_location = caster == attacking_hero ? hex_location_t{0, 3} : hex_location_t{game_config::BATTLEFIELD_WIDTH, 3};
			std::sort(hexes.begin(), hexes.end(), [this, caster_location](battlefield_hex_t* l, battlefield_hex_t* r) {
				return hex_grid.distance(l->x, l->y, caster_location.first, caster_location.second) < hex_grid.distance(r->x, r->y, caster_location.first, caster_location.second);
			});
			int target_count = 0;
			std::vector<battlefield_unit_t*> hit_units;
			for(const auto h : hexes) {
				auto hit_unit = h->unit; //need temporary here because deal_damage_to_stack may set hex->unit to nullptr if the damage is fatal
				if(!hit_unit || hit_unit->is_empty() || !is_spell_target_valid(caster, hit_unit, SPELL_ARCANE_MISSILES) || utils::contains(hit_units, hit_unit))
					continue;
				
				bool is_target_ally = (hit_unit->is_attacker == (caster == attacking_hero));
				if(is_target_ally) //arcane missiles only hits enemy units
					continue;

				damage = spell.multiplier[0].get_value(get_hero_adjusted_power(caster), caster->get_spell_effect_multiplier(spell_id));
				damage = calculate_magic_damage_to_stack(damage, *hit_unit, spell.damage_type);
				auto kills = deal_magic_damage_to_stack(damage, *hit_unit);
				action.affected_units.push_back({ *hit_unit, damage, kills, (hit_unit->stack_size == 0) });
				total_kills += kills;
				hit_units.push_back(hit_unit);

				target_count++;
				if(target_count >= max_targets)
					break;
			}

		}
		break;

		case SPELL_APOCALYPSE: {
			std::vector<battlefield_unit_t*> valid_targets;
			for(auto& t : enemy_troops) {
				if(!t.is_empty() && is_spell_target_valid(caster, &t, SPELL_APOCALYPSE))
					valid_targets.push_back(&t);
			}
			if(!valid_targets.size())
				break;

			int target_count = caster->get_spell_effect_multiplier(spell_id) * spell.multiplier[0].get_value(get_hero_adjusted_power(caster));
			for(int i = 0; i < target_count; i++) {
				auto t = valid_targets[rand() % valid_targets.size()];
				damage = spell.multiplier[1].get_value(get_hero_adjusted_power(caster), caster->get_spell_effect_multiplier(spell_id));
				damage = calculate_magic_damage_to_stack(damage, *t, spell.damage_type);
				auto kills = deal_magic_damage_to_stack(damage, *t);
				action.affected_units.push_back({ *t, damage, kills, (t->stack_size == 0) });
				total_kills += kills;
			}
		}
		break;
		
		case SPELL_ARMAGEDDON:
			for(auto& unit : caster_troops) {
				if(unit.is_empty() || !is_spell_target_valid(caster, &unit, spell_id))
					continue;				

				damage = spell.multiplier[0].get_value(get_hero_adjusted_power(caster), caster->get_spell_effect_multiplier(spell_id));
				damage = calculate_magic_damage_to_stack(damage, unit, spell.damage_type);
				auto kills = deal_magic_damage_to_stack(damage, unit);
				action.affected_units.push_back({ unit, damage, kills, (unit.stack_size == 0) });
				total_kills += kills;
			}
			for(auto& unit : enemy_troops) {
				if(unit.is_empty() || !is_spell_target_valid(caster, &unit, spell_id))
					continue;				

				damage = spell.multiplier[0].get_value(get_hero_adjusted_power(caster), caster->get_spell_effect_multiplier(spell_id));
				damage = calculate_magic_damage_to_stack(damage, unit, spell.damage_type);
				auto kills = deal_magic_damage_to_stack(damage, unit);
				action.affected_units.push_back({ unit, damage, kills, (unit.stack_size == 0) });
				total_kills += kills;
			}

			break;

		//MARK: dispells/cures
		case SPELL_CURE:
			if(!target)
				return 1;

			for(auto& buff : target->buffs) {
				const auto& binfo = game_config::get_buff_info(buff.buff_id);
				if(binfo.type == BUFF_TYPE_NEGATIVE)
					target->remove_buff(buff.buff_id);
			}

			action.affected_units.push_back({ *target, 0, 0, false });
			break;

		case SPELL_MASS_CURE:
			for(auto& unit : caster_troops) {
				if(unit.is_empty() || !is_spell_target_valid(caster, &unit, spell_id))
					continue;

				for(auto& buff : unit.buffs) {
					const auto& binfo = game_config::get_buff_info(buff.buff_id);
					if(binfo.type == BUFF_TYPE_NEGATIVE)
						unit.remove_buff(buff.buff_id);
				}

				action.affected_units.push_back({ unit, 0, 0, false });
			}
			break;
			
		//MARK: buffs
		case SPELL_STEELSKIN:
		case SPELL_MANA_SHIELD:
		case SPELL_DIAMOND_SKIN:
		case SPELL_AIR_SHIELD:
		case SPELL_FIRE_SHIELD:
		case SPELL_BLOODLUST:
		case SPELL_FRENZY:
		case SPELL_BLESS:
		case SPELL_FORTITUDE:
		case SPELL_THORNS:
		case SPELL_SHIELD:
		case SPELL_SWIFTNESS:
		case SPELL_CATS_REFLEXES:
		case SPELL_BARKSKIN:
		case SPELL_STONESKIN:
		case SPELL_HASTE:
		//case SPELL_ANGELS_WINGS:
		case SPELL_DIVINE_INSPIRATION: {
			if(!target)
				return 1;

			auto buff = get_buff_for_spell(spell_id);
			float prebuff_hp_percent = 1.f;
			if(buff == BUFF_INCREASED_HEALTH) //adjust hp to match the previous percentage
				prebuff_hp_percent = target->unit_health / (float)get_unit_adjusted_hp(*target);
			
			auto magnitude = spell.multiplier[0].get_value(caster->get_effective_power(), caster->get_spell_effect_multiplier(spell_id));
			auto duration = spell.multiplier[1].get_value(caster->get_effective_power(), caster->get_spell_effect_multiplier(spell_id));
			target->add_buff(buff, duration, magnitude);
			
			if(caster->has_talent(TALENT_INNERVATE)) {
				target->add_buff(BUFF_INCREASED_INITIATIVE, 2);
				action.applicable_talent = TALENT_INNERVATE;
			}

			if(caster->has_talent(TALENT_BLOSSOM_OF_LIFE)) {
				for(auto& existing_buff : target->buffs) {
					if(existing_buff.buff_id == BUFF_NONE)
						continue;

					const auto& binfo = game_config::get_buff_info(existing_buff.buff_id);
					if(binfo.type == BUFF_TYPE_NEGATIVE) {
						target->remove_buff(existing_buff.buff_id);
						break;
					}
				}
			}

			if(buff == BUFF_INCREASED_HEALTH)
				target->unit_health = std::clamp((uint)(get_unit_adjusted_hp(*target) * prebuff_hp_percent), 1u, get_unit_adjusted_hp(*target));

			if(buff == BUFF_CATS_SWIFTNESS)
				target->retaliations_remaining += target->get_buff(BUFF_CATS_SWIFTNESS).magnitude;

			action.affected_units.push_back({*target, 0, 0, false});
			action.buff_id = buff;
			}
			break;
				
		//MARK: mass buffs
		case SPELL_NATURES_FORTUNE:
		case SPELL_MIRTH:
		case SPELL_MASS_BARKSKIN:
		case SPELL_MASS_STONESKIN:
		case SPELL_MASS_STEELSKIN:
		case SPELL_MASS_DIAMOND_SKIN:
		case SPELL_MASS_FORTITUDE:
		case SPELL_MASS_THORNS:
		case SPELL_MASS_BLESS:
		case SPELL_MASS_SHIELD:
		case SPELL_MASS_AIR_SHIELD:
		case SPELL_MASS_HASTE: {
			for(auto& unit : caster_troops) {
				if(unit.is_empty() || !is_spell_target_valid(caster, &unit, spell_id))
					continue;
				
				auto buff = get_buff_for_spell(spell_id);
				unit.add_buff(buff, caster->get_effective_power(),
							 spell.multiplier[0].get_value(caster->get_effective_power(),
							 caster->get_spell_effect_multiplier(spell_id)));

				if(caster->has_talent(TALENT_INNERVATE)) {
					unit.add_buff(BUFF_INCREASED_INITIATIVE, 2);
					action.applicable_talent = TALENT_INNERVATE;
				}
				
				if(caster->has_talent(TALENT_BLOSSOM_OF_LIFE)) {
					for(auto& existing_buff : unit.buffs) {
						if(existing_buff.buff_id == BUFF_NONE)
							continue;

						const auto& binfo = game_config::get_buff_info(existing_buff.buff_id);
						if(binfo.type == BUFF_TYPE_NEGATIVE) {
							unit.remove_buff(existing_buff.buff_id);
							break;
						}
					}
				}
				
				action.affected_units.push_back({unit, 0, 0, false});
				action.buff_id = buff;
			}
			
			break;
		}
		
		//MARK: crowd-control
		case SPELL_ENTANGLING_ROOTS:
		case SPELL_PACIFY:
		case SPELL_PARALYZE:
		case SPELL_BLIND:

		//MARK: debuffs
		case SPELL_CURSE:
		case SPELL_CRIPPLE:
		case SPELL_SORROW:
		case SPELL_MISFORTUNE:
		case SPELL_FEAR:
		case SPELL_DAMNATION:
		case SPELL_BERSERK:
		case SPELL_INFEST:
		case SPELL_SLOW: {
			if(!target)
				return 1;
			
			auto buff = get_buff_for_spell(spell_id);
			//berserk lasts until the affected unit performs an attack
			int duration = (spell_id == SPELL_BERSERK) ? -1 : spell.multiplier[1].get_value(caster->get_effective_power(),
																						  caster->get_spell_effect_multiplier(spell_id));
			int magnitude = spell.multiplier[0].get_value(caster->get_effective_power(),
														  caster->get_spell_effect_multiplier(spell_id));
			
			target->add_buff(buff, duration, magnitude);
			
			action.affected_units.push_back({*target, 0, 0, false});
			action.buff_id = buff;
			}
			break;			
			
		//MARK: mass debuffs
		case SPELL_MASS_CRIPPLE:
		case SPELL_MASS_CURSE:
		case SPELL_MASS_SLOW:
			for(auto& unit : enemy_troops) {
				if(unit.is_empty() || !is_spell_target_valid(caster, &unit, spell_id))
					continue;
				
				auto buff = get_buff_for_spell(spell_id);
				unit.add_buff(buff, caster->get_effective_power(),
							 spell.multiplier[0].get_value(caster->get_effective_power(),
							 caster->get_spell_effect_multiplier(spell_id)));
				
				action.affected_units.push_back({unit, 0, 0, false});
				action.buff_id = buff;
			}
			break;
			
		//MARK: summons
		//case SPELL_SUMMON_ANGEL:
		//case SPELL_SUMMON_TREANTS:
		//case SPELL_SUMMON_INFERNAL:
		//case SPELL_HYDRA:
		//	break;
		
		//MARK: heals/resurrection
		case SPELL_REJUVENATION: {
			if(!target)
				return 1;

			//remove debuffs first, then perform healing
			int max_debuffs_to_remove = spell.multiplier[1].get_value(caster->get_effective_power(), caster->get_spell_effect_multiplier(spell_id));
			int debuffs_removed = 0;
			for(auto& buff : target->buffs) {
				if(debuffs_removed >= max_debuffs_to_remove)
					break;

				const auto& binfo = game_config::get_buff_info(buff.buff_id);
				if(binfo.type == BUFF_TYPE_NEGATIVE) {
					target->remove_buff(buff.buff_id);
					debuffs_removed++;
				}
			}

			int hp_to_heal = spell.multiplier[0].get_value(caster->get_effective_power(), caster->get_spell_effect_multiplier(spell_id));
			bool can_resurrect = caster->is_artifact_in_effect(ARTIFACT_SANDALS_OF_SERENITY) || caster->has_talent(TALENT_REVIVE);

			auto actual_healing = apply_healing_to_stack(hp_to_heal, *target, can_resurrect);
			//action.action
			action.affected_units.push_back({ *target, actual_healing.first, actual_healing.second, false, true });
			}
			break;

		case SPELL_MASS_REJUVENATION: {
			//remove debuffs first, then perform healing
			int max_debuffs_to_remove = spell.multiplier[1].get_value(caster->get_effective_power(), caster->get_spell_effect_multiplier(spell_id));
			int debuffs_removed = 0;
			
			for(auto& unit : caster_troops) {
				if(unit.is_empty() || !is_spell_target_valid(caster, &unit, spell_id))
					continue;

				for(auto& buff : unit.buffs) {
					if(debuffs_removed >= max_debuffs_to_remove)
						break;

					const auto& binfo = game_config::get_buff_info(buff.buff_id);
					if(binfo.type == BUFF_TYPE_NEGATIVE) {
						unit.remove_buff(buff.buff_id);
						debuffs_removed++;
					}
				}

				int hp_to_heal = spell.multiplier[0].get_value(caster->get_effective_power(), caster->get_spell_effect_multiplier(spell_id));
				bool can_resurrect = caster->is_artifact_in_effect(ARTIFACT_SANDALS_OF_SERENITY) || caster->has_talent(TALENT_REVIVE);

				auto actual_healing = apply_healing_to_stack(hp_to_heal, unit, can_resurrect);
				action.affected_units.push_back({ unit, actual_healing.first, actual_healing.second, false, true });
			}
			}
			break;

		case SPELL_HOLY_LIGHT:
			break;

		case SPELL_ARCANE_REANIMATION:
		case SPELL_RESURRECTION:
		case SPELL_REANIMATE_DEAD: {
			if(!target || !is_spell_target_valid(caster, target, spell_id))
				return 1;

			int hp_to_heal = spell.multiplier[0].get_value(caster->get_effective_power(), caster->get_spell_effect_multiplier(spell_id));
			if(spell_id == SPELL_RESURRECTION) {
				int effectiveness_percent = spell.multiplier[1].get_value(caster->get_effective_power(), caster->get_spell_effect_multiplier(spell_id));
				float actual_effectiveness = pow(effectiveness_percent / 100.f, target->resurrection_count++);
				hp_to_heal = std::round(hp_to_heal * (actual_effectiveness));
			}

			auto actual_healing = apply_healing_to_stack(hp_to_heal, *target, true);
			action.affected_units.push_back({ *target, actual_healing.first, actual_healing.second, false, true });
			}
			break;
		
		//MARK: other
		case SPELL_SACRIFICE:
		case SPELL_MIRROR_IMAGE:
			break;
		case SPELL_TIME_DILATION: {
			//we can have the case where time dilation is already in effect.  if that is the case,
			//then the duration is extended if the caster is the same as the 'owner' of the current effect.
			//otherwise, the effect is cancelled out.
			bool caster_is_attacker = caster == attacking_hero;
			if(time_dilation_in_effect) {
				if(caster_is_attacker == time_dilation_is_attacker_effect) { //refresh the duration
					time_dilation_rounds_remaining = spell.multiplier[1].get_value(caster->get_effective_power(),
																				   caster->get_spell_effect_multiplier(spell_id));
				}
				else { //cancel out the effect
					time_dilation_in_effect = false;
					time_dilation_rounds_remaining = 0;
					if(!is_quick_combat) {
						battle_action_t td_action;
						td_action.action = ACTION_BUFF_EXPIRED;
						td_action.buff_id = BUFF_TIME_WARP;
						fn_emit_combat_action(td_action);
					}
				}
			}
			else {
				time_dilation_in_effect = true;
				time_dilation_is_attacker_effect = caster_is_attacker;
				time_dilation_speed_increase = spell.multiplier[0].get_value(0, caster->get_spell_effect_multiplier(spell_id));
				time_dilation_initiative_increase = spell.multiplier[2].get_value(0, caster->get_spell_effect_multiplier(spell_id));
				time_dilation_rounds_remaining = spell.multiplier[1].get_value(caster->get_effective_power(),
													caster->get_spell_effect_multiplier(spell_id));
			}
		}
		break;
			
		//warcries
		case SPELL_SLAYER:
		case SPELL_CULL_THE_WEAK:
		case SPELL_RAGE:
			break;
			
		//non-combat spells
		case SPELL_WORMHOLE:
		case SPELL_MANA_BATTERY:
		case SPELL_RITUAL:
			break;
	}
	
	if(caster) {
		caster->mana -= mana_cost;
		if(caster == attacking_hero) {
			attacking_hero_used_cast = true;

			if(defending_hero && defending_hero->has_talent(TALENT_ETHER_FEAST))
				restore_hero_mana(defending_hero, std::round(mana_cost * .3f), TALENT_ETHER_FEAST);		
			
		}
		else {
			defending_hero_used_cast = true;
		}
	}
	
	if(!is_quick_combat)
		fn_emit_combat_action(action);
	
	if(!troops_remain())
		end_combat();
	
	return 0;
}

battlefield_unit_t* get_random_troop(army_t::battlefield_unit_group_t& troops) {
	size_t count = 0;
	size_t chosen_index = 0;
	
	for(uint i = 0; i < troops.size(); i++) {
		if(troops[i].is_empty())
			continue;
		
		count++;
		
		if((std::rand() % count) == 0)
			chosen_index = i; //with probability 1/count, select this troop.
	}
	
	if(count)
		return &troops[chosen_index];
	
	return nullptr;
}

int get_army_strength_value(const army_t::battlefield_unit_group_t& unit_group) {
	int total = 0;
	for(const auto& u : unit_group) {
		if(u.is_empty())
			continue;

		const auto& creature = game_config::get_creature(u.unit_type);
		total += creature.health * u.stack_size;
	}

	return total;
}

bool is_spell_target_friendly(spell_target_e target_type) {
	return (target_type == TARGET_SINGLE_ALLY 
			|| target_type == TARGET_ALL_ALLIED
			|| target_type == TARGET_ALL_UNITS);
}

void battlefield_t::reset() {
	combat_started = false;
	result = BATTLE_IN_PROGRESS;
	round = 0;
	unit_actions_this_round = 0;
	unit_actions_per_round = 0;
	attacking_hero_used_cast = false;
	defending_hero_used_cast = false;
	attacking_army.clear();
	//kind of a hack
	if(!is_creature_bank_battle)
		defending_army.clear();

	were_troops_raised = false;
	attacker_moved_last = false;
	time_dilation_in_effect = false;
	time_dilation_rounds_remaining = 0;
	necromancy_raised_troops.clear();
	captured_artifacts.clear();
	
	if(attacking_hero) {
		attacking_hero->mana = attacking_hero_initial_mana;
		attacking_hero->dark_energy = attacking_hero_initial_dark_energy;
	}
	
	if(defending_hero) {
		defending_hero->mana = defending_hero_initial_mana;
		defending_hero->dark_energy = defending_hero_initial_dark_energy;
	}
	
	hex_grid.clear_units();
	
	auto init_troop = [this] (battlefield_unit_t& troop, int x, int y, bool attacker) {
		troop.x = x;
		troop.y = y;

		troop.has_moved = false;
		troop.has_moraled = false;
		troop.has_waited = false;
		troop.has_defended = false;
		troop.has_cast_spell = false;
		troop.is_attacker = attacker;
		troop.unit_health = get_unit_adjusted_hp(troop);
		troop.stack_size = troop.original_stack_size;
		if(troop.is_two_hex()) {
			hex_grid.get_hex(troop.x, troop.y)->unit = &troop;
			hex_grid.get_hex(troop.x + (attacker ? -1 : 1), troop.y)->unit = &troop;
		}
		else {
			hex_grid.get_hex(troop.x, troop.y)->unit = &troop;
		}

		if(attacker && attacking_hero && attacking_hero->has_talent(TALENT_ON_GUARD))
			troop.add_buff(BUFF_ON_GUARD, 1);
		if(!attacker && defending_hero && defending_hero->has_talent(TALENT_ON_GUARD))
			troop.add_buff(BUFF_ON_GUARD, 1);
		if(attacker && attacking_hero && attacking_hero->has_talent(TALENT_FIRST_STRIKE))
			troop.add_buff(BUFF_FIRST_STRIKE, 1);
		if(!attacker && defending_hero && defending_hero->has_talent(TALENT_FIRST_STRIKE))
			troop.add_buff(BUFF_FIRST_STRIKE, 1);
		if(attacker && attacking_hero && attacking_hero->has_talent(TALENT_SWIFT_STRIKE))
			troop.add_buff(BUFF_SWIFT_STRIKE, 1);
		if(!attacker && defending_hero && defending_hero->has_talent(TALENT_SWIFT_STRIKE))
			troop.add_buff(BUFF_SWIFT_STRIKE, 1);
	};
	
	const static int y_layout[] = {0, 2, 4, 5, 6, 8, 10};

	const static int x_layout_bank_attacker[] = { 6, 11, 5, 5, 12, 5, 11 };
	const static int y_layout_bank_attacker[] = { 3, 3, 5, 5, 5, 7, 7 };

	const static int x_layout_bank_monsters[] = { 0, 16, 0, 8, 16, 0, 16 };
	const static int y_layout_bank_monsters[] = { 0, 0, 10, 10, 10, 5, 5 };

	//todo: short-circuit here if one of the heroes has all empty troop slots
	
	if(attacking_hero) {
		uint i = 0;
		for(; i < game_config::HERO_TROOP_SLOTS; i++) {
			if(attacking_hero->troops[i].is_empty())
				continue;
			
			attacking_army.troops[i] = attacking_hero->troops[i];
			//todo: handle tactics/group formation
			int base_x = (is_creature_bank_battle ? x_layout_bank_attacker[i] : 0);
			int xpos = attacking_army.troops[i].is_two_hex() ? base_x + 1 : base_x;
			int ypos = (is_creature_bank_battle ? y_layout_bank_attacker[i] : y_layout[i]);
			init_troop(attacking_army.troops[i], xpos, ypos, true);
		}
		
		//give the attacking hero a catapult in a siege
		if(is_siege()) {
			battlefield_unit_t catapult;
			catapult.unit_type = UNIT_CATAPULT;
			catapult.stack_size = 1;
			catapult.is_attacker = true;
			attacking_army.troops[i++] = catapult;
		}
		else if(attacking_hero->has_ballista) { //only bring ballista onto battlefield in non-sieges
			battlefield_unit_t ballista;
			ballista.unit_type = UNIT_BALLISTA;
			ballista.stack_size = 1;
			ballista.is_attacker = true;
			attacking_army.troops[i++] = ballista;
		}

		if(attacking_hero->has_talent(TALENT_BISHOPS_BLESSING)) {
			auto rt = get_random_troop(attacking_army.troops);
			if(rt)
				rt->add_buff(BUFF_BLESSED, 1);
		}
	}
		
	if(defending_town) {
		gate_hp = 2;
		main_turret_hp = 5;
		top_turret_hp = 3;
		bottom_turret_hp = 3;
		top_inner_wall_hp = 2;
		top_outer_wall_hp = 2;
		bottom_inner_wall_hp = 2;
		bottom_outer_wall_hp = 2;

		int i = 0;
		if(defending_town->garrisoned_hero) {
			//take garrison hero's troops + strongest garrison troops (if available)
			std::deque<troop_t> potential_garrison_troops;
			for(auto& tr : defending_town->garrison_troops) {
				if(tr.unit_type == UNIT_UNKNOWN)
					continue;

				potential_garrison_troops.push_back(tr);
			}
			//sort by 'strength'
			std::sort(potential_garrison_troops.begin(), potential_garrison_troops.end(), [] (const troop_t& lhs, const troop_t& rhs) {
				int lhstrength = lhs.get_base_max_hitpoints() * lhs.stack_size;
				int rhstrength = rhs.get_base_max_hitpoints() * rhs.stack_size;
				return lhstrength < rhstrength;
			});

			for(const auto& tr : defending_town->garrisoned_hero->troops) {
				defending_army.troops[i] = tr;
				if(tr.is_empty()) { //take from strongest garrison troops first
					if(potential_garrison_troops.size()) {
						defending_army.troops[i] = potential_garrison_troops.front();
						potential_garrison_troops.pop_front();
					}
				}
				if(!defending_army.troops[i].is_empty()) {
					int xpos = defending_army.troops[i].is_two_hex() ? game_config::BATTLEFIELD_WIDTH - 2 : game_config::BATTLEFIELD_WIDTH - 1;
					init_troop(defending_army.troops[i], xpos, y_layout[i], false);
				}
				i++;
			}

			//add turrets (if the town has them)
			if(defending_town->is_building_built(BUILDING_CASTLE)) {
				battlefield_unit_t main_turret;
				main_turret.unit_type = UNIT_TURRET_MAIN;
				main_turret.stack_size = 1;
				main_turret.is_attacker = false;
				defending_army.troops[i++] = main_turret;
			}
			if(defending_town->is_building_built(BUILDING_TURRET_LEFT)) {
				battlefield_unit_t left_turret;
				left_turret.unit_type = UNIT_TURRET_LEFT;
				left_turret.stack_size = 1;
				left_turret.is_attacker = false;
				defending_army.troops[i++] = left_turret;
			}
			if(defending_town->is_building_built(BUILDING_TURRET_RIGHT)) {
				battlefield_unit_t right_turret;
				right_turret.unit_type = UNIT_TURRET_RIGHT;
				right_turret.stack_size = 1;
				right_turret.is_attacker = false;
				defending_army.troops[i++] = right_turret;
			}

		}
		else { //otherwise, just take the garrisoned troops as defenders
			for(auto& tr : defending_town->garrison_troops) {
				if(tr.unit_type == UNIT_UNKNOWN)
					continue;

				defending_army.troops[i] = tr;
				//todo: fix handling two-hex units here
				int xpos = defending_army.troops[i].is_two_hex() ? game_config::BATTLEFIELD_WIDTH - 2 : game_config::BATTLEFIELD_WIDTH - 1;
				init_troop(defending_army.troops[i], xpos, y_layout[i], false);
				i++;
			}
		}
	}
	else if(defending_hero) {
		int i = 0;
		for(; i < game_config::HERO_TROOP_SLOTS; i++) {
			if(defending_hero->troops[i].stack_size == 0)
				continue;
			
			defending_army.troops[i] = defending_hero->troops[i];
			int xpos = defending_army.troops[i].is_two_hex() ? game_config::BATTLEFIELD_WIDTH - 2 : game_config::BATTLEFIELD_WIDTH - 1;
			init_troop(defending_army.troops[i], xpos, y_layout[i], false);
		}
		
		if(defending_hero->has_ballista) {
			battlefield_unit_t ballista;
			ballista.unit_type = UNIT_BALLISTA;
			ballista.stack_size = 1;
			ballista.is_attacker = false;
			defending_army.troops[i++] = ballista;
		}

		if(defending_hero->has_talent(TALENT_BISHOPS_BLESSING)) {
			auto rt = get_random_troop(defending_army.troops);
			if(rt)
				rt->add_buff(BUFF_BLESSED, 1);
		}
	}
	else if(defending_monster) {
		auto& creature = game_config::get_creature(defending_monster->unit_type);
		//todo: handle splitting for wandering monsters

		int attacker_strength = get_army_strength_value(attacking_army.troops);
		int defender_strength = defending_monster->quantity * creature.health;

		int min_stacks = 1;
		int max_stacks = 3;

		if(defender_strength > 0) {
			float relative_strength = (attacker_strength / (float)defender_strength) * 100.f;

			if(relative_strength < 50.f) {
				min_stacks = 6; max_stacks = 7;
			}
			else if(relative_strength < 67.f) {
				min_stacks = 5; max_stacks = 7;
			}
			else if(relative_strength < 100.f) {
				min_stacks = 4; max_stacks = 6;
			}
			else if(relative_strength < 150.f) {
				min_stacks = 2; max_stacks = 4;
			}
			else if(relative_strength < 200.f) {
				min_stacks = 1; max_stacks = 2;
			}
		}

		int stack_count = utils::rand_range(min_stacks, max_stacks);
		stack_count = std::clamp(stack_count, 1, (int)game_config::HERO_TROOP_SLOTS);

		uint16_t base_size = defending_monster->quantity / stack_count;
		uint16_t remainder = defending_monster->quantity % stack_count;

		for(int i = 0; i < stack_count; i++) {
			uint16_t size = base_size + (i < remainder ? 1 : 0);
			defending_army.troops[i].stack_size = size;
			defending_army.troops[i].unit_type = defending_monster->unit_type;
			defending_army.troops[i].original_stack_size = size;

			int xpos = defending_army.troops[i].is_two_hex() ? game_config::BATTLEFIELD_WIDTH - 2 : game_config::BATTLEFIELD_WIDTH - 1;
			init_troop(defending_army.troops[i], xpos, y_layout[i], false);
		}
	}
	else if(is_creature_bank_battle) {
		int i = 0;
		for(auto& tr : defending_army.troops) {
			if(tr.unit_type == UNIT_UNKNOWN)
				continue;

			//todo: fix handling two-hex units here
			init_troop(tr, x_layout_bank_monsters[i], y_layout_bank_monsters[i], false);
			i++;
		}
	}
	
	int id = 0;
	for(auto& t : attacking_army.troops) {
		if(!t.is_empty())
			t.troop_id = id++;
	}
	for(auto& t : defending_army.troops) {
		if(!t.is_empty())
			t.troop_id = id++;
	}

	total_stats = {};
	attacker_stats = {};
	defender_stats = {};
}

//has to be called after troops have been setup
void battlefield_t::start_combat() {
	if(attacking_hero && attacking_hero->has_talent(TALENT_HEXMASTER))
		cast_spell_on_random_troops(defending_army.troops, SPELL_CURSE, 3);
	
	if(defending_hero && defending_hero->has_talent(TALENT_HEXMASTER))
		cast_spell_on_random_troops(attacking_army.troops, SPELL_CURSE, 3);

	auto check_for_battle_start_spellcast_artifacts = [this](hero_t* hero, army_t& friendly_army, army_t& enemy_army) {
		if(!hero)
			return;

		for(int i = 0; i < hero->artifacts.size(); i++) {
			if(hero->artifacts[i] == ARTIFACT_NONE)
				continue;

			const auto& art_info = game_config::get_artifact(hero->artifacts[i]);
			for(int n = 0; n < art_info.effects.size(); n++) {
				if(art_info.effects[n].type == EFFECT_SPELL_CAST_ON_BATTLE_START) {
					auto spell_id = art_info.effects[n].property1.spell;
					const auto& sp_info = game_config::get_spell(spell_id);
					auto buff = get_buff_for_spell(spell_id);
					int target_count = art_info.effects[n].magnitude_1;
					int duration = art_info.effects[n].magnitude_2;
					bool cast_on_friendly = is_spell_target_friendly(sp_info.target);
					
					auto affected_troops = cast_spell_on_random_troops(cast_on_friendly ? friendly_army.troops : enemy_army.troops, spell_id, target_count);
					if(affected_troops.size()) {
						if(!is_quick_combat) {
							battle_action_t artifact_cast_spell_action;
							artifact_cast_spell_action.action = ACTION_ATTACKER_SPELLCAST;
							for(auto& at : affected_troops)
								artifact_cast_spell_action.affected_units.push_back({ *at });

							artifact_cast_spell_action.spell_id = spell_id;
							artifact_cast_spell_action.buff_id = buff;
							artifact_cast_spell_action.applicable_artifact = art_info.id;
							fn_emit_combat_action(artifact_cast_spell_action);
						}
					}
				}
			}
		}
	};

	check_for_battle_start_spellcast_artifacts(attacking_hero, attacking_army, defending_army);
	check_for_battle_start_spellcast_artifacts(defending_hero, defending_army, attacking_army);
	
	combat_started = true;

	compute_next_unit_to_move();
}

const std::vector<std::vector<battlefield_direction_e>> battlefield_t::obstacle_shapes = {
	{},
	{RIGHT},
	{BOTTOMLEFT},
	{BOTTOMRIGHT},
	{BOTTOMLEFT, RIGHT},
	{RIGHT, RIGHT, RIGHT, RIGHT, BOTTOMLEFT, LEFT, LEFT, LEFT}
};

const int obstacle_margin = 2;
bool obstacle_location_valid(int x, int y) {
	return (x >= obstacle_margin && x < (game_config::BATTLEFIELD_WIDTH - obstacle_margin) && y < game_config::BATTLEFIELD_HEIGHT);
}

void battlefield_t::setup_obstacles(bool only_clear) {	
	//clear existing obstacles if they exist
	for(int y = 0; y < game_config::BATTLEFIELD_HEIGHT; y++)
		for(int x = 0; x < game_config::BATTLEFIELD_WIDTH; x++)
			hex_grid.get_hex(x, y)->passable = true;
	

	if(only_clear)
		return;

	srand(time(nullptr));

	//special case for ship combat
	if(environment_type == BATTLEFIELD_ENVIRONMENT_WATER) {

	}
	else if(is_siege()) { //special case for siege
		//top turret
		hex_grid.get_hex(13, 0)->passable = false;
		hex_grid.get_hex(14, 0)->passable = false;
		//bottom turret
		hex_grid.get_hex(13, 10)->passable = false;
		hex_grid.get_hex(14, 10)->passable = false;
		//top of gate
		hex_grid.get_hex(10, 4)->passable = false;
		hex_grid.get_hex(11, 4)->passable = false;
		//bottom of gate
		hex_grid.get_hex(10, 6)->passable = false;
		hex_grid.get_hex(11, 6)->passable = false;
		//top wall connector
		hex_grid.get_hex(12, 2)->passable = false;
		//bottom wall connector
		hex_grid.get_hex(12, 8)->passable = false;

		//top walls
		hex_grid.get_hex(13, 1)->passable = false;
		hex_grid.get_hex(12, 3)->passable = false;
		//bottom walls
		hex_grid.get_hex(12, 7)->passable = false;
		hex_grid.get_hex(13, 9)->passable = false;

		return;
	}
	
	int covered_hexes = 0;
	int attempts = 0;
	int max_attempts = 1000;
	const float coverage_percentage = utils::rand_rangef(6.0f, 12.0f);
	while(covered_hexes < ((coverage_percentage / 100.f) * game_config::BATTLEFIELD_WIDTH * game_config::BATTLEFIELD_HEIGHT) && attempts < max_attempts) {
		//pick a random hex in the valid area
		int start_x = obstacle_margin + (rand() % (game_config::BATTLEFIELD_WIDTH - (2 * obstacle_margin)));
		int start_y = (rand() % game_config::BATTLEFIELD_HEIGHT);
		
		//pick a random obstacle shape
		const auto& shape = obstacle_shapes.at(rand() % obstacle_shapes.size());
		
		//see if the obstacle placement would work
		bool placement_valid = true;
		auto xpos = start_x;
		auto ypos = start_y;
		for(const auto& dir : shape) {
			const auto adjacent_hex = hex_grid.get_adjacent_hex(xpos, ypos, dir);
			if(!adjacent_hex || !obstacle_location_valid(adjacent_hex->x, adjacent_hex->y)) {
				placement_valid = false;
				break;
			}
			xpos = adjacent_hex->x;
			ypos = adjacent_hex->y;
		}
		
		//bail out if we can't place it
		if(!placement_valid)
			continue;
		
		//place the obstacle
		xpos = start_x;
		ypos = start_y;
		hex_grid.get_hex(xpos, ypos)->passable = false; //implicit starting hex
		covered_hexes++;
		for(const auto& dir : shape) {
			auto adjacent_hex = hex_grid.get_adjacent_hex(xpos, ypos, dir);
			adjacent_hex->passable = false;
			xpos = adjacent_hex->x;
			ypos = adjacent_hex->y;
			covered_hexes++;
		}
		
		attempts++;
	}
	
}

void battlefield_t::init_hero_creature_bank_battle(hero_t* attacker, army_t& defender, interactable_object_t* object) {
	defending_army = defender;
	attacking_hero = attacker;
	defending_hero = nullptr;
	defending_monster = nullptr;
	defending_map_object = object;
	defending_town = nullptr;
	attacking_army.hero = attacker;
	defending_army.hero = nullptr;

	attacking_hero_initial_mana = attacking_hero->mana;
	attacking_hero_initial_dark_energy = attacking_hero->dark_energy;

	is_deathmatch = false;
	is_creature_bank_battle = true;

	setup_obstacles(true);

	for(auto& tr: defending_army.troops)
		tr.original_stack_size = tr.stack_size;

	reset();
}

//todo: need to fold this and init_hero_hero_battle into one function
void battlefield_t::init_hero_monster_battle(hero_t* attacker, map_monster_t* defender) {
	attacking_hero = attacker;
	defending_hero = nullptr;
	defending_town = nullptr;
	defending_map_object = nullptr;
	defending_monster = defender;
	attacking_army.hero = attacker;
	defending_army.hero = nullptr;
	
	attacking_hero_initial_mana = attacking_hero->mana;
	attacking_hero_initial_dark_energy = attacking_hero->dark_energy;

	is_deathmatch = false;
	is_creature_bank_battle = false;
	
	setup_obstacles();
	
	reset();
}

void battlefield_t::init_hero_town_battle(hero_t* attacker, town_t* defender, bool is_deathmatch_battle) {
	attacking_hero = attacker;
	defending_hero = defender->garrisoned_hero ? defender->garrisoned_hero : defender->visiting_hero;
	attacking_army.hero = attacker;
	defending_monster = nullptr;
	defending_map_object = nullptr;
	defending_town = defender;
	defending_army.hero = defender->garrisoned_hero ? defender->garrisoned_hero : defender->visiting_hero;

	attacking_hero_initial_mana = attacking_hero->mana;
	attacking_hero_initial_dark_energy = attacking_hero->dark_energy;
	if(defending_hero) {
		defending_hero_initial_mana = defending_hero->mana;
		defending_hero_initial_dark_energy = defending_hero->dark_energy;
	}

	is_deathmatch = is_deathmatch_battle;
	is_creature_bank_battle = false;

	setup_obstacles();

	reset();
}

void battlefield_t::init_hero_hero_battle(hero_t* attacker, hero_t* defender, bool is_deathmatch_battle) {
	attacking_hero = attacker;
	defending_hero = defender;
	attacking_army.hero = attacker;
	defending_monster = nullptr;
	defending_map_object = nullptr;
	defending_town = nullptr;
	defending_army.hero = defender;
	
	attacking_hero_initial_mana = attacking_hero->mana;
	attacking_hero_initial_dark_energy = attacking_hero->dark_energy;
	defending_hero_initial_mana = defending_hero->mana;
	defending_hero_initial_dark_energy = defending_hero->dark_energy;
	
	is_deathmatch = is_deathmatch_battle;
	is_creature_bank_battle = false;

	setup_obstacles();
	
	reset();
}

bool battlefield_t::round_ended() const {
	for(uint i = 0; i < army_t::MAX_BATTLEFIELD_TROOPS; i++) {
		if(can_troop_act(attacking_army.troops[i]))
			return false;
		if(can_troop_act(defending_army.troops[i]))
			return false;
	}
	
	return true;
}

void battlefield_t::next_round() {
	auto next_round_unit = [this] (battlefield_unit_t& unit) {
		unit.has_moved = false;
		unit.has_moraled = false;
		unit.has_cast_spell = false;
		unit.has_waited = false;
		unit.has_defended = false;

		for(auto& buff : unit.buffs) {
			if(buff.buff_id == BUFF_NONE)
				continue;
			
			if(buff.duration > 0)
				buff.duration--;
			
			if(buff.duration == 0) {
				if(!is_quick_combat) {
					battle_action_t action; //todo: group these up?
					action.action = ACTION_BUFF_EXPIRED;
					action.acting_unit = unit;
					action.buff_id = buff.buff_id;
					fn_emit_combat_action(action);
				}
				unit.remove_buff(buff.buff_id);
			}
		}

		//have to apply this after the above code updates buffs
		if(unit.has_buff(BUFF_MULTIPLE_RETAILIATIONS))
			unit.retaliations_remaining = 1 + unit.get_buff(BUFF_MULTIPLE_RETAILIATIONS).magnitude;
		else if(unit.has_buff(BUFF_CATS_SWIFTNESS))
		   unit.retaliations_remaining = 1 + unit.get_buff(BUFF_CATS_SWIFTNESS).magnitude;
		else
			unit.retaliations_remaining = 1;

	};
	
	for(uint i = 0; i < army_t::MAX_BATTLEFIELD_TROOPS; i++) {
		next_round_unit(attacking_army.troops[i]);
		next_round_unit(defending_army.troops[i]);
	}

	auto handle_new_round_effects = [this](hero_t* hero, army_t& army_of_hero) {
		if(hero && hero->is_artifact_in_effect(ARTIFACT_STEELWEAVE_CHAINMAIL)) {
			auto spell_id = SPELL_STEELSKIN;
			const auto& spell = game_config::get_spell(spell_id);
			auto troop = get_random_troop(army_of_hero.troops);
			if(troop)
				troop->add_buff(get_buff_for_spell(spell_id), 1, spell.multiplier[0].get_value(1));
		}
	
		//mysticism only applies in hero-hero battles
		if(attacking_hero && defending_hero && hero->has_talent(TALENT_MYSTICISM)) 
			restore_hero_mana(hero, (int)std::round(hero->get_maximum_mana() * 0.05f), TALENT_MYSTICISM); //5% of max mana

		for(auto& unit : army_of_hero.troops) {
			if(unit.is_empty())
				continue;

			if(unit.has_buff(BUFF_TROLL_REGENERATION) || army_of_hero.is_affected_by_talent(TALENT_MENDING)) {
				apply_healing_to_stack(1000, unit, false);
			}
		}

	};
	
	handle_new_round_effects(attacking_hero, attacking_army);
	handle_new_round_effects(defending_hero, defending_army);

	time_dilation_rounds_remaining = std::max(0, time_dilation_rounds_remaining - 1);
	if(time_dilation_rounds_remaining == 0 && time_dilation_in_effect) {
		time_dilation_in_effect = false;
		if(!is_quick_combat) {
			battle_action_t action;
			action.action = ACTION_BUFF_EXPIRED;
			action.buff_id = BUFF_TIME_WARP;
			fn_emit_combat_action(action);
		}
	}

	round++;
	unit_actions_this_round = 0;

	if(!is_quick_combat) {
		battle_action_t action;
		action.action = ACTION_ROUND_ENDED;
		action.effect_value = round - 1;
		fn_emit_combat_action(action);
	}
	//if(log_level > 0)
	//	combat_log("-End of Round " + std::to_string(round) + "-");

	if(round > game_config::MAX_COMBAT_ROUNDS) {
		//combat_log("Combat exceeded " + std::to_string(game_config::MAX_COMBAT_ROUNDS) + "rounds!");
		end_combat();
	}
}

bool battlefield_t::can_troop_shoot(const battlefield_unit_t* troop) {
	if(!troop || !troop->is_shooter())
		return false;
	
	if(troop->has_buff(BUFF_RANGED_CAN_SHOOT_ADJACENT))
		return true;

	auto& army_of_unit = troop->is_attacker ? attacking_army : defending_army;
	if(troop->unit_type == UNIT_BISHOP && army_of_unit.is_affected_by_specialty(SPECIALTY_BISHOPS))
		return true;

	bool can_shoot = true;
	
	magic_enum::enum_for_each<battlefield_direction_e>([this, troop, &can_shoot](auto val) {
		constexpr battlefield_direction_e direction = val;
		const auto adjacent_hex = hex_grid.get_adjacent_hex(troop->x, troop->y, direction);
		if(!adjacent_hex)
			return;
		
		auto adjacent_unit = get_unit_on_hex(adjacent_hex->x, adjacent_hex->y);
		if(!adjacent_unit)
			return;
		
		if((troop->is_attacker && !adjacent_unit->is_attacker) || (!troop->is_attacker && adjacent_unit->is_attacker))
			can_shoot = false;
	});
	
	return can_shoot;
}

bool battlefield_t::will_defender_retaliate(battlefield_unit_t& attacker, battlefield_unit_t& defender) {
	if(attacker.has_buff(buff_e::BUFF_NO_ENEMY_RETALIATION))
		return false;
	
	if(!defender.stack_size || !defender.retaliations_remaining)
		return false;

	if(defender.is_disabled())
		return false;
	
	return true;
}

uint32_t get_unit_value(const battlefield_unit_t* unit) {
	if(!unit)
		return 0;
	
	auto& cr = game_config::get_creature(unit->unit_type);
	
	return (uint32_t)((cr.health * unit->stack_size * (cr.has_inherent_buff(BUFF_SHOOTER) ? 1.4f: 1.f)));
}

std::pair<battlefield_unit_t*, battlefield_hex_t*> battlefield_t::get_nearest_target(battlefield_unit_t* acting_unit, bool include_friendly, bool ignore_range) {
	battlefield_unit_t* target = nullptr;
	battlefield_hex_t* attack_from_hex = nullptr;
	
	auto& enemy_units = acting_unit->is_attacker ? defending_army.troops : attacking_army.troops;
	auto& friendly_units = acting_unit->is_attacker ? attacking_army.troops : defending_army.troops;

	int min_distance = 255;

	auto check_and_update = [&](battlefield_unit_t& potential_target) {
		if(potential_target.is_empty() || &potential_target == acting_unit)
			return;

		int distance = hex_grid.distance(acting_unit->x, acting_unit->y, potential_target.x, potential_target.y);

		if(!ignore_range && distance > get_unit_adjusted_speed(*acting_unit))
			return;

		for(int i = 0; i < 6; i++) {
			auto direction = (battlefield_direction_e)(TOPLEFT + i);
			auto hex = hex_grid.get_adjacent_hex(potential_target.x, potential_target.y, direction);
			if(!hex || !hex->passable)
				continue;
			
			if(get_unit_on_hex(hex->x, hex->y) && (get_unit_on_hex(hex->x, hex->y) != acting_unit))
				continue;
			
			//skip finding route if we are already on the destination hex
			if(!(acting_unit->x == hex->x && acting_unit->y == hex->y)) {
				auto route = get_unit_route(*acting_unit, hex->x, hex->y);
				if(!route.size() || route.size() > get_unit_adjusted_speed(*acting_unit))
					continue;

				distance = (int)route.size();
			}

			if(distance < min_distance) {
				min_distance = distance;
				target = &potential_target;
				attack_from_hex = hex;
			}
		}
	};

	for(auto& unit : enemy_units)
		check_and_update(unit);

	if(include_friendly) {
		for(auto& unit : friendly_units)
			check_and_update(unit);
	}		
	
	return std::make_pair(target, attack_from_hex);
}

std::pair<battlefield_unit_t*, battlefield_hex_t*> battlefield_t::get_target_in_range(battlefield_unit_t* acting_unit) {
	battlefield_unit_t* target = nullptr;
	battlefield_hex_t* attack_from_hex = nullptr;
	
	std::vector<battlefield_unit_t*> possible_targets;
	auto& enemy_units = acting_unit->is_attacker ? defending_army.troops : attacking_army.troops;
	
	for(auto& unit : enemy_units) {
		if(unit.is_empty())
			continue;
		
		if(hex_grid.distance(acting_unit->x, acting_unit->y, unit.x, unit.y) > (get_unit_adjusted_speed(*acting_unit) + 1))
			continue;
		
		possible_targets.push_back(&unit);
	}
	
	std::sort(possible_targets.begin(), possible_targets.end(), [](battlefield_unit_t* a, battlefield_unit_t* b) {
		return get_unit_value(a) > get_unit_value(b);
	});
	
	
	for(auto potential_target : possible_targets) {
		for(int i = 0; i < 6; i++) {
			auto direction = (battlefield_direction_e)(TOPLEFT + i);
			auto hex = hex_grid.get_adjacent_hex(potential_target->x, potential_target->y, direction);
			if(!hex || !hex->passable)
				continue;
			
			if(get_unit_on_hex(hex->x, hex->y) && (get_unit_on_hex(hex->x, hex->y) != acting_unit))
				continue;
			
			//skip finding route if we are already on the destination hex
			if(!(acting_unit->x == hex->x && acting_unit->y == hex->y)) {
				auto route = get_unit_route(*acting_unit, hex->x, hex->y);
				if(!route.size() || route.size() > get_unit_adjusted_speed(*acting_unit))
					continue;
			}
			
			target = potential_target;
			attack_from_hex = hex;
		}
		
		if(target)
			break;
	}
	
	return std::make_pair(target, attack_from_hex);
}

//battlefield_unit_t* get_highest_priority_target


battlefield_hex_t* battlefield_t::get_target_movement_hex(battlefield_unit_t* acting_unit) {
	battlefield_hex_t* target_hex = nullptr;
		
	auto& enemy_troops = acting_unit->is_attacker ? defending_army.troops : attacking_army.troops;
	battlefield_unit_t* highest_priority_unit = nullptr;
	for(auto& dt : enemy_troops) {
		if(dt.is_empty())
			continue;
		
		if(get_unit_value(&dt) > get_unit_value(highest_priority_unit))
			highest_priority_unit = &dt;
		
	}
	if(!highest_priority_unit)
		return nullptr;
			
	auto movement_area = get_movement_range(*acting_unit, get_unit_adjusted_speed(*acting_unit), acting_unit->is_flyer());
	
	sint min_dist = 255; //should be unsigned ?
	for(auto hex : movement_area) {
		auto unit = get_unit_on_hex(hex->x, hex->y);
		if(unit) //hex is occupied
			continue;
		
		auto dist = hex_grid.distance(highest_priority_unit->x, highest_priority_unit->y, hex->x, hex->y);
		if(dist < min_dist) {
			min_dist = dist;
			target_hex = hex;
		}
	}
	
	return target_hex;
}

battle_result_e battlefield_t::update_battle(game_t* game_instance, bool single_step) {
	while(true) {
		auto troop = get_active_unit();
		
		if(!troop) {
			////
			return result;
		}		
		
		if(!troop->has_buff(BUFF_BERSERK) && is_troop_human_controlled(troop, game_instance)) //effectively do nothing, waiting on player to act
			return BATTLE_IN_PROGRESS;
		
		auto_move_troop();
		
		if(single_step)
			return result;
	}
	
	return result;
}

bool battlefield_t::auto_move_troop() {
	auto troop = get_active_unit();
	
	//no action to take if there are no units capable of acting
	if(!troop)
		return false;
	
	auto& enemy_troops = troop->is_attacker ? defending_army.troops : attacking_army.troops;
	
	//if(!troop->is_attacker && defending_hero && !defending_hero_used_cast) {
	//	//select a spell to cast as ai hero
	//	bool hastened = false;
	//	for(const auto& t : defending_army.troops) {
	//		if(t.has_buff(BUFF_HASTENED))
	//			hastened = true;
	//	}
	//	if(round == 0 && !hastened) {
	//		cast_spell(defending_hero, SPELL_MASS_HASTE);
	//	}
	//	else {
	//		for(auto& t : defending_army.troops) {
	//			if(!t.is_empty()) {
	//				cast_spell(defending_hero, SPELL_BLESS, t.x, t.y, &t);
	//				break;
	//			}
	//		}
	//	}
	//}

	if(troop->has_buff(BUFF_BERSERK)) {
		auto target = get_nearest_target(troop, true);
		if(target.first) {
			auto source_hex = hex_grid.get_hex(troop->x, troop->y);
			auto target_hex = target.second;
			move_and_attack_unit(*troop, *target.first, target_hex->x, target_hex->y);
			return true;
		}
		else { //no target in range
			auto target_hex = get_target_movement_hex(troop);
			if(target_hex) {
				return true;
			}
		}
	}
		
	if(can_troop_shoot(troop)) {
		for(auto& dt : enemy_troops) {
			if(dt.is_empty())
				continue;
			
			attack_unit(*troop, &dt, true, dt.x, dt.y);
			return true;
		}
	}
	auto target = get_target_in_range(troop);
	//auto route = get_unit_route(<#battlefield_unit_t &unit#>, <#uint target_x#>, <#uint target_y#>)
	if(target.first) {
		auto source_hex = hex_grid.get_hex(troop->x, troop->y);
		auto target_hex = target.second;
		move_and_attack_unit(*troop, *target.first, target_hex->x, target_hex->y);
		return true;
	}
	else { //no target in range
		auto target_hex = get_target_movement_hex(troop);
		if(target_hex) {
			bool did_move = move_unit(*troop, target_hex->x, target_hex->y);
			if(!did_move) {
				defend_unit(troop);
				return false;
			}

			return true;
		}
		defend_unit(troop);
	}
	
	if(!troop->has_moved)
		finish_troop_action(troop);
	
	return true;
}

void battlefield_t::finish_troop_action(battlefield_unit_t* unit) {
	unit->has_moved = true;
	unit->has_moraled = false;
	unit_actions_this_round++;
	attacker_moved_last = unit->is_attacker;
	
	if(attacking_hero_cast_interval && (unit_actions_this_round % attacking_hero_cast_interval == 0) && attacking_hero_used_cast) {
		attacking_hero_used_cast = false;
	}
	
	if(defending_hero_cast_interval && (unit_actions_this_round % defending_hero_cast_interval == 0) && defending_hero_used_cast) {
		defending_hero_used_cast = false;
	}

	compute_next_unit_to_move();

	//at this point the next unit in the move queue could mis-morale.
	//we need to check to see if this happens, and if so, recompute the move queue

	
//	if(!is_quick_combat)
//		fn_update_combat_unit(*unit);
	
}

bool battlefield_t::defend_unit(battlefield_unit_t* unit) {
	if(!unit || unit->has_defended)
		return false;
	
	unit->has_defended = true;
	finish_troop_action(unit);
	
	if(!is_quick_combat) {
		battle_action_t action;
		action.action = ACTION_DEFEND;
		action.acting_unit = *unit;
		fn_emit_combat_action(action);
	}

	total_stats.total_defends++;
	unit->is_attacker ? attacker_stats.total_defends++ : defender_stats.total_defends++;

	return true;
}

bool battlefield_t::wait_unit(battlefield_unit_t* unit) {
	if(!unit || unit->has_waited)
		return false;
	
	unit->has_waited = true;
	
	if(!is_quick_combat) {
		battle_action_t action;
		action.action = ACTION_WAIT;
		action.acting_unit = *unit;
		fn_emit_combat_action(action);
	}

	total_stats.total_waits++;
	unit->is_attacker ? attacker_stats.total_waits++ : defender_stats.total_waits++;

	compute_next_unit_to_move();

	return true;
}

bool battlefield_t::can_troop_act(const battlefield_unit_t& unit, int round_offset) const {
	if(unit.stack_size == 0)
		return false;
	
	if(round_offset == 0 && unit.has_moved)
		return false;
	
	if(unit.is_disabled(round_offset))
		return false;
	
	return true;
}

bool battlefield_t::can_troop_morale(const battlefield_unit_t& unit) {
	if(unit.has_moraled) //can't morale twice in the same round
		return false;

	if(!can_troop_act(unit))
		return false;

	if(unit.has_buff(BUFF_UNDEAD) || unit.has_buff(BUFF_ANIMATED))
		return false;

	//we could be in the situation where a unit lands a killing blow on the last enemy stack, thus ending combat,
	//but there is a roll for a morale that could otherwise succeed without this check
	if(!troops_remain())
		return false;

	return true;
}

constexpr std::array<int, 9> positive_morale_chance_table = { 0, 4, 8, 12, 15, 17, 18, 19, 20 };
constexpr std::array<int, 9> negative_morale_chance_table = { 0, -8, -15, -20, -23, -25, -26, -27, -28 };

int battlefield_t::get_unit_morale_chance(const battlefield_unit_t& unit) const {
	int chance = 0;

	if(unit.has_buff(BUFF_UNDEAD) || unit.has_buff(BUFF_ANIMATED) || unit.is_turret_or_war_machine())
		return 0;

	int morale = get_unit_adjusted_morale(unit);

	if(morale == 0)
		return 0;

	if(morale > 0)
		chance = positive_morale_chance_table[std::clamp(morale, 0, (int)positive_morale_chance_table.size() - 1)];
	else
		chance = negative_morale_chance_table[std::clamp(std::abs(morale), 0, (int)negative_morale_chance_table.size() - 1)];

	return chance;
}

bool battlefield_t::is_attackers_turn() {
	auto active_unit = get_active_unit();
	if(active_unit && active_unit->is_attacker)
		return true;

	return false;
}

battlefield_unit_t* battlefield_t::get_active_unit() {
	return (unit_move_queue.size() ? unit_move_queue[0] : nullptr);
}

player_e battlefield_t::get_player_of_active_unit() {
	player_e player = PLAYER_NONE;

	auto current_unit = get_active_unit();
	if(!current_unit)
		return player;

	if(current_unit->is_attacker) {
		if(attacking_hero)
			return attacking_hero->player;
		//else if(attacking_army) //could be the case for deathmatches?
		else
			return player;
	}
	else {
		if(defending_hero)
			return defending_hero->player;
		else if(defending_town)
			return defending_town->player;
		else if(defending_monster)
			return PLAYER_NEUTRAL;
	}

	return player;
}

castle_wall_section_e battlefield_t::get_catapult_auto_target_wall() const {
	if(bottom_inner_wall_hp > 0)
		return CASTLE_WALL_SECTION_BOTTOM_INNER_WALL;
	if(bottom_outer_wall_hp > 0)
		return CASTLE_WALL_SECTION_BOTTOM_OUTER_WALL;
	if(top_inner_wall_hp > 0)
		return CASTLE_WALL_SECTION_TOP_INNER_WALL;
	if(top_outer_wall_hp > 0)
		return CASTLE_WALL_SECTION_TOP_OUTER_WALL;

	return CASTLE_WALL_SECTION_NONE;
}

bool battlefield_t::are_any_castle_walls_remaining() const {
	if(bottom_inner_wall_hp > 0)
		return true;
	if(bottom_outer_wall_hp > 0)
		return true;
	if(top_inner_wall_hp > 0)
		return true;
	if(top_outer_wall_hp > 0)
		return true;

	return false;
}

bool battlefield_t::catapult_shoot_wall(castle_wall_section_e wall_section) {
	if(!is_siege() || wall_section == CASTLE_WALL_SECTION_NONE)
		return false;

	auto active_unit = get_active_unit();
	if(!active_unit || active_unit->unit_type != UNIT_CATAPULT)
		return false;

	bool did_destroy = false;
	bool already_destroyed = false;
	int damage = 2;

	auto check_wall_section = [this, wall_section, damage, &did_destroy, &already_destroyed] (castle_wall_section_e section_to_check, int& section_hp_var) {
		if(section_to_check == wall_section) {
			if(section_hp_var == 0) {
				already_destroyed = true;
				return;
			}

			section_hp_var = std::max(0, section_hp_var - damage);
			did_destroy = (section_hp_var == 0);
		}
	};
	
	check_wall_section(CASTLE_WALL_SECTION_BOTTOM_INNER_WALL, bottom_inner_wall_hp);
	check_wall_section(CASTLE_WALL_SECTION_BOTTOM_OUTER_WALL, bottom_outer_wall_hp);
	check_wall_section(CASTLE_WALL_SECTION_TOP_INNER_WALL, top_inner_wall_hp);
	check_wall_section(CASTLE_WALL_SECTION_TOP_OUTER_WALL, top_outer_wall_hp);

	if(already_destroyed)
		return false;

	if(did_destroy) {
		switch(wall_section) {
			case CASTLE_WALL_SECTION_BOTTOM_INNER_WALL:
				hex_grid.get_hex(12, 7)->passable = true;
				break;
			case CASTLE_WALL_SECTION_BOTTOM_OUTER_WALL:
				hex_grid.get_hex(13, 9)->passable = true;
				break;
			case CASTLE_WALL_SECTION_TOP_INNER_WALL:
				hex_grid.get_hex(12, 3)->passable = true;
				break;
			case CASTLE_WALL_SECTION_TOP_OUTER_WALL:
				hex_grid.get_hex(13, 1)->passable = true;
				break;

			default:
				break;
		}
	}

	if(!is_quick_combat) {
		battle_action_t catapult_action;
		catapult_action.acting_unit = *active_unit;
		catapult_action.action = ACTION_CATAPULT_FIRED_AT_WALL;
		catapult_action.effect_value = (did_destroy ? 1 : 0);
		catapult_action.affected_wall_section = wall_section;

		fn_emit_combat_action(catapult_action);
	}

	finish_troop_action(active_unit);
	return true;
}

void battlefield_t::compute_next_unit_to_move() {
	while(true) {
		recompute_unit_move_queue();
		auto active_unit = get_active_unit();
		if(!active_unit)
			break;

		if(active_unit->unit_type == UNIT_CATAPULT) {
			//if there are no castle walls left to destroy, then we continue to the next unit
			if(!are_any_castle_walls_remaining()) {
				finish_troop_action(active_unit);
				continue;
			}

			//break out and let player control catapult if we have an attacking hero war machines skill
			if(attacking_hero && attacking_hero->get_secondary_skill_level(SKILL_WAR_MACHINES) != 0)
				break;

			//otherwise, have the catapult auto-fire and then we continue to the next unit to act
			auto target_wall_section = get_catapult_auto_target_wall();
			catapult_shoot_wall(target_wall_section);
			continue;

		}
		else if(active_unit->is_turret()) {
			//break out and let player control turret if we have a hero in town with war machines skill
			if(defending_hero && defending_hero->get_secondary_skill_level(SKILL_WAR_MACHINES) != 0)
				break;

			//otherwise, have the turret auto-fire and then we continue to the next unit to act
			auto_move_troop();
			continue;
		}
		else if(active_unit->unit_type == UNIT_BALLISTA) {
			//break out and let player control ballista if we have an attacking hero war machines skill
			if(attacking_hero && attacking_hero->get_secondary_skill_level(SKILL_WAR_MACHINES) != 0)
				break;

			//otherwise, have the ballista auto-fire and then we continue to the next unit to act
			//auto target_troop = get_ballista_auto_target_troop();
			auto_move_troop();
			continue;
		}

		//check for mismorale
		int morale_chance = get_unit_morale_chance(*active_unit);
		if(morale_chance < 0 && can_troop_act(*active_unit) && !active_unit->has_waited) { //can't mismorale on the wait turn
			if(utils::rand_chance(std::abs(morale_chance))) {
				//we mismoraled, emit the action and move on to the next unit
				active_unit->has_moved = true;
				if(!is_quick_combat) {
					battle_action_t mismorale_action;
					mismorale_action.action = ACTION_MISMORALED;
					mismorale_action.acting_unit = *active_unit;

					fn_emit_combat_action(mismorale_action);
				}
				continue;
			}
		}

		//we did not mismorale, break out of loop and let this unit act
		break;
	}
}

//FIXME THIS IS BROKEN
bool battlefield_t::recompute_unit_move_queue() {
	//first clear the 'queues'
	unit_move_queue.clear();
	unit_move_queue_with_markers.clear();

	if(round_ended())
		next_round();
	
	if(!troops_remain())
		return false;

	int _round = 0;
	int troop_offset = 0;

	while(unit_move_queue.size() < BATTLE_QUEUE_DEPTH) {
		std::deque<battlefield_unit_t*> attacker_queue;
		std::deque<battlefield_unit_t*> defender_queue;
		std::deque<battlefield_unit_t*> attacker_wait_queue;
		std::deque<battlefield_unit_t*> defender_wait_queue;
		for(uint i = 0; i < army_t::MAX_BATTLEFIELD_TROOPS; i++) {
			auto& a = attacking_army.troops[i];
			auto& d = defending_army.troops[i];
			if(can_troop_act(a, _round)) {
				if(_round > 0 || !a.has_waited)
					attacker_queue.push_back(&a);
				else
					attacker_wait_queue.push_back(&a);
			}
			if(can_troop_act(d, _round)) {
				if(_round > 0 || !d.has_waited)
					defender_queue.push_back(&d);
				else
					defender_wait_queue.push_back(&d);
			}
		}
		
		auto comp = [this](battlefield_unit_t* a, battlefield_unit_t* b) {
			if(get_unit_adjusted_initiative(*a) == get_unit_adjusted_initiative(*b)) {
				if(get_unit_adjusted_speed(*a) == get_unit_adjusted_speed(*b))
					return b->y > a->y; //for same initiative and speed, troops near top of screen move first
				else
					return get_unit_adjusted_speed(*a) > get_unit_adjusted_speed(*b);
			}
			else {
				return get_unit_adjusted_initiative(*a) > get_unit_adjusted_initiative(*b);
			}
		};
		
		std::sort(attacker_queue.begin(), attacker_queue.end(), comp);
		std::sort(defender_queue.begin(), defender_queue.end(), comp);
		
		bool last_took_from_attacker = attacker_moved_last;
		while(attacker_queue.size() || defender_queue.size()) {
			auto a = attacker_queue.size() ? attacker_queue.front() : nullptr;
			auto d = defender_queue.size() ? defender_queue.front() : nullptr;
			bool take_from_attacker = true;
			if(!a)
				take_from_attacker = false;
			else if(!d)
				;
			else {
				if(get_unit_adjusted_initiative(*d) > get_unit_adjusted_initiative(*a))
					take_from_attacker = false;
				else if(get_unit_adjusted_initiative(*d) == get_unit_adjusted_initiative(*a)) {
					//if initiative is equal, the unit with the higher speed moves first.  if both
					//are equal, take from attacker, then defender, alternating
					if(get_unit_adjusted_speed(*d) > get_unit_adjusted_speed(*a))
						take_from_attacker = false;
					else if(get_unit_adjusted_speed(*d) == get_unit_adjusted_speed(*a)) {
						take_from_attacker = !last_took_from_attacker;
					}
				}
			}

			move_queue_slot_t troop_slot;
			
			if(take_from_attacker) {
				unit_move_queue.push_back(attacker_queue.front());
				troop_slot.unit = attacker_queue.front();
				attacker_queue.pop_front();
				last_took_from_attacker = true;
			}
			else {
				unit_move_queue.push_back(defender_queue.front());
				troop_slot.unit = defender_queue.front();
				defender_queue.pop_front();
				last_took_from_attacker = false;
			}

			troop_slot.attacker_can_cast = !attacking_hero_used_cast || (unit_actions_this_round + troop_offset > attacking_hero_cast_interval);
			troop_slot.defender_can_cast = !defending_hero_used_cast || (unit_actions_this_round + troop_offset > defending_hero_cast_interval);
			unit_move_queue_with_markers.push_back(troop_slot);
			troop_offset++;
		}		
		
		//todo: replace with comp and use std::reverse
		auto comp_reversed = [](battlefield_unit_t* a, battlefield_unit_t* b) {
			auto a_i = a->get_base_initiative();
			auto b_i = b->get_base_initiative();
			
			if(a_i == b_i) //for same initiative, troops near top of screen move first //fixme: should be based on troop id instead(?)
				return a->y < b->y;
			else
				return  a_i < b_i;
		
		};
		
		std::sort(attacker_wait_queue.begin(), attacker_wait_queue.end(), comp_reversed);
		std::sort(defender_wait_queue.begin(), defender_wait_queue.end(), comp_reversed);
		
		//todo: factor out
		while(attacker_wait_queue.size() || defender_wait_queue.size()) {
			auto a = attacker_wait_queue.size() ? attacker_wait_queue.front() : nullptr;
			auto d = defender_wait_queue.size() ? defender_wait_queue.front() : nullptr;
			bool take_from_attacker = true;
			if(!a)
				take_from_attacker = false;
			else if(!d)
				;
			else {
				if(d->get_base_initiative() < a->get_base_initiative())
					take_from_attacker = false;
			}

			move_queue_slot_t troop_slot;

			if(take_from_attacker) {
				unit_move_queue.push_back(a);
				troop_slot.unit = a;
				attacker_wait_queue.pop_front();
			}
			else {
				unit_move_queue.push_back(d);
				troop_slot.unit = d;
				defender_wait_queue.pop_front();
			}

			troop_slot.attacker_can_cast = !attacking_hero_used_cast || (unit_actions_this_round + troop_offset > attacking_hero_cast_interval);
			troop_slot.defender_can_cast = !defending_hero_used_cast || (unit_actions_this_round + troop_offset > defending_hero_cast_interval);
			unit_move_queue_with_markers.push_back(troop_slot);
			troop_offset++;
		}
		
		_round++;

		move_queue_slot_t round_start_slot;
		round_start_slot.unit = nullptr;
		round_start_slot.round_marker = round + _round + 1;
		round_start_slot.attacker_can_cast = !attacking_hero_used_cast || (unit_actions_this_round + troop_offset > attacking_hero_cast_interval);
		round_start_slot.defender_can_cast = !defending_hero_used_cast || (unit_actions_this_round + troop_offset > defending_hero_cast_interval);
		unit_move_queue_with_markers.push_back(round_start_slot);
	}
	
	return (unit_move_queue.size() != 0);
}

bool battlefield_t::move_unit(battlefield_unit_t& unit, sint x, sint y, bool finalize_action) {
	int effective_x = get_two_hex_effective_x(unit, x, y);

	auto from_hex = hex_grid.get_hex(unit.x, unit.y);
	auto target_hex = hex_grid.get_hex(effective_x, y);
	if(!from_hex || !target_hex || !target_hex->passable)
		return false;

	if(get_unit_on_hex(effective_x, y) && get_unit_on_hex(effective_x, y) != &unit)
		return false;
	
	if(!can_troop_act(unit))
		return false;
	
	if(unit.x == effective_x && unit.y == y)
		return false;
	
	if(!is_move_valid(unit, effective_x, y))
		return false;

	auto route = get_unit_route(unit, effective_x, y);
	
	//unit loses ON_GUARD buff once they take an action
	unit.remove_buff(BUFF_ON_GUARD);
	
	//todo: check to see if target hex is in range of unit
	//todo: move unit through hexes (to account for quicksand, firewall, hydra, etc)

	auto original_x = unit.x;
	auto original_y = unit.y;

	unit.x = effective_x;
	unit.y = y;
	assert(target_hex != from_hex);
	from_hex->unit = nullptr;

	battlefield_hex_t* target_tail_hex = nullptr;

	if(unit.is_two_hex()) {
		int tail_direction = (unit.is_attacker ? -1 : 1);
		target_tail_hex = hex_grid.get_hex(target_hex->x + tail_direction, target_hex->y);
		auto from_tail_hex = hex_grid.get_hex(from_hex->x + tail_direction, from_hex->y);
		assert(target_tail_hex && from_tail_hex);
		from_tail_hex->unit = nullptr;
	}

	target_hex->unit = &unit;
	if(target_tail_hex)
		target_tail_hex->unit = &unit;

	if((attacking_hero && attacking_hero->has_talent(TALENT_OVERWHELM)) || (defending_hero && defending_hero->has_talent(TALENT_OVERWHELM)))
		update_all_units_overwhelm_status();	
	
	battle_action_t movement_action;
	if(!is_quick_combat) {
		movement_action.action = unit.is_flyer() ? ACTION_FLY_TO_HEX : ACTION_WALK_TO_HEX;
		if(movement_action.action == ACTION_WALK_TO_HEX)
			movement_action.route = route;

		movement_action.acting_unit = unit;
		movement_action.source_hex = make_hex_location(original_x, original_y);
		movement_action.target_hex = make_hex_location(unit.x, unit.y);
	}

	//'finalize_action' should only be set when the unit is moving to another hex, not moving+attacking
	if(finalize_action) {
		//we can potentially morale here
		bool morale = utils::rand_chance(get_unit_morale_chance(unit)) && can_troop_morale(unit);
		if(morale) {
			unit.has_moraled = true;
			unit.has_moved = false;

			if(!is_quick_combat) {
				battle_action_t morale_action;
				morale_action.action = ACTION_MORALED;
				morale_action.acting_unit = unit;
				
				//emit the movement action first
				fn_emit_combat_action(movement_action);

				fn_emit_combat_action(morale_action);
			}
		}
		else {
			finish_troop_action(&unit);
			if(!is_quick_combat)
				fn_emit_combat_action(movement_action);
		}
	}
	else if(!is_quick_combat) {
		fn_emit_combat_action(movement_action);
	}
	
	return true;
}

void battlefield_t::update_all_units_overwhelm_status() {
	auto compute_overwhelm = [this] (battlefield_unit_t& unit) {
		std::set<battlefield_unit_t*> adjacent_enemies;

		for(int i = 0; i < 6; i++) {
			auto adjacent_hex = hex_grid.get_adjacent_hex(unit.x, unit.y, (battlefield_direction_e)i);
			if(adjacent_hex && adjacent_hex->unit && (adjacent_hex->unit->is_attacker != unit.is_attacker))
				adjacent_enemies.insert(adjacent_hex->unit);

			if(unit.is_two_hex()) {
				int tail_x = unit.is_attacker ? (unit.x - 1) : (unit.x + 1);
				auto tail_adjacent_hex = hex_grid.get_adjacent_hex(tail_x, unit.y, (battlefield_direction_e)i);
				
				if(tail_adjacent_hex && tail_adjacent_hex->unit && (tail_adjacent_hex->unit->is_attacker != unit.is_attacker))
					adjacent_enemies.insert(tail_adjacent_hex->unit);
			}
		}

		if(adjacent_enemies.size())
			unit.add_buff(BUFF_OVERWHELMED, -1, (uint8_t)adjacent_enemies.size());
		else
			unit.remove_buff(BUFF_OVERWHELMED);
	};

	if(defending_hero && defending_hero->has_talent(TALENT_OVERWHELM)) {
		for(auto& unit : attacking_army.troops) {
			if(!unit.is_empty())
				compute_overwhelm(unit);
		}
	}
	if(attacking_hero && attacking_hero->has_talent(TALENT_OVERWHELM)) {
		for(auto& unit : defending_army.troops) {
			if(!unit.is_empty())
				compute_overwhelm(unit);
		}
	}
}

int battlefield_t::unit_belongs_to_army(battlefield_unit_t* unit) {
	if(!unit)
		return -1;
	
	for(auto& troop : attacking_army.troops) {
		if(&troop == unit)
			return 0;
	}
	for(auto& troop : defending_army.troops) {
		if(&troop == unit)
			return 1;
	}
	
	return -1;
}

bool battlefield_t::are_units_adjacent(battlefield_unit_t* attacker, battlefield_unit_t* defender) {
	bool is_adjacent = false;
	for(int i = 0; i < 6; i++) {
		auto direction = (battlefield_direction_e)(TOPLEFT + i);
		auto next_hex = hex_grid.get_adjacent_hex(defender->x, defender->y, direction);
		if(!next_hex)
			continue;
		
		if(next_hex->unit == attacker)
			return true;
	}

	if(!defender->is_two_hex())
		return false;

	int tail_x = defender->is_attacker ? (defender->x - 1) : (defender->x + 1);
	//check tail hex for two-hexed units
	for(int i = 0; i < 6; i++) {
		auto direction = (battlefield_direction_e)(TOPLEFT + i);
		auto next_hex = hex_grid.get_adjacent_hex(tail_x, defender->y, direction);
		if(!next_hex)
			continue;

		if(next_hex->unit == attacker)
			return true;
	}

	return false;
}

bool battlefield_t::unit_can_attack(battlefield_unit_t* attacker, battlefield_unit_t* defender, uint from_x, uint from_y) {
	if(attacker == defender || !attacker || !defender)
		return false;
	
	if(!attacker->has_buff(BUFF_BERSERK) && unit_belongs_to_army(attacker) == unit_belongs_to_army(defender))
		return false;
	
	//if melee attack, is there a path from attacker to defender?
	if(!can_troop_shoot(attacker)) {
		if(attacker->x == from_x && attacker->y == from_y)
			return true;
		
		if(!is_move_valid(*attacker, from_x, from_y))
			return false;
	}
	
	return true;
}

bool battlefield_t::move_and_attack_unit(battlefield_unit_t& attacker, battlefield_unit_t& defender, sint from_x, sint from_y) {
	//if(!move_unit(attacker, from_x, from_y, false))
	//	return false;

	auto source_movement_hex = hex_grid.get_hex(attacker.x, attacker.y); //used for cavalry's 'mounted' more damage per hex traveled ability
	move_unit(attacker, from_x, from_y, false);
	if(!attack_unit(attacker, &defender, false, from_x, from_y, source_movement_hex))
		return false;
	
	return true;
}

std::pair<uint32_t, uint16_t> battlefield_t::calculate_healing_to_stack(hero_t* caster, spell_e spell_id, uint32_t healing, battlefield_unit_t& unit, bool can_resurrect) {
	if(!caster || spell_id == SPELL_UNKNOWN)
		return {0, 0};

	auto hp_to_heal = healing;
	if(spell_id == SPELL_RESURRECTION) {
		const auto& sp_info = game_config::get_spell(spell_id);
		int effectiveness_percent = sp_info.multiplier[1].get_value(caster->get_effective_power(), caster->get_spell_effect_multiplier(spell_id));
		float actual_effectiveness = pow(effectiveness_percent / 100.f, unit.resurrection_count++);
		hp_to_heal = std::round(hp_to_heal * (actual_effectiveness));
	}

	return apply_healing_to_stack(hp_to_heal, unit, can_resurrect, true);
}

std::pair<uint32_t, uint16_t> battlefield_t::apply_healing_to_stack(uint32_t healing, battlefield_unit_t& unit, bool can_resurrect, bool simulate) {
	bool was_dead = (unit.stack_size == 0);

	uint32_t adjusted_healing = healing;
	if(unit.has_buff(BUFF_DRAGON_MAGIC_DAMPER)) {
		auto& army_of_unit = unit.is_attacker ? attacking_army : defending_army;
		float multi = .2f;
		if(army_of_unit.hero && army_of_unit.hero->has_talent(TALENT_DRAGON_MASTER))
			multi = .6f;

		adjusted_healing = std::round(healing * multi);
	}

	uint32_t unit_max_hp = get_unit_adjusted_hp(unit);
	uint32_t total_max_hp = unit.original_stack_size * unit_max_hp;
	uint32_t current_total_hp = (unit.stack_size - 1) * unit_max_hp + unit.unit_health;

	uint32_t new_total_hp = std::min(current_total_hp + adjusted_healing, total_max_hp);
	uint32_t actual_heal_amount = new_total_hp - current_total_hp;

	if(!can_resurrect) {
		new_total_hp = std::min(new_total_hp, unit.stack_size * unit_max_hp);
		actual_heal_amount = new_total_hp - current_total_hp;
	}

	uint16_t new_stack_size = (new_total_hp + unit_max_hp - 1) / unit_max_hp;
	uint16_t revive_count = new_stack_size - unit.stack_size;

	if(!simulate) {
		unit.stack_size = new_stack_size;
		unit.unit_health = new_total_hp % unit_max_hp;
		if(unit.unit_health == 0 && new_stack_size > 0) {
			unit.unit_health = unit_max_hp;
		}

		if(was_dead) {
			auto unit_hex = hex_grid.get_hex(unit.x, unit.y);
			if(unit_hex->unit != nullptr)
				throw;

			unit_hex->unit = &unit;

			if(unit.is_two_hex()) {
				int tail_direction = unit.is_attacker ? -1 : 1;
				auto unit_tail_hex = hex_grid.get_hex(unit.x + tail_direction, unit.y);
				if(unit_tail_hex->unit != nullptr)
					throw;

				unit_tail_hex->unit = &unit;
			}
		}

	}

	return std::make_pair(actual_heal_amount, revive_count);



	/*
	uint32_t actual_heal_amount = 0;
	
	if(!can_resurrect) {
		actual_heal_amount = get_unit_adjusted_hp(unit) - std::min(get_unit_adjusted_hp(unit), ((uint32_t)unit.unit_health) + healing);
		unit.unit_health += actual_heal_amount;
		return std::make_pair(actual_heal_amount, 0);
	}
	
	int32_t total_hp = (unit.stack_size * get_unit_adjusted_hp(unit)) + unit.unit_health;

	total_hp += healing;

	//healing not enough to rez, short circuit here
	if(healing <= (get_unit_adjusted_hp(unit) - unit.unit_health)) {
		unit.unit_health += (uint16_t)healing;
		return std::make_pair(healing, 0);
	}
	
	healing -= (get_unit_adjusted_hp(unit) - unit.unit_health);
	
	uint16_t revive_count = 1 + (healing / (uint32_t)get_unit_adjusted_hp(unit));
	//can't rez to greater than max stack size
	revive_count = std::min((int)revive_count, unit.original_stack_size - unit.stack_size);
	unit.stack_size += revive_count;

	if(revive_count) {
		uint16_t leftover = healing % (revive_count * get_unit_adjusted_hp(unit));
		unit.unit_health = leftover == 0 ? get_unit_adjusted_hp(unit) : leftover;
		actual_heal_amount = (revive_count * get_unit_adjusted_hp(unit)) + leftover;
	}
	else {
		actual_heal_amount = get_unit_adjusted_hp(unit) - unit.unit_health;
		unit.unit_health = get_unit_adjusted_hp(unit);
	}
	
	return std::make_pair(actual_heal_amount, revive_count);
	*/
}

uint32_t battlefield_t::calculate_magic_damage_to_stack(uint32_t base_damage, battlefield_unit_t& defender, magic_damage_e damage_type) {
	auto resistance = get_unit_adjusted_resistance(defender, damage_type);

	double damage_percent = 1. - (resistance / 100.);
	return std::max(1u, (uint32_t)std::round(damage_percent * base_damage));
}

uint16_t battlefield_t::deal_magic_damage_to_stack(uint32_t damage, battlefield_unit_t& defender) {
	auto kills = deal_damage_to_stack(damage, defender, true);
	if(defender.stack_size == 0)
		return kills;

	auto friendly_army = (defender.is_attacker ? attacking_army : defending_army);
	if(friendly_army.is_affected_by_talent(TALENT_FEROCITY))
		defender.add_buff(BUFF_FEROCITY);

	return kills;
}

uint16_t battlefield_t::deal_damage_to_stack(uint32_t damage, battlefield_unit_t& defender, bool is_spell_damage) {
	auto starting_troop_count = defender.stack_size;
	int32_t total_hp = (get_unit_adjusted_hp(defender) * (defender.stack_size - 1)) + defender.unit_health;
	auto remaining_hp = std::max((int32_t)0, total_hp - (int32_t)damage);
	auto actual_damage_done = total_hp - remaining_hp;
	auto remaining_troops = std::max((uint32_t)0, (remaining_hp / get_unit_adjusted_hp(defender)) + (remaining_hp % get_unit_adjusted_hp(defender) != 0));
	auto kills = starting_troop_count - remaining_troops;

	assert(kills <= defender.stack_size);
	
	defender.stack_size -= kills;
	defender.unit_health = remaining_hp % get_unit_adjusted_hp(defender);
	if(defender.unit_health == 0)
		defender.unit_health = get_unit_adjusted_hp(defender);

	//fixme for berserked units??
	auto& damage_dealer_stats = (defender.is_attacker ? defender_stats : attacker_stats);
	auto& damage_receiver_stats = (defender.is_attacker ? attacker_stats : defender_stats);
	
	damage_dealer_stats.total_damage_done += actual_damage_done;
	damage_dealer_stats.total_units_killed += kills;
	damage_dealer_stats.total_physical_damage_done += (is_spell_damage ? 0 : actual_damage_done);
	damage_dealer_stats.total_spell_damage_done += (is_spell_damage ? actual_damage_done : 0);
	damage_receiver_stats.total_damage_received += actual_damage_done;
	damage_receiver_stats.total_units_lost += kills;
	damage_receiver_stats.total_physical_damage_received += (is_spell_damage ? 0 : actual_damage_done);
	damage_receiver_stats.total_spell_damage_received += (is_spell_damage ? actual_damage_done : 0);

	if(defender.stack_size == 0) {
		hex_grid.get_hex(defender.x, defender.y)->unit = nullptr;
		if(defender.is_two_hex()) {
			int tail_direction = defender.is_attacker ? -1 : 1;
			hex_grid.get_hex(defender.x + tail_direction, defender.y)->unit = nullptr;
		}

		//if overwhelm is in effect, a unit's death could result in fewer enemies surrounding any given unit
		if((attacking_hero && attacking_hero->has_talent(TALENT_OVERWHELM)) || (defending_hero && defending_hero->has_talent(TALENT_OVERWHELM)))
			update_all_units_overwhelm_status();
	}
	else { //certain buffs are automatically removed when a unit takes damage, such as blind, fear, seduce, etc.
		auto remove_buff_if_present = [this, &defender] (buff_e buff_id) {
			bool buff_removed = defender.remove_buff(buff_id);
			if(buff_removed && !is_quick_combat) {
				battle_action_t action;
				action.action = ACTION_BUFF_EXPIRED;
				action.acting_unit = defender;
				action.buff_id = buff_id;
				fn_emit_combat_action(action);
			}
		};

		remove_buff_if_present(BUFF_SEDUCED);
		remove_buff_if_present(BUFF_BLINDED);
		remove_buff_if_present(BUFF_FEARED);
		remove_buff_if_present(BUFF_PARALYZED);
		remove_buff_if_present(BUFF_FROZEN);
		remove_buff_if_present(BUFF_PACIFIED);
		remove_buff_if_present(BUFF_STUNNED);
	}
	
	return kills;
}

void battlefield_t::handle_post_attack_effects(battlefield_unit_t& attacker, battlefield_unit_t& defender, uint32_t damage, uint16_t kills, bool was_melee_attack, bool was_lucky_strike) {
	auto& army_of_attacker = attacker.is_attacker ? attacking_army : defending_army;
	auto& army_of_defender = defender.is_attacker ? attacking_army : defending_army;
	
	//attacking/getting hit grants rage to barbarians
	if(army_of_attacker.hero && army_of_attacker.hero->hero_class == HERO_BARBARIAN)
		army_of_attacker.hero->mana = std::min((uint16_t)(army_of_attacker.hero->mana + 1), army_of_attacker.hero->get_maximum_mana());
	if(army_of_defender.hero && army_of_defender.hero->hero_class == HERO_BARBARIAN)
		army_of_defender.hero->mana = std::min((uint16_t)(army_of_defender.hero->mana + 1), army_of_defender.hero->get_maximum_mana());
	
	if(army_of_attacker.is_affected_by_talent(TALENT_CRUSADE)) {
		if(defender.has_buff(BUFF_CRUSADE_DEBUFF_STACK)) {
			uint8_t crusade_debuff_stacks = std::min(10u, defender.get_buff(BUFF_CRUSADE_DEBUFF_STACK).magnitude + 1u);
			defender.get_buff(BUFF_CRUSADE_DEBUFF_STACK).magnitude = crusade_debuff_stacks;
		}
		else {
			defender.add_buff(BUFF_CRUSADE_DEBUFF_STACK, -1, 1);
		}
	}
	
	if(attacker.has_buff(BUFF_VAMPIRE_LIFESTEAL)) {
		//first check to see if defender is affected by lifesteal
		bool can_lifesteal = true;
		if(defender.has_buff(BUFF_UNDEAD))
			can_lifesteal = false;
		if(defender.has_buff(BUFF_ANIMATED))
			can_lifesteal = false;
		//BUFF_ELEMENTAL
		
		if(can_lifesteal) {
			double lifesteal_percent = .5;
			if(attacker.unit_type == UNIT_VAMPIRE && army_of_attacker.is_affected_by_talent(TALENT_VAMPIRE_LORD))
				lifesteal_percent += .25;

			if(attacker.unit_type == UNIT_VAMPIRE && army_of_attacker.hero && army_of_attacker.hero->specialty == SPECIALTY_VAMPIRES)
				lifesteal_percent += get_skill_value(.2f, .02f, army_of_attacker.hero->level);
			
			bool plural = attacker.stack_size > 1;
			//if we steal any hp, we steal a minimum of 1 hp
			uint32_t max_lifesteal_hp = std::max(1u, (uint32_t)std::round(lifesteal_percent * damage));
			auto actual_lifesteal = apply_healing_to_stack(max_lifesteal_hp, attacker, true);
			
			if(!is_quick_combat && actual_lifesteal.first > 0) {
				battle_action_t action;
				action.action = ACTION_UNIT_LIFESTEAL;
				action.affected_units.push_back({defender, actual_lifesteal.first, actual_lifesteal.second, actual_lifesteal.second > 0});
				action.acting_unit = attacker;
				action.buff_id = BUFF_VAMPIRE_LIFESTEAL;
				fn_emit_combat_action(action);
			}
		}
	}
	
	if(attacker.has_buff(BUFF_GHOST_SOULSTEAL)) {
		
	}

	if(attacker.has_buff(BUFF_FEROCITY))
		attacker.remove_buff(BUFF_FEROCITY);
		
	if(attacker.has_buff(BUFF_BERSERK))
		attacker.remove_buff(BUFF_BERSERK);

	if(defender.has_buff(BUFF_THORNS) && was_melee_attack) {
		int thorns_damage = defender.get_buff(BUFF_THORNS).magnitude;
		deal_magic_damage_to_stack(thorns_damage, attacker);
	}

	if(was_lucky_strike && army_of_attacker.is_affected_by_talent(TALENT_INSPIRATION))
		restore_hero_mana(army_of_attacker.hero, 1, TALENT_INSPIRATION);

	//at this point, if the defender is dead, the following effects cannot apply, so we bail out
	if(!defender.stack_size)
		return;

	buff_e applied_buff = BUFF_NONE;

	if(attacker.has_buff(BUFF_SEDUCE_ON_ATTACK) && utils::rand_chance(30)) {
		if(!defender.has_buff(BUFF_UNDEAD) && !defender.has_buff(BUFF_MIND_SPELL_IMMUNITY) && !defender.has_buff(BUFF_ANIMATED)) {
			defender.add_buff(BUFF_SEDUCED, 3);
			applied_buff = BUFF_SEDUCED;
		}
	}
	
	if(attacker.has_buff(BUFF_BLIND_ON_ATTACK) && utils::rand_chance(20)) {
		if(!defender.has_buff(BUFF_UNDEAD) && !defender.has_buff(BUFF_MIND_SPELL_IMMUNITY) && !defender.has_buff(BUFF_ANIMATED)) {
			defender.add_buff(BUFF_BLINDED, 3);
			applied_buff = BUFF_BLINDED;
		}
	}

	if(attacker.has_buff(BUFF_POISON_ON_ATTACK) && utils::rand_chance(40)) {
		if(!defender.has_buff(BUFF_UNDEAD) && !defender.has_buff(BUFF_ANIMATED)) { //&& !defender.has_buff(BUFF_POISON_IMMUNITY)) {
			uint8_t magnitude = attacker.stack_size > 255 ? 255 : (int8_t)attacker.stack_size;
			defender.add_buff(BUFF_POISONED, 3, magnitude);
			applied_buff = BUFF_POISONED;
		}
	}

	if(!was_melee_attack && army_of_attacker.is_affected_by_talent(TALENT_WING_CLIP) && defender.has_buff(BUFF_FLYER)) {
		defender.add_buff(BUFF_WING_CLIPPED, 1);
	}


	if(applied_buff != BUFF_NONE && !is_quick_combat) {
		battle_action_t action;
		action.action = ACTION_BUFF_APPLIED;
		action.affected_units.push_back({defender, 0, 0, false});
		action.acting_unit = attacker;
		action.buff_id = applied_buff;
		//action.log_message = log_message;
		fn_emit_combat_action(action);
	}

}

const int range_penalty_hex_distance = 10;

double battlefield_t::get_ranged_attack_penalty(battlefield_unit_t& attacker, int hex_x, int hex_y) const {
	if(attacker.has_buff(BUFF_NO_RANGE_PENALTY))
		return 1.0f;

	auto distance = hex_grid.distance(attacker.x, attacker.y, hex_x, hex_y);
	
	//todo: handle castle walls
	if(distance <= range_penalty_hex_distance)
		return 1.0f;
	
	float penalty = 0.5f;
	auto& army_of_attacker = attacker.is_attacker ? attacking_army : defending_army;
	if(army_of_attacker.is_affected_by_skill(SKILL_ARCHERY))
		penalty -= army_of_attacker.hero->get_secondary_skill_level(SKILL_ARCHERY) * 0.05f;

	return (1.0f - penalty);
}

double battlefield_t::get_ranged_attack_penalty(battlefield_unit_t& attacker, battlefield_unit_t& defender) const {
	return get_ranged_attack_penalty(attacker, defender.x, defender.y);
}

uint32_t battlefield_t::apply_ranged_attack_penalty(uint32_t damage, battlefield_unit_t& attacker, battlefield_unit_t& defender) {
	auto penalty = get_ranged_attack_penalty(attacker, defender);
	
	return (uint32_t)std::round(damage * penalty);
}

uint32_t battlefield_t::apply_shooter_melee_penalty(uint32_t damage, battlefield_unit_t&, battlefield_unit_t&) {
	return damage / 2;
}

int battlefield_t::is_hit_lucky(battlefield_unit_t& unit) {
	//return (rand() % 2 == 0);

	auto luck = get_unit_adjusted_luck(unit);
	if(luck == 0)
		return 0;

	if(luck < 0) {
		if(luck == -1)
			return (utils::rand_chance(8) ? -1 : 0);
		else if(luck == -2)
			return (utils::rand_chance(16) ? -1 : 0);
		else //luck <= -3
			return (utils::rand_chance(24) ? -1 : 0);
	}

	//positive luck
	if(luck == 1)
		return (utils::rand_chance(5) ? 1 : 0);
	else if(luck == 2)
		return (utils::rand_chance(10) ? 1 : 0);
	else if(luck == 3)
		return (utils::rand_chance(15) ? 1 : 0);

	//if we get here, luck > 3, and we need to check if the unit's hero
	//has luck or not to see if they can benefit from additional luck
	auto& army_of_unit = unit.is_attacker ? attacking_army : defending_army;
	if(!army_of_unit.hero || !army_of_unit.hero->get_secondary_skill_level(SKILL_LUCK)) //no benefit
		return (utils::rand_chance(15) ? 1 : 0);

	//we do benefit from luck > +3
	auto chance = 15 + 5 * (pow(luck - 3, .6));
	return (utils::rand_chance(chance) ? 1 : 0);
}

uint32_t battlefield_t::apply_luck_damage_modifier(const battlefield_unit_t& unit, uint32_t base_damage, int luck_effect) {
	if(luck_effect == 0)
		return base_damage;

	if(luck_effect == -1)
		return std::max(1u, base_damage / 2);

	auto& army_of_unit = unit.is_attacker ? attacking_army : defending_army;
	int luck_level = army_of_unit.hero ? army_of_unit.hero->get_secondary_skill_level(SKILL_LUCK) : 0;

	double damage_bonus = 1.5 + get_skill_value(.2, .1, luck_level);

	//we deal at least 2 damage if we get a lucky strike
	return std::max(2u, (uint32_t)(std::round(base_damage * damage_bonus)));
}

bool battlefield_t::attack_unit(battlefield_unit_t& attacker, battlefield_unit_t* target_unit, bool use_ranged_attack, int hex_x, int hex_y, battlefield_hex_t* source_movement_hex) {
	auto& army_of_attacker = attacker.is_attacker ? attacking_army : defending_army;
	auto& attacking_unit_stats = (attacker.is_attacker ? attacker_stats : defender_stats);
	auto& target_unit_stats = (attacker.is_attacker ? defender_stats : attacker_stats);
	bool ranged_attack = use_ranged_attack && can_troop_shoot(&attacker);
	
	//see if we get a luck proc
	int luck_effect = is_hit_lucky(attacker);

	//handle units with AOE attacks (lich, demon)
	bool death_cloud = attacker.has_buff(BUFF_LICH_DEATH_CLOUD_ATTACK);
	bool fireball = attacker.has_buff(BUFF_MAGOG_FIREBALL_ATTACK);
	std::vector<std::pair<battlefield_unit_t*, std::pair<uint32_t, uint16_t>>> units_hit;
	if(ranged_attack && (death_cloud || fireball)) {
		int total_kills = 0;
		uint32_t total_damage = 0;
		int total_units_hit = 0;

		for(auto i = 0; i < 7; i++)	{
			battlefield_hex_t* hex = nullptr;
			if(i == 0)
				hex = hex_grid.get_hex(hex_x, hex_y);
			else
				hex = hex_grid.get_adjacent_hex(hex_x, hex_y, battlefield_direction_e((int)battlefield_direction_e::TOPLEFT + (i-1)));

			if(!hex)
				continue;

			auto unit = get_unit_on_hex(hex->x, hex->y);
			if(!unit || unit->stack_size == 0)
				continue;

			bool already_hit = false;
			for(const auto& hit_unit : units_hit) {
				if(unit == hit_unit.first) {
					already_hit = true;
					break;
				}
			}

			//2-hex units in multiple adjacent hexes don't get hit twice
			if(already_hit)
				continue;

			if(i != 0 && death_cloud && unit->is_undead())
				continue;

			if(i != 0 && fireball && unit->has_buff(BUFF_FIRE_IMMUNITY))
				continue;

			auto adjacent_damage = calculate_damage(attacker, *unit, ranged_attack, false);
			adjacent_damage = apply_luck_damage_modifier(attacker, adjacent_damage, luck_effect);
			total_damage += adjacent_damage;
			auto adjacent_kills = deal_damage_to_stack(adjacent_damage, *unit);
			total_kills += adjacent_kills;
			units_hit.push_back({unit, {adjacent_damage, adjacent_kills}});

			handle_post_attack_effects(attacker, *unit, adjacent_damage, adjacent_kills, false, luck_effect == 1);
		}

		if(!is_quick_combat) {
			battle_action_t action;
			action.action = ACTION_RANGED_AOE_ATTACK;
			for(auto& au : units_hit)
				action.affected_units.push_back({*au.first, au.second.first, au.second.second, (au.first->stack_size == 0)});
			
			action.acting_unit = attacker;
			action.luck_effect = luck_effect;
			action.target_hex = make_hex_location(hex_x, hex_y);
			fn_emit_combat_action(action);
		}

		//we can potentially morale here
		bool morale = utils::rand_chance(get_unit_morale_chance(attacker)) && can_troop_morale(attacker);
		if(morale) {
			if(!is_quick_combat) {
				battle_action_t action;
				action.action = ACTION_MORALED;
				action.acting_unit = attacker;
				fn_emit_combat_action(action);
			}
			attacker.has_moraled = true;
			attacker.has_moved = false;
		}
		else {
			finish_troop_action(&attacker);
		}

		if(!troops_remain())
			end_combat();

		return 1;
	}

	//the only valid call to this function with a null 'target_unit' parameter is if
	//we are attacking a hex using a ranged aoe attack (lich/gog), which is handled above
	if(!target_unit)
		return false;

	battlefield_unit_t& defender = *target_unit;
	auto& army_of_defender = defender.is_attacker ? attacking_army : defending_army;
	//melee units cannot attack non-adjacent units
	if(!ranged_attack && !are_units_adjacent(&attacker, &defender))
		return false;
	
	//unit loses ON_GUARD buff once they take an action
	attacker.remove_buff(BUFF_ON_GUARD);

	auto attack_from_hex = hex_grid.get_hex(hex_x, hex_y);
	
	auto damage = calculate_damage(attacker, defender, ranged_attack, false, attack_from_hex, source_movement_hex);
	damage = apply_luck_damage_modifier(attacker, damage, luck_effect);
	auto kills = deal_damage_to_stack(damage, defender);

	//apply bonechill debuff on melee attack (if talent applies)
	if(!ranged_attack && attacker.has_buff(BUFF_UNDEAD) && army_of_attacker.is_affected_by_talent(TALENT_BONECHILL) && defender.stack_size) {
		defender.add_buff(BUFF_BONECHILLED, 2);

		if(!is_quick_combat) {
			battle_action_t action;
			action.action = ACTION_BUFF_APPLIED;
			action.affected_units.push_back({ defender, 0, 0, false });
			action.buff_id = BUFF_BONECHILLED;
			fn_emit_combat_action(action);
		}
	}

	battle_action_t attack_action;
	if(!is_quick_combat) {
		attack_action.action = ranged_attack ? ACTION_RANGED_ATTACK : ACTION_MELEE_ATTACK;
		attack_action.affected_units.push_back({defender, damage, kills, (defender.stack_size == 0)});
		attack_action.acting_unit = attacker;
		attack_action.luck_effect = luck_effect;
		attack_action.target_hex = make_hex_location(hex_x, hex_y);
	}
	
	//handle breath attack
	if(!ranged_attack && attacker.has_buff(BUFF_DRAGON_BREATH_ATTACK)) {
		auto breath_dir = hex_grid.get_adjacent_hex_direction(hex_x, hex_y, target_unit->x, target_unit->y);
		auto breath_hex = hex_grid.get_adjacent_hex(target_unit->x, target_unit->y, breath_dir);
		auto breath_target = breath_hex ? breath_hex->unit : nullptr;
		auto breath_immune = army_of_defender.hero && army_of_defender.hero->is_artifact_in_effect(ARTIFACT_SCALES_OF_THE_RED_DRAGON);
		if(breath_hex && breath_target && !breath_immune && (breath_target != target_unit)) { //can't hit the same unit twice
			auto breath_damage = calculate_damage(attacker, *breath_target, false, false);
			breath_damage = apply_luck_damage_modifier(attacker, breath_damage, luck_effect);
			auto breath_kills = deal_damage_to_stack(breath_damage, *breath_target);
			handle_post_attack_effects(attacker, *breath_target, breath_damage, breath_kills, false, false); //lucky strike effects do not apply to breath hits

			if(!is_quick_combat)
				attack_action.affected_units.push_back({*breath_target, breath_damage, breath_kills, (breath_target->stack_size == 0)});
		}		
	}

	if(!is_quick_combat)
		fn_emit_combat_action(attack_action);
	
	handle_post_attack_effects(attacker, defender, damage, kills, ranged_attack, luck_effect == 1);

	int retaliation_count = 0;
	if(!ranged_attack && will_defender_retaliate(attacker, defender))
		retaliation_count = 1;

	//check for additional retaliation via Vengeance proc
	if(retaliation_count && utils::rand_chance(30) && attacker.stack_size > 0 && army_of_defender.is_affected_by_talent(TALENT_VENGEANCE))
		retaliation_count = 2;

	for(int retaliation_number = 0; retaliation_number < retaliation_count; retaliation_number++) {
		if(!defender.stack_size || !attacker.stack_size || defender.is_disabled())
			break;

		damage = calculate_damage(defender, attacker, false, true);
		luck_effect = is_hit_lucky(defender);
		damage = apply_luck_damage_modifier(defender, damage, luck_effect);
		kills = deal_damage_to_stack(damage, attacker);
		
		battle_action_t retaliation_action;
		if(!is_quick_combat) {
			retaliation_action.action = ACTION_RETALIATION;
			retaliation_action.affected_units.push_back({attacker, damage, kills, (attacker.stack_size == 0)});
			retaliation_action.acting_unit = defender;
			retaliation_action.luck_effect = luck_effect;
		}

		//handle breath attack during retaliation
		if(!ranged_attack && defender.has_buff(BUFF_DRAGON_BREATH_ATTACK)) {
			auto defender_effective_x = defender.x;
			//for two-hex units, we need to pick the adjacent hex, which could be the tail hex
			if(hex_grid.distance(defender_effective_x, defender.y, attacker.x, attacker.y) != 1)
				defender_effective_x += (attacker.x < defender.x ? -1 : 1);

			auto breath_dir = hex_grid.get_adjacent_hex_direction(defender_effective_x, defender.y, attacker.x, attacker.y);
			auto breath_hex = hex_grid.get_adjacent_hex(attacker.x, attacker.y, breath_dir);
			auto breath_target = breath_hex ? breath_hex->unit : nullptr;
			auto breath_immune = army_of_attacker.hero && army_of_attacker.hero->is_artifact_in_effect(ARTIFACT_SCALES_OF_THE_RED_DRAGON);
			if(breath_hex && breath_target && !breath_immune && (breath_target != &attacker)) {
				auto breath_damage = calculate_damage(defender, *breath_target, false, false);
				breath_damage = apply_luck_damage_modifier(defender, breath_damage, luck_effect);
				auto breath_kills = deal_damage_to_stack(breath_damage, *breath_target);
				handle_post_attack_effects(defender, *breath_target, breath_damage, breath_kills, false, luck_effect == 1);

				if(!is_quick_combat)
					retaliation_action.affected_units.push_back({*breath_target, breath_damage, breath_kills, (breath_target->stack_size == 0)});
			}		
		}

		if(!defender.has_buff(BUFF_UNLIMITED_RETALIATIONS))
			defender.retaliations_remaining = std::max(0, defender.retaliations_remaining - 1);
		
		if(!is_quick_combat)
			fn_emit_combat_action(retaliation_action);

		handle_post_attack_effects(defender, attacker, damage, kills, ranged_attack, luck_effect == 1);
	}
	
	//handle units with a second (or more) attack(s)
	bool quickdraw_proc = false;
	int additional_attack_count = 0;
	if(attacker.has_buff(BUFF_DOUBLE_ATTACK) || ((attacker.unit_type == UNIT_FOOTMAN) && army_of_attacker.is_affected_by_specialty(SPECIALTY_FOOTMEN)))
		additional_attack_count++;
	if((ranged_attack && attacker.has_buff(BUFF_SHOOTS_TWICE)))
		additional_attack_count++;
	if(ranged_attack && army_of_attacker.is_affected_by_talent(TALENT_QUICKDRAW) && utils::rand_chance(20)) { //quickdraw proc
		additional_attack_count++;
		quickdraw_proc = true;
	}
	
	if(ranged_attack && attacker.unit_type == UNIT_BALLISTA) {
		if(army_of_attacker.is_affected_by_skill(SKILL_WAR_MACHINES))
			additional_attack_count++;
		if(army_of_attacker.is_affected_by_talent(TALENT_MACHINIST))
			additional_attack_count++;
	}
	if(ranged_attack && attacker.unit_type == UNIT_ORC && army_of_attacker.is_affected_by_specialty(SPECIALTY_ORCS)) {
		const auto& sp = game_config::get_specialty(SPECIALTY_ORCS);
		int proc_chance = sp.multiplier.get_value(army_of_attacker.hero ? army_of_attacker.hero->level : 0);
		if(utils::rand_chance(proc_chance))
			additional_attack_count++;
	}

	for(int additional_attack_number = 0; additional_attack_number < additional_attack_count; additional_attack_number++) {
		if(!defender.stack_size || !attacker.stack_size || attacker.is_disabled())
			break;
		
		damage = calculate_damage(attacker, defender, ranged_attack, false);
		luck_effect = is_hit_lucky(attacker);
		damage = apply_luck_damage_modifier(attacker, damage, luck_effect);

		//handle certain specialties where a unit's second attack deals more damage
		if((attacker.unit_type == UNIT_CRUSADER && army_of_attacker.is_affected_by_specialty(SPECIALTY_CRUSADERS)) ||
			(attacker.unit_type == UNIT_WOLF && army_of_attacker.is_affected_by_specialty(SPECIALTY_WOLVES))) {
			damage *= army_of_attacker.hero->get_specialty_multiplier();
		}

		kills = deal_damage_to_stack(damage, defender);

		if(!is_quick_combat) {
			battle_action_t action;
			action.action = ranged_attack ? ACTION_RANGED_ATTACK : ACTION_MELEE_ATTACK;
			action.affected_units.push_back({defender, damage, kills, (defender.stack_size == 0)});
			action.acting_unit = attacker;
			action.luck_effect = luck_effect;
			action.target_hex = make_hex_location(hex_x, hex_y);
			if(quickdraw_proc)
				action.applicable_talent = TALENT_QUICKDRAW;
			
			fn_emit_combat_action(action);
		}

		handle_post_attack_effects(attacker, defender, damage, kills, ranged_attack, luck_effect == 1);
	}
	
	//we can potentially morale here
	bool morale = utils::rand_chance(get_unit_morale_chance(attacker)) && can_troop_morale(attacker);
	if(morale) {
		if(!is_quick_combat) {
			battle_action_t action;
			action.action = ACTION_MORALED;
			action.acting_unit = attacker;
			bool plural = attacker.stack_size > 1;
			fn_emit_combat_action(action);
		}
		attacker.has_moraled = true;
		attacker.has_moved = false;
	}
	else {
		finish_troop_action(&attacker);
	}
	
	if(!troops_remain())
		end_combat();
	
	return true;
}

battlefield_unit_t* battlefield_t::get_unit_by_id(int8_t unit_id) {
	if(unit_id == -1) //default/invalid value
		return nullptr;

	for(auto& tr : attacking_army.troops) {
		if(tr.troop_id == unit_id) {
			return &tr;
		}
	}
	for(auto& tr : defending_army.troops) {
		if(tr.troop_id == unit_id) {
			return &tr;
		}
	}

	return nullptr;
	//if(unit_id < 0 || unit_id >= (int8_t)(attacking_army.troops.size() + defending_army.troops.size()))
	//	return nullptr;
	//
	//if(unit_id < (int8_t)attacking_army.troops.size())
	//	return &attacking_army.troops[unit_id];
	//else
	//	return &defending_army.troops[unit_id - attacking_army.troops.size()];
}

battlefield_unit_t* battlefield_t::get_unit_on_hex(hex_location_t point) {
	return get_unit_on_hex(point.first, point.second);
}

//this function only returns units that are able to be resurrected/revived
//fixme: change to wrapper function get_revivable_unit_on_hex(sint x, sint y) which checks for clearance
battlefield_unit_t* battlefield_t::get_dead_unit_on_hex(sint x, sint y) {
	for(auto i = 0; i < army_t::MAX_BATTLEFIELD_TROOPS; i++) {
		auto& at = attacking_army.troops[i];
		auto& dt = defending_army.troops[i];

		if(at.stack_size == 0 && at.unit_type != UNIT_UNKNOWN && at.x == x) {
			int tail_direction = at.is_attacker ? -1 : 1;
			if(at.is_two_hex() && (at.y == y || (at.y + tail_direction) == y))
				return &at;
			else if(!at.is_two_hex() && at.y == y)
				return &at;
		}

		if(dt.stack_size == 0 && dt.unit_type != UNIT_UNKNOWN && dt.x == x) {
			int tail_direction = dt.is_attacker ? -1 : 1;
			if(dt.is_two_hex() && (dt.y == y || (dt.y + tail_direction) == y))
				return &dt;
			else if(!dt.is_two_hex() && dt.y == y)
				return &dt;
		}
	}

	return nullptr;
}

battlefield_unit_t* battlefield_t::get_unit_on_hex(sint x, sint y) {
	auto hex = hex_grid.get_hex(x, y);
	return hex ? hex->unit : nullptr;
}

bool battlefield_t::is_spell_target_valid(hero_t* caster, int target_x, int target_y, spell_e spell_id) {
	if(!caster || !hex_grid.get_hex(target_x, target_y) || spell_id == SPELL_UNKNOWN)
		return false;

	const auto& spell_info = game_config::get_spell(spell_id);
	switch(spell_info.target) { 
		case TARGET_NONE:
			return false;
		
		case TARGET_HEX_RADIUS_1_EXCLUDE_CENTER:
		case TARGET_HEX_RADIUS_1:
		case TARGET_HEX_RADIUS_2:
		case TARGET_HEX_RADIUS_3:
			return true;

		case TARGET_SINGLE_ALLY:
		case TARGET_SINGLE_ENEMY: {
			auto target = get_unit_on_hex(target_x, target_y);
			if(!target && (spell_id == SPELL_RESURRECTION || spell_id == SPELL_REANIMATE_DEAD || spell_id == SPELL_ARCANE_REANIMATION))
				target = get_dead_unit_on_hex(target_x, target_y);

			return is_spell_target_valid(caster, target, spell_id);
		}

	}

	return true;
}

bool battlefield_t::is_unit_immune_to_spell(const battlefield_unit_t* unit, spell_e spell_id) const {
	if(unit->unit_type == UNIT_CATAPULT)
		return true;
	
	if(unit->has_buff(BUFF_UNDEAD)) {
		if(spell_id == SPELL_BLESS || spell_id == SPELL_MASS_BLESS)
			return true;
	}
	if(unit->has_buff(BUFF_ANIMATED) || unit->has_buff(BUFF_UNDEAD)) {
		if(spell_id == SPELL_RESURRECTION)
			return false;
	}

	if(spell_id == SPELL_ARCANE_REANIMATION)
		return (unit->unit_type == UNIT_GOLEM || unit->unit_type == UNIT_ARCANE_CONSTRUCT);

	if(unit->has_buff(BUFF_MIND_SPELL_IMMUNITY) || unit->has_buff(BUFF_ANIMATED) || unit->has_buff(BUFF_UNDEAD)) {
		if(spell_id == SPELL_PARALYZE || spell_id == SPELL_CURSE || spell_id == SPELL_MASS_CURSE ||
			spell_id == SPELL_BERSERK || spell_id == SPELL_SORROW || spell_id == SPELL_FEAR || spell_id == SPELL_DAMNATION ||
			spell_id == SPELL_BLIND || spell_id == SPELL_PACIFY)
			return true;
	}

	if(unit->is_turret() || unit->unit_type == UNIT_BALLISTA) {
		if(spell_id == SPELL_MASS_BLESS || spell_id == SPELL_MASS_CURSE)
			return true;

		return false;
	}

	const auto& spell_info = game_config::get_spell(spell_id);
	if(spell_info.school == SCHOOL_ARCANE && unit->has_buff(BUFF_ARCANE_IMMUNITY))
		return true;
	if(spell_info.damage_type == MAGIC_DAMAGE_FROST && unit->has_buff(BUFF_FROST_IMMUNITY))
		return true;
	if(spell_info.damage_type == MAGIC_DAMAGE_FIRE && unit->has_buff(BUFF_FIRE_IMMUNITY))
		return true;
	if(spell_info.damage_type == MAGIC_DAMAGE_LIGHTNING && unit->has_buff(BUFF_LIGHTNING_IMMUNITY))
		return true;

	return false;
}

bool battlefield_t::is_spell_target_valid(hero_t* caster, battlefield_unit_t* unit, spell_e spell_id) {
	if(!caster || !unit || spell_id == SPELL_UNKNOWN)
		return false;
	
	auto& army_of_unit = unit->is_attacker ? attacking_army : defending_army;

	if(army_of_unit.is_affected_by_talent(TALENT_HOLY_WARD) && (spell_id == SPELL_CURSE || spell_id == SPELL_MASS_CURSE))
		return false;

	if(caster == attacking_hero && attacking_hero_used_cast)
		return false;
	else if(caster == defending_hero && defending_hero_used_cast)
		return false;
	
	if(is_unit_immune_to_spell(unit, spell_id))
		return false;
		
	bool is_target_ally = (unit->is_attacker == (caster == attacking_hero));
	const auto& spell_info = game_config::get_spell(spell_id);
	if(spell_info.target == TARGET_NONE)
		return false;
	else if((spell_info.target == TARGET_SINGLE_ALLY || spell_info.target == TARGET_ALL_ALLIED) && !is_target_ally)
		return false;
	else if((spell_info.target == TARGET_SINGLE_ENEMY || spell_info.target == TARGET_ALL_ENEMY) && is_target_ally)
		return false;

	//todo: check for healing/resurrection spells not healing because the target is at full hp
	// 
	
	
//	TARGET_HEX_RADIUS_1,
//	TARGET_HEX_RADIUS_2,
//	TARGET_HEX_RADIUS_3,
//	TARGET_HEX_RADIUS_1_EXCLUDE_CENTER,
//	TARGET_2_HEX_UP_DOWN,
//	TARGET_2_HEX_LEFT_RIGHT,
//	TARGET_3_HEX_UP_DOWN,
//	TARGET_3_HEX_LEFT_RIGHT,
	
	return true;
}

battle_result_e battlefield_t::end_combat() {
	if(result != BATTLE_IN_PROGRESS)
		return result;
	
	int attacker_remaining = 0;
	int defender_remaining = 0;
	
	//if(log_level > 1)
	//	combat_log("Attacker Casualties:");
	
	for(auto& tr : attacking_army.troops) {
		attacker_remaining += tr.stack_size;
		auto losses = tr.original_stack_size - tr.stack_size;
		if(losses == 0)
			continue;
		
		//if(log_level > 1)
		//	combat_log(tr.get_unit_name() + " - " + std::to_string(losses) + "\t");
	}
	
	//if(log_level > 1)
	//	combat_log("Defender Casualties:");
	
	for(auto& tr : defending_army.troops) {
		defender_remaining += tr.stack_size;
		auto losses = tr.original_stack_size - tr.stack_size;
		if(losses == 0)
			continue;
		
		//if(log_level > 1)
		//	combat_log(tr.get_unit_name() + " - " + std::to_string(losses) + "\t");
	}
	
	if(attacker_remaining == 0 && defender_remaining == 0) //could happen with summons, or certain spells (e.g. chain lightning)
		result = BATTLE_BOTH_LOSE;
	else if(attacker_remaining == 0)
		result = BATTLE_DEFENDER_VICTORY;
	else if(defender_remaining == 0)
		result = BATTLE_ATTACKER_VICTORY;
	else //draw condition, could happen if combat is unable to be resolved
		result = BATTLE_DEFENDER_VICTORY;
		
	calculate_necromancy_results(result);
	calculate_captured_artifacts(result);
	
	return result;
}

void battlefield_t::calculate_necromancy_results(battle_result_e battle_result) {
	if(battle_result == BATTLE_BOTH_LOSE)
		return;
	
	auto calc_necro = [this](hero_t* hero, const army_t& army_to_raise_from) {
		std::vector<troop_t> reanimation_pool;
		
		//we need to handle the situation where the hero has insufficient troop slots
		//and no matching units in his/her army.  if this is the case, we need to calc
		//the number of potential open army slots (after grouping up any units) and also
		//the number and type of undead in the army, to determine what to raise
		std::set<unit_type_e> existing_undead;
		int potential_open_slots = 0;
		//for(
		
		
		//todo: issue here where, if there are multiple stacks of the same troop type,
		//we could run into rounding issues (i.e. if there are 7 stacks of 2 infantrymen,
		//and we can raise 20%, we need to group them up into a stack of 14 to correctly
		//get the number to potentially raise
		for(const auto& tr : army_to_raise_from.troops) {
			if(tr.unit_type == UNIT_UNKNOWN)
				continue;
			
			uint16_t loss_count = tr.original_stack_size - tr.stack_size;
			const auto& cr = game_config::get_creature(tr.unit_type);
			if(!loss_count || tr.was_summoned || cr.reanimates_as_unit_type == UNIT_UNKNOWN)
				continue;
			
			reanimation_pool.push_back({tr.unit_type, loss_count});
		}
		
		std::sort(reanimation_pool.begin(), reanimation_pool.end(), [](troop_t& lhs, troop_t& rhs) {
			auto l_reanims = game_config::get_creature(lhs.unit_type).reanimates_as_unit_type;
			auto r_reanims = game_config::get_creature(rhs.unit_type).reanimates_as_unit_type;
			return game_config::get_creature(l_reanims).tier < game_config::get_creature(r_reanims).tier;
		});
		
		int slot = 0;
		for(auto& tr : reanimation_pool) {
			const auto& cr = game_config::get_creature(tr.unit_type);
			int total = std::min((uint16_t)(hero->dark_energy / cr.health), tr.stack_size);
			if(total == 0)
				continue;
			
			int de_cost = total * cr.health;
			hero->dark_energy -= de_cost;
			were_troops_raised = true;
			necromancy_raised_troops.troops[slot].unit_type = cr.reanimates_as_unit_type;
			necromancy_raised_troops.troops[slot].stack_size = total;
		}
	};
	
	if(battle_result == BATTLE_ATTACKER_VICTORY) {
		if(attacking_hero && attacking_hero->get_secondary_skill_level(SKILL_NECROMANCY) != 0)
			calc_necro(attacking_hero, defending_army);
	}
	else if(battle_result == BATTLE_DEFENDER_VICTORY) {
		if(defending_hero && defending_hero->get_secondary_skill_level(SKILL_NECROMANCY) != 0)
			calc_necro(defending_hero, attacking_army);
	}
}

void battlefield_t::calculate_captured_artifacts(battle_result_e battle_result) {
	if(battle_result == BATTLE_BOTH_LOSE)
		return;

	hero_t* defeated_hero = (battle_result == BATTLE_ATTACKER_VICTORY) ? defending_hero : attacking_hero;
	hero_t* winning_hero = (battle_result == BATTLE_ATTACKER_VICTORY) ? attacking_hero : defending_hero;
	
	if(!defeated_hero || !winning_hero)
		return;

	std::deque<artifact_e> potential_captured_artifacts;
	for(auto a : defeated_hero->artifacts) {
		if(a != ARTIFACT_NONE)
			potential_captured_artifacts.push_back(a);
	}
	for(auto a : defeated_hero->backpack) {
		if(a != ARTIFACT_NONE)
			potential_captured_artifacts.push_back(a);
	}

	std::sort(potential_captured_artifacts.begin(), potential_captured_artifacts.end(), [](const artifact_e lhs, const artifact_e rhs) {
		const auto& lhsinfo = game_config::get_artifact(lhs);
		const auto& rhsinfo = game_config::get_artifact(rhs);
		return lhsinfo.rarity < rhsinfo.rarity;
	});

	while(potential_captured_artifacts.size()) { //fixme //&& battle.attacking_hero->pickup_artifact(potential_captured_artifacts.back())) {
		captured_artifacts.push_back(potential_captured_artifacts.back());
		potential_captured_artifacts.pop_back();
	}
}

battle_result_e battlefield_t::compute_quick_combat() {
	if(!combat_started)
		start_combat();

	is_quick_combat = true;

	while(true) {
		//move_unit(unit_move_queue.first());
		auto_move_troop();

		if(!troops_remain())
			break;
			
		if(round > game_config::MAX_COMBAT_ROUNDS) {
			std::cout << "warning: combat exceeded maximum round limit.\n";
			break;
		}
	}
	
	is_quick_combat = false;
	return end_combat();
}

bool battlefield_t::is_troop_human_controlled(battlefield_unit_t* unit, game_t* game_instance) {
	if(!unit || !game_instance)
		return false;
	
	auto player = get_player_for_troop(unit);
	if(player == PLAYER_NONE || player == PLAYER_NEUTRAL)
		return false;
	
	return game_instance->get_player(player).is_human;
}

player_e battlefield_t::get_player_for_troop(battlefield_unit_t* unit) {
	if(!unit)
		return PLAYER_NONE;
	
	if(unit->is_attacker) {
		if(!attacking_hero) //shouldn't happen
			return PLAYER_NONE;
		
		return attacking_hero->player;
	}
	else { //unit is defender
		if(!defending_hero)
			return PLAYER_NEUTRAL;
		
		return defending_hero->player;
	}
	
	//return PLAYER_NONE;
}

troop_group_t get_remaining_troops(const army_t& army) {
	troop_group_t remaining;
	uint i = 0;
	for(auto& tr : remaining) {
		if(army.troops[i].was_summoned)
			continue;

		tr = troop_t(army.troops[i].stack_size == 0 ? UNIT_UNKNOWN : army.troops[i].unit_type, army.troops[i].stack_size);
		i++;
	}
	
	return remaining;
}

troop_group_t battlefield_t::get_remaining_troops_attacker() const {
	return get_remaining_troops(attacking_army);
}

troop_group_t battlefield_t::get_remaining_troops_defender() const {
	return get_remaining_troops(defending_army);
}

uint32_t battlefield_t::get_winning_hero_xp() const {
	uint32_t xp = 0;
	if(result == BATTLE_ATTACKER_VICTORY) {
		for(auto& tr : defending_army.troops) {
			auto& cr = game_config::get_creature(tr.unit_type);
			auto losses = tr.original_stack_size - tr.stack_size;
			xp += (losses * tr.get_base_max_hitpoints()) * (1. + .2*(cr.tier - 1));
		}
		if(defending_hero) {
			xp += 500 + (defending_hero->level * 50);
		}
	}
	else if(result == BATTLE_DEFENDER_VICTORY) {
		for(auto& tr : attacking_army.troops) {
			auto& cr = game_config::get_creature(tr.unit_type);
			auto losses = tr.original_stack_size - tr.stack_size;
			xp += (losses * tr.get_base_max_hitpoints()) * (1. + .2*(cr.tier - 1));
		}
		xp += 500 + (attacking_hero->level * 50);
	}
	
	
	return xp;
}
