#include "rmg.h"
#include "game.h"

#include "../../editor/src/perlin_noise.h"

#include "core/brushes.h"
#include "core/utils.h"

#include <queue>

const int TILE_SIZE = 128;

static std::mt19937_64 rng;

static uint32_t tile_hash(int x, int y) {
	uint32_t h = static_cast<uint32_t>(x) * 2246822519u ^ static_cast<uint32_t>(y) * 3266489917u;
	h ^= h >> 17;
	h *= 0xbf324c81u;
	h ^= h >> 11;
	h *= 0x9c7f03a5u;
	h ^= h >> 16;
	return h;
}

const tree_brush_t* get_tree_brush_for_tile(int tilex, int tiley, map_tile_t& tile) {
	static std::map<terrain_type_e, std::vector<const tree_brush_t*>> brushes_for_terrain;
	if(brushes_for_terrain.size() == 0)	{
		for(const auto& b : game_config::get_tree_brushes()) {
			for(int i = 1; i < b.native_terrain_types.size(); i++) {
				auto terrain = (terrain_type_e)i;
				if(b.native_terrain_types.test(i))
					brushes_for_terrain[terrain].push_back(&b);
			}
		}
	}

	auto tile_terrain = tile.terrain_type;
	if(!brushes_for_terrain.contains(tile_terrain) || !brushes_for_terrain[tile_terrain].size())
		return nullptr;

	//brush_index = (uint64_t)tile_hash(x, y) * tree_brushes.size() >> 32
	return brushes_for_terrain[tile_terrain][tile_hash(tilex, tiley) % brushes_for_terrain[tile_terrain].size()];
}

void draw_road_between_object_and_point(adventure_map_t& map, interactable_object_t* obj_start, coord_t end, road_type_e road_type);
void draw_road_between_objects(adventure_map_t& map, interactable_object_t* obj_start, interactable_object_t* obj_end, road_type_e road_type);
void draw_road_between_points(adventure_map_t& map, coord_t start, coord_t end, road_type_e road_type);

int rand_int() {
	return std::uniform_int_distribution<int>(0, INT_MAX)(rng);
}

std::pair<unit_type_e, int> get_monster_guard(int total_value, std::bitset<8> allowed_tiers = 0xf, bool melee = true, bool ranged = true) {
	std::vector<unit_type_e> potential_unit_types;
	for(auto& cr : game_config::get_creatures()) {
		troop_t troop;
		troop.unit_type = cr.unit_type;
		if(cr.unit_type == UNIT_UNKNOWN || troop.is_turret_or_war_machine() || troop.is_summoned_creature())
			continue;

		if(!allowed_tiers.test(cr.tier))
			continue;

		if(melee && ranged) //any
			potential_unit_types.push_back(cr.unit_type);
		else if(ranged && cr.has_inherent_buff(BUFF_SHOOTER)) //ranged only
			potential_unit_types.push_back(cr.unit_type);
		else if(melee && !cr.has_inherent_buff(BUFF_SHOOTER)) //melee only
			potential_unit_types.push_back(cr.unit_type);
	}

	if(!potential_unit_types.size())
		return { UNIT_UNKNOWN, 0 };

	auto unit_type = utils::rand_item(potential_unit_types, rng);
	const auto& cr = game_config::get_creature(unit_type);
	int per_unit_value = 3. * (1. + (cr.tier / 5.)) * (cr.health * .8) * (cr.has_inherent_buff(BUFF_SHOOTER) ? 1.2 : 1.);
	int base_count = total_value / per_unit_value;
	int unit_count = utils::rand_rangef(base_count * .9, base_count * 1.1, rng);

	return {unit_type, std::clamp(unit_count, 1, (int)game_config::MAX_TROOP_STACK_SIZE)};
}

coord_t get_rand_coord_in_range(int x, int y, int dist, int range = 0) {
	int total_dist = dist + utils::rand_range(-range, range, rng);
	int xoff = utils::rand_range(-total_dist, total_dist, rng);
	int yoff = (xoff >= 0) ? total_dist - xoff : total_dist + xoff;
	
	if(rand_int() % 2 == 0)
		yoff = -yoff;
	
	return {x + xoff, y + yoff};
}

coord_t get_rand_coord_in_biome(const adventure_map_t& map, const biome_t& biome, int tries = 25) {
	for(int i = 0; i < tries; i++) {
		int x = rand_int() % map.width;
		int y = rand_int() % map.height;
		if(map.get_tile(x, y).zone_id == biome.zone_id)
			return {x, y};
	}

	return {-1, -1};
}

void flood_fill_biome(adventure_map_t& map, biome_t& biome) {
	std::queue<coord_t> queue;
	queue.push(biome.terrain_center);
	
	// Ensure initial tile settings are consistent
	auto& center_tile = map.get_tile(biome.terrain_center.x, biome.terrain_center.y);
	auto initial_zone_id = center_tile.zone_id;
	center_tile.terrain_type = biome.terrain_type;
	center_tile.zone_id = biome.zone_id;

	while(!queue.empty()) {
		coord_t current = queue.front();
		queue.pop();

		// Directions for moving to adjacent tiles (4-connectivity: up, down, left, right)
		int dx[] = {0, 1, 0, -1};
		int dy[] = {1, 0, -1, 0};

		for (int i = 0; i < 4; ++i) {
			coord_t adj = {current.x + dx[i], current.y + dy[i]};
			if (!map.tile_valid(adj.x, adj.y))
				continue;

			auto& adj_tile = map.get_tile(adj.x, adj.y);

			// Only process adjacent tiles that match the biome's terrain type but have not been updated yet
			if (adj_tile.terrain_type == biome.terrain_type && adj_tile.zone_id == initial_zone_id) {
				adj_tile.zone_id = biome.zone_id;  // Update zone_id before enqueueing to prevent duplication
				queue.push(adj);
			}
		}
	}
};

void draw_road_between_objects(adventure_map_t& map, interactable_object_t* obj_start, interactable_object_t* obj_end, road_type_e road_type) {
	coord_t mi = {obj_end->x, obj_end->y};
	map.get_tile(mi.x, mi.y).road_type = road_type;
	mi.y++;
	map.get_tile(mi.x, mi.y).road_type = road_type;
	int ew_direction = (mi.x <= obj_start->x) ? 1 : -1;

	if(obj_end->y > obj_start->y) {
		auto padx = 2;
		if(obj_start->object_type == OBJECT_MAP_TOWN)
			padx = 4;

		for(int i = 0; i < padx; i++) {
			if(map.tile_valid(mi.x, mi.y)) {
				mi.x += ew_direction;
				map.get_tile(mi.x, mi.y).road_type = road_type;
			}
		}
	}
	
	draw_road_between_object_and_point(map, obj_start, mi, road_type);
}

void draw_road_between_object_and_point(adventure_map_t& map, interactable_object_t* obj_start, coord_t end, road_type_e road_type) {
	coord_t mi = { obj_start->x, obj_start->y };
	map.get_tile(mi.x, mi.y).road_type = road_type;
	mi.y++;
	map.get_tile(mi.x, mi.y).road_type = road_type;
	int ew_direction = (mi.x <= end.x) ? 1 : -1;

	if(obj_start->y > end.y) {
		auto padx = 2;
		if(obj_start->object_type == OBJECT_MAP_TOWN)
			padx = 4;

		for(int i = 0; i < padx; i++) {
			if(map.tile_valid(mi.x, mi.y)) {
				mi.x += ew_direction;
				map.get_tile(mi.x, mi.y).road_type = road_type;
			}
		}
	}
	
	draw_road_between_points(map, mi, end, road_type);
}

void draw_road_between_points(adventure_map_t& map, coord_t start, coord_t end, road_type_e road_type) {
	// Calculate the direction vector from start to end
	int dx = end.x - start.x;
	int dy = end.y - start.y;

	// Determine a perpendicular vector (dy, -dx) or (-dy, dx)
	int perpendicular_x = dy;
	int perpendicular_y = -dx;

	// Randomize the choice of perpendicular direction
	if (rand_int() % 2) {
		perpendicular_x = -dy;
		perpendicular_y = dx;
	}
	
	// Compute the magnitude of the deviation (can be tweaked)
	float min_deviation = -.5f;
	float max_deviation = .5f;
	float deviation_magnitude = utils::rand_rangef(min_deviation, max_deviation, rng);
	float deviation_delta = .1f;
	float deviation_direction = (rand_int() % 2 == 0) ? 1.f : -1.f;

	coord_t virtual_end;
	virtual_end.x = (int)(end.x + perpendicular_x * deviation_magnitude);
	virtual_end.y = (int)(end.y + perpendicular_y * deviation_magnitude);
	
	int sx = start.x < virtual_end.x ? 1 : -1;
	int sy = start.y < virtual_end.y ? 1 : -1;
	int err = std::abs(virtual_end.x - start.x) - std::abs(virtual_end.y - start.y);

	coord_t pos = start;
	int e2;

	while(true) {
		if(map.tile_valid(pos.x, pos.y) && map.get_tile(pos.x, pos.y).is_passable()) {
			map.get_tile(pos.x, pos.y).road_type = road_type;
		}
		
		if(pos.x == end.x && pos.y == end.y)
			break;
		
		// Recalculate the virtual end to gradually adjust to actual end
		if(utils::mh_dist(pos.x, pos.y, end.x, end.y) > 15) {
			deviation_magnitude += utils::rand_rangef(0.f, deviation_delta, rng) * deviation_direction;
			
			if(deviation_magnitude >= max_deviation) {
				deviation_magnitude = max_deviation;
				deviation_direction = -deviation_direction;
			}
			else if(deviation_magnitude <= min_deviation) {
				deviation_magnitude = min_deviation;
				deviation_direction = -deviation_direction;
			}
			
			virtual_end.x = (int)(end.x + perpendicular_x * deviation_magnitude);
			virtual_end.y = (int)(end.y + perpendicular_y * deviation_magnitude);
		}
		else { //we are close enough, just set the virtual end to the actual end
			virtual_end = end;
			//return;
		}

		// Re-calculate differences and error
		dx = std::abs(virtual_end.x - pos.x);
		sx = pos.x < virtual_end.x ? 1 : -1;
		dy = -std::abs(virtual_end.y - pos.y);
		sy = pos.y < virtual_end.y ? 1 : -1;
		err = dx + dy;
		
		e2 = 2 * err;
		if(e2 >= dy) {
			//if(pos.x == end.x)
			//	break;
			
			err += dy;
			pos.x += sx;
		}
		if(e2 <= dx) {
			//if(pos.y == end.y)
			//	break;
		
			err += dx;
			pos.y += sy;
		}

	}
}

// Bresenham's line algorithm to find connection point
coord_t find_zone_connection_point(adventure_map_t& map, coord_t start, coord_t end) {
	int dx = std::abs(end.x - start.x), sx = start.x < end.x ? 1 : -1;
	int dy = -std::abs(end.y - start.y), sy = start.y < end.y ? 1 : -1;
	int err = dx + dy, e2; /* error value e_xy */

	int current_zone = map.get_tile(start.x, start.y).zone_id;
	coord_t current_position = start;

	while(true) {  // The loop will break when the connection point is found
		if (start.x == end.x && start.y == end.y) break; // Optionally break if you reach the end
		e2 = 2 * err;
		if (e2 >= dy) { /* e_xy+e_x > 0 */
			if (start.x == end.x) break;
			err += dy;
			start.x += sx;
		}
		if (e2 <= dx) { /* e_xy+e_y < 0 */
			if (start.y == end.y) break;
			err += dx;
			start.y += sy;
		}

		// Check if there is a zone change
		if (map.get_tile(start.x, start.y).zone_id != current_zone) {
			return current_position;  // Spawn a guard at the last position in the current zone
			
		}
		current_position = start; // Update the current position
	}
	
	return {-1, -1};
}

void place_blockers(const adventure_map_t& map, QBitArray& obstacles, int x, int y, adventure_map_direction_e direction) {
	auto offset = [&map](int mx, int my) {
		return mx + map.width * my;
	};

	int count = 0;
	for(int i = -1; i < 2; i++) {
		if(direction == DIRECTION_EAST && map.tile_valid(x + 1, y + i) && map.get_tile(x + 1, y + i).is_passable())
			obstacles.setBit(offset(x + 1, y + i));
		else if(direction == DIRECTION_WEST && map.tile_valid(x - 1, y + i) && map.get_tile(x - 1, y + i).is_passable())
			obstacles.setBit(offset(x - 1, y + i));
		else if(direction == DIRECTION_NORTH && map.tile_valid(x + i, y - 1) && map.get_tile(x + i, y - 1).is_passable())
			obstacles.setBit(offset(x + i, y - 1));
		else if(direction == DIRECTION_SOUTH && map.tile_valid(x + i, y + 1) && map.get_tile(x + i, y + 1).is_passable())
			obstacles.setBit(offset(x + i, y + 1));
	}
}

int side_block_count(const adventure_map_t& map, const QBitArray& obstacles, int x, int y, adventure_map_direction_e direction) {
	auto is_tile_blocking = [&map, &obstacles](int tx, int ty) {
		if(!map.tile_valid(tx, ty) || !map.get_tile(tx, ty).is_passable())
			return true;

		auto offset = tx + map.width * ty;
		if(obstacles.testBit(offset))
			return true;

		return false;
	};

	int count = 0;
	for(int i = -1; i < 2; i++) {
		if(direction == DIRECTION_EAST)
			count += is_tile_blocking(x + 1, y + i) ? 1 : 0;
		else if(direction == DIRECTION_WEST)
			count += is_tile_blocking(x - 1, y + i) ? 1 : 0;
		else if(direction == DIRECTION_NORTH)
			count += is_tile_blocking(x + i, y - 1) ? 1 : 0;
		else if(direction == DIRECTION_SOUTH)
			count += is_tile_blocking(x + i, y + 1) ? 1 : 0;
	}

	return count;
}

bool does_object_fit_at_location(adventure_map_t& map, interactable_object_e object_type, coord_t pos, bool is_guarded, int16_t asset_id = 0) {
	if(!map.tile_valid(pos.x, pos.y))
		return false;

	const auto& obj_info = game_config::get_object_info(object_type, asset_id);
	for(auto y = 0; y < 8; y++) {
		for(auto x = 0; x < 8; x++) {
			auto offset = y*8 + x;
			auto tilex = pos.x - 3 + x;
			auto tiley = pos.y - 6 + y;

			if(!obj_info.interactability.test(offset) && obj_info.passability.test(offset))
				continue;

			if(!map.tile_valid(tilex, tiley))
				return false;

			const auto& tile = map.get_tile(tilex, tiley);
			if(!tile.is_passable() || tile.is_interactable())
				return false;

			if(tile.road_type != ROAD_NONE && !interactable_object_t::is_pickupable(object_type))
				return false;

			if(obj_info.interactability.test(offset)) {
				//if object is pickupable, it needs to be blocked by the top 3 or bottom 3 tiles for the guard to be effective
				if(interactable_object_t::is_pickupable(object_type)) {
					if(is_guarded) {
					}
				}
				else {
					if(!map.tile_valid(tilex, tiley + 1) || !map.get_tile(tilex, tiley + 1).is_passable())
						return false;
				}

			}
		}
	}

	return true;
}

bool add_object_to_map(interactable_object_t* object, adventure_map_t& map) {
	QBitArray dummy;
	return add_object_to_map(object, map, dummy);
}

bool add_object_to_map(interactable_object_t* object, adventure_map_t& map, QBitArray& obstacles) {
	if(!object)
		return false;

	auto object_offset = map.objects.size() + 1;
	map.objects.push_back(object);
	if(!map.tile_valid(object->x, object->y))
		return false;

	auto& tile = map.get_tile(object->x, object->y);
	tile.interactable_object = object_offset;
	tile.passability = 2;

	//skip setting passability for known 1x1 objects
	if(interactable_object_t::is_pickupable(object) || object->object_type == OBJECT_MAP_MONSTER) {
		if(obstacles.size())
			obstacles.clearBit(object->x + map.width * object->y);

		return true;
	}

	const auto& obj_info = game_config::get_object_info(object);
	for(auto y = 0; y < 8; y++) {
		for(auto x = 0; x < 8; x++) {
			auto offset = y*8 + x;
			auto tilex = object->x - 3 + x;
			auto tiley = object->y - 6 + y;
			if(!map.tile_valid(tilex, tiley))
				continue;

			if(obj_info.interactability.test(offset)) {
				map.get_tile(tilex, tiley).interactable_object = object_offset;
				map.get_tile(tilex, tiley).passability = 2;
				if(obstacles.size())
					obstacles.clearBit(tilex + map.width * tiley);
			}

			if(!obj_info.passability.test(offset)) {
				map.get_tile(tilex, tiley).passability = 0;
				if(obstacles.size())
					obstacles.clearBit(tilex + map.width * tiley);
			}
		}
	}

	return true;
}

bool is_road_diagonal(int x, int y, const adventure_map_t& map) {
	bool top = map.tile_valid(x - 1, y) && map.get_tile(x - 1, y).road_type != ROAD_NONE;
	bool bottom = map.tile_valid(x + 1, y) && map.get_tile(x + 1, y).road_type != ROAD_NONE;
	bool left = map.tile_valid(x, y - 1) && map.get_tile(x, y - 1).road_type != ROAD_NONE;
	bool right = map.tile_valid(x, y + 1) && map.get_tile(x, y + 1).road_type != ROAD_NONE;

	return (top && left) || (top && right) || (bottom && left) || (bottom && right);
}

rmg_result_e generate_random_map(adventure_map_t& map, std::vector<biome_t>& biomes, int64_t map_seed) {
	siv::PerlinNoise::seed_type seed = map_seed;
	if(seed == -1)
		seed = std::random_device()(); //(static_cast<uint64_t>(rd()) << 32) | rd();

	rng.seed(seed);
	const siv::PerlinNoise perlin{seed};

	const float frequency = 4.0;
	const double fx = (frequency / map.width);
	const double fy = (frequency / map.height);

	map.clear(true, false);
	
	for(auto& b : biomes) {
		if(b.faction == HERO_CLASS_ALL || b.faction == HERO_CLASS_NONE)
			b.faction = get_random_hero_class(HERO_CLASS_ALL, rng);
		
		if(b.terrain_type == TERRAIN_UNKNOWN)
			b.terrain_type = get_faction_native_terrain(b.faction);
		/*else if(no towns in biome) {
			if(b.terrain_type == TERRAIN_UNKNOWN)
				b.terrain_type = (terrain_type_e)(2 + rand_int() % 8);
		}*/
	}	

	for (int y = 0; y < map.height; ++y) {
		for (int x = 0; x < map.width; ++x) {
			map_tile_t& tile = map.get_tile(x, y);

			int dist = INT_MAX;
			int i = 2;
			terrain_type_e nearest = TERRAIN_WATER;
			int nearest_zone_id = -1;
			
			for(auto& b : biomes) {
				float scale = b.scale;
				
				float d = utils::eu_dist(x, y, b.terrain_center.x, b.terrain_center.y);
				float dr = 4000.f;
				auto tfx = 4 / 250.f;
				auto tfy = 4 / 250.f;
				auto df = (dr/2.f) - (perlin.octave2D_01(x * tfx * i, y * tfy * i, 1) * dr);
				d += df;
				if(d < (dist * scale)) {
					nearest = b.terrain_type;
					nearest_zone_id = b.zone_id;
					dist = d;
				}

				i++;
			}
			
			tile.terrain_type = nearest;
			tile.zone_id = 100 + nearest_zone_id;
			//clear any existing roads
			tile.road_type = ROAD_NONE;
		}
	}

	for(auto& b : biomes)
		flood_fill_biome(map, b);

	//fixup 'bleeding'
	for(int y = 0; y < map.height; y++) {
		for(int x = 0; x < map.width; x++) {
			auto& tile = map.get_tile(x, y);
			if(tile.zone_id < 100)
				continue;

			biome_t* closest_biome = nullptr;
			int closest_dist = 10000;
			for(auto& b : biomes) {
				auto d = utils::eu_dist(x, y, b.terrain_center.x, b.terrain_center.y);
				if(d < closest_dist) {
					closest_dist = d;
					closest_biome = &b;
				}
			}

			if(closest_biome) {
				tile.zone_id = closest_biome->zone_id;
				tile.terrain_type = closest_biome->terrain_type;
			}
		}
	}

	QBitArray obstacle_tiles;
	obstacle_tiles.resize(map.width * map.height);

	int current_town_id = 0;
	std::vector<town_t*> biome_towns;
	//add towns to each biome, if applicable
	for(auto& b : biomes) {

		//for(int i = 0; i < b.owned_town_count; i++) {
			auto main_town = (town_t*)interactable_object_t::make_new_object(OBJECT_MAP_TOWN);
			main_town->x = b.terrain_center.x;
			main_town->y = b.terrain_center.y;
		
			auto main_town_x = main_town->x;
			auto main_town_y = main_town->y;
		
			//main_town->town_type = town_t::hero_class_to_town_type(b.faction);
			main_town->player = b.player;
			main_town->setup_default_spells();
			//main_town->setup_buildings();
			//main_town->setup_default_spells();
			main_town->town_id = current_town_id++;

			add_object_to_map(main_town, map);
			biome_towns.push_back(main_town);
		//}

		//add mines
		for(int n = 0; n < 2; n++) {
			for(int i = 0; i < 10; i++) {
				auto pos = get_rand_coord_in_range(main_town_x, main_town_y, 15 - i, 5);
				
				if(!map.tile_valid(pos.x, pos.y) || !map.get_tile(pos.x, pos.y).is_passable() || map.get_tile(pos.x, pos.y).zone_id != b.zone_id)
					continue;
				
				//todo: set maximum y value, given that it cannot be accessed when
				//at the very bottom of the map
				
				auto mine = (mine_t*)interactable_object_t::make_new_object(OBJECT_MINE);
				mine->x = pos.x;
				mine->y = pos.y;
				//fixme
				mine->asset_id = 1331;
				
				if(n == 0)
					mine->mine_type = RESOURCE_WOOD;
				else if(n == 1)
					mine->mine_type = RESOURCE_ORE;
				//else
				
				add_object_to_map(mine, map);
				
				//put road between town and mines
				if(b.spawn_roads_to_basic_mines)
					draw_road_between_objects(map, mine, main_town, ROAD_COBBLESTONE);

				break;
			}
		}
		
		for(int i = 0; i < b.neutral_town_count; i++) {
			for(int n = 0; n < 10; n++) {
				auto pos = get_rand_coord_in_range(main_town_x, main_town_y, 40 - n, 5);
				
				if(!map.tile_valid(pos.x, pos.y) || !map.get_tile(pos.x, pos.y).is_passable() || map.get_tile(pos.x, pos.y).zone_id != b.zone_id)
					continue;

				bool is_guarded = true;
				if(!does_object_fit_at_location(map, OBJECT_MAP_TOWN, pos, is_guarded))
					continue;

				auto town = (town_t*)interactable_object_t::make_new_object(OBJECT_MAP_TOWN);
				town->x = pos.x;
				town->y = pos.y;
				town->town_type = town_t::hero_class_to_town_type(get_random_hero_class(HERO_CLASS_ALL, rng));
				town->player = PLAYER_NONE;
				town->town_id = current_town_id++;
				town->setup_default_spells();

				add_object_to_map(town, map);
				draw_road_between_objects(map, main_town, town, ROAD_COBBLESTONE);
				//biome_towns.push_back(town);
				break;
			}
		}
		
	}
	
	auto desert_town = biome_towns[4];
	//find zone-connecting locations
	for(int i = 0; i < 4; i++) {
		auto btown = biome_towns[i];
		//todo: adjust this so that the connection point is more randomly placed on the biome boundaries
		auto connection_pt = find_zone_connection_point(map, biomes[4].terrain_center, {btown->x, btown->y});

		//draw_road_between_points(map, biomes[4].terrain_center, biomes[i].terrain_center);
		//draw_road_between_objects(map, biome_towns[4], biome_towns[i], ROAD_COBBLESTONE);
		draw_road_between_object_and_point(map, btown, connection_pt, ROAD_COBBLESTONE);
		draw_road_between_object_and_point(map, desert_town, connection_pt, ROAD_COBBLESTONE);

		auto monster = (map_monster_t*)interactable_object_t::make_new_object(OBJECT_MAP_MONSTER);
		
		std::bitset<8> tiers = 0;
		tiers.set(5);
		tiers.set(6);
		auto minfo = get_monster_guard(45000, tiers);
		monster->unit_type = minfo.first;
		monster->quantity = minfo.second;
		//monster->asset_id = 274;
		monster->x = connection_pt.x;
		monster->y = connection_pt.y;
		add_object_to_map(monster, map);
		
		//map.get_tile(connection_pt.x, connection_pt.y).terrain_type = TERRAIN_WATER;
	}
	
	for (int y = 0; y < map.height; ++y) {
		for (int x = 0; x < map.width; ++x) {
			auto& tile = map.get_tile(x, y);

			int dx[] = { 1, 0, -1, 0, 1, 1, -1, -1 };
			int dy[] = { 0, 1, 0, -1, 1, -1, 1, -1 };

			coord_t selected_tile = {-1, -1};
			bool is_edge = false;

			for (int i = 0; i < 8; i++) {
				int nx = x + dx[i];
				int ny = y + dy[i];
				if(!map.tile_valid(nx, ny))
					continue;

				auto& neighbor = map.get_tile(nx, ny);
				if(neighbor.zone_id != tile.zone_id) {
				//if(neighbor.terrain_type != tile.terrain_type) {
					is_edge = true;
					break;
				}
			}

			if(is_edge && tile.road_type == ROAD_NONE && tile.passability == 1) {
				auto brush = get_tree_brush_for_tile(x, y, map.get_tile(x, y));
				bool is_near_diagonal_road = is_road_diagonal(x, y, map);
				if(is_near_diagonal_road)
					continue;

				obstacle_tiles.setBit(x + y*map.width);
			}
		}
	}
	
	float ofx = fx * 10;
	float ofy = fy * 10;

	for (int y = 0; y < map.height; ++y) {
 		for (int x = 0; x < map.width; ++x) {
 			map_tile_t& tile = map.get_tile(x, y);

 			// Generate a noise value for this tile
 			auto noise = perlin.octave2D_01(x * ofx, y * ofy, 1);
			if(noise >= 0.6 && tile.passability == 1 && tile.road_type == ROAD_NONE) {
				//don't put obstacles too close to diagonal roads
				bool is_near_diagonal_road = is_road_diagonal(x, y, map);
				if(is_near_diagonal_road)
					continue;

				obstacle_tiles.setBit(x + y * map.width);
			}
 		}
 	}

	for(auto& b : biomes) {
		int total_biome_value = 0;
		for(auto& o : b.placeable_objects) {
			int placed_objects = 0;
			for(int i = 0; i < o.max_spawned; i++) {
				for(int n = 0; n < 50; n++) {
					auto pos = get_rand_coord_in_biome(map, b);
					if(!does_object_fit_at_location(map, o.interactable_object_type, pos, o.guarded))
						continue;

					auto obj = interactable_object_t::make_new_object(o.interactable_object_type);
					obj->x = pos.x;
					obj->y = pos.y;

					auto modified_value = o.value;
					if(o.interactable_object_type == OBJECT_ARTIFACT) {
						int rarity = 1 + (rand_int() % 4);
						auto art = ((map_artifact_t*)obj);
						art->artifact_id = adventure_map_t::get_random_artifact_of_rarity((artifact_rarity_e)rarity, rng);
						art->asset_id = game_config::get_artifact(art->artifact_id).asset_id;
						modified_value *= rarity;
					}
					else if(o.interactable_object_type == OBJECT_PANDORAS_BOX) {
						auto box = (pandoras_box_t*)obj;
						int type = rand_int() % 2;
						pandoras_box_t::reward_t reward;
						
						if(type == 0) {
							reward.type = pandoras_box_t::PANDORAS_BOX_REWARD_CREATURES;
							reward.subtype = UNIT_PIXIE;
							reward.magnitude = 20;
						}
						else if(type == 1) {
							reward.type = pandoras_box_t::PANDORAS_BOX_REWARD_EXPERIENCE;
							reward.magnitude = 5000 + ((rand_int() % 5) * 2500);
							modified_value += (reward.magnitude / 500);
						}

						box->rewards.push_back(reward);
					}

					add_object_to_map(obj, map, obstacle_tiles);

					if(o.guarded) {
						coord_t guard_pos = {pos.x, pos.y + 1};
						if(interactable_object_t::is_pickupable(o.interactable_object_type)) {
							//look at left/right/top/bottom for already existing blockers
							bool is_left_blocked = true;
							constexpr std::array<adventure_map_direction_e, 4> directions = {DIRECTION_NORTH, DIRECTION_SOUTH, DIRECTION_EAST, DIRECTION_WEST};
							int max_count = 0;
							int best_direction_index = 0;
							for(int c = 0; c < directions.size(); c++) {
								auto bc = side_block_count(map, obstacle_tiles, pos.x, pos.y, directions[c]);
								if(bc > max_count) {
									best_direction_index = c;
									max_count = bc;
								}
							}
							adventure_map_direction_e best_direction = directions[best_direction_index];
							const std::array<coord_t, 4> offsets = { { {0, -1}, {0, 1}, {1, 0}, {-1, 0} } };
							place_blockers(map, obstacle_tiles, pos.x, pos.y, best_direction);
							guard_pos.x = pos.x - offsets[best_direction_index].x;
							guard_pos.y = pos.y - offsets[best_direction_index].y;
						}

						auto monster = (map_monster_t*)interactable_object_t::make_new_object(OBJECT_MAP_MONSTER);

						auto minfo = get_monster_guard(modified_value);
						monster->unit_type = minfo.first;
						monster->quantity = minfo.second;

						monster->x = guard_pos.x;
						monster->y = guard_pos.y;
						add_object_to_map(monster, map, obstacle_tiles);
					}

					break;
				}
			}
		}
	}

	//first pass, place mountains
	for (int y = 0; y < map.height; ++y) {
 		for (int x = 0; x < map.width; ++x) {
			if(!obstacle_tiles.testBit(x + y * map.width))
				continue;

			int dx[] = {-1, 0, 1};
			auto noise = perlin.octave2D_01(x * ofx / 5., y * ofy / 5., 1);
			bool place_mountain_here = utils::rand_chance(90 * noise);
			if(!place_mountain_here)
				continue;

			std::vector<const mountain_brush_t*> candidate_brushes;
			for(const auto& b : game_config::get_mountain_brushes()) {
				if(b.native_terrain_types.test(map.get_tile(x, y).terrain_type)) {
					candidate_brushes.push_back(&b);
				}
			}
			std::shuffle(candidate_brushes.begin(), candidate_brushes.end(), rng);

			std::vector<const mountain_brush_t*> placeable_brushes;
			for(const auto& cb : candidate_brushes) {
				bool brush_valid = true;
				for(auto my = 0; my < 8; my++) {
					for(auto mx = 0; mx < 8; mx++) {
						auto offset = my*8 + mx;
						auto tilex = x - 3 + mx;
						auto tiley = y - 6 + my;
					
						int tile_offset = tilex + tiley * map.width;
						if(tile_offset < 0 || tile_offset >= obstacle_tiles.size() || cb->passability.test(offset))
							continue;

						if(!obstacle_tiles.testBit(tile_offset)) {
							brush_valid = false;
							break;
						}
					}
				}

				if(brush_valid)
					placeable_brushes.push_back(cb);
			}

			if(!placeable_brushes.size())
				continue;

			const auto brush = utils::rand_item(placeable_brushes, rng);

			for(const auto& b : brush->mountain_placement_info) {
				doodad_t d;
				d.asset_id = 100 + b.mountain_type;
				d.x = (TILE_SIZE * x) + b.x;
				d.y = (TILE_SIZE * y) + b.y;
				map.doodads.push_back(d);
			}

			for(auto my = 0; my < 8; my++) {
				for(auto mx = 0; mx < 8; mx++) {
					auto offset = my*8 + mx;
					auto tilex = x - 3 + mx;
					auto tiley = y - 6 + my;
					
					if(brush->passability.test(offset) || !map.tile_valid(tilex, tiley))
						continue;

					int tile_offset = tilex + tiley * map.width;
					map.get_tile(tilex, tiley).passability = 0;
					obstacle_tiles.clearBit(tile_offset);
				}
			}
		}
	}

	//second pass, place trees and smaller obstacles
	for (int y = 0; y < map.height; ++y) {
 		for (int x = 0; x < map.width; ++x) {
			if(!obstacle_tiles.testBit(x + y * map.width))
				continue;

 			map_tile_t& tile = map.get_tile(x, y);
			auto brush = get_tree_brush_for_tile(x, y, tile);
			if(brush) {
				for(const auto& b : brush->tree_placement_info) {
					doodad_t d;
					d.asset_id = b.tree_type;
					d.x = (TILE_SIZE * x) + b.x;
					d.y = (TILE_SIZE * y) + b.y;
					map.doodads.push_back(d);
				}

				tile.passability = 0;
			}
		}
	}

	//add resources
	for(const auto& b : biomes) {
		int count = b.resource_frequency * 20 * (map.width / 100.f);
		for(int i = 0; i < count; i++) {
			auto pos = get_rand_coord_in_biome(map, b);
			if(!map.tile_valid(pos.x, pos.y) || !map.get_tile(pos.x, pos.y).is_passable())
				continue;
			
			auto res = (map_resource_t*)interactable_object_t::make_new_object(OBJECT_RESOURCE);
			res->x = pos.x;
			res->y = pos.y;
			res->resource_type = get_random_resource(0xFF, rng);
			res->min_quantity = utils::rand_range(3, 8, rng);
			if(res->resource_type == RESOURCE_GOLD)
				res->min_quantity *= 100;
			else if(res->resource_type == RESOURCE_WOOD || res->resource_type == RESOURCE_ORE)
				res->min_quantity *= 2;

			res->max_quantity = res->min_quantity;
			add_object_to_map(res, map);
		}
	}

	//add treasure chests
	for(const auto& b : biomes) {
		int count = 15 * (map.width / 100.f);
		for(int i = 0; i < count; i++) {
			auto pos = get_rand_coord_in_biome(map, b);
			if(!map.tile_valid(pos.x, pos.y) || !map.get_tile(pos.x, pos.y).is_passable())
				continue;
			
			auto chest = (treasure_chest_t*)interactable_object_t::make_new_object(OBJECT_TREASURE_CHEST);
			chest->x = pos.x;
			chest->y = pos.y;
			add_object_to_map(chest, map);
		}
	}

	//add campfires
	/*for(auto town : biome_towns) {
		int count = 8 * (map.width / 100.f);
		for(int i = 0; i < count; i++) {
			auto tx = town->x;
			auto ty = town->y;

			auto pos = get_rand_coord_in_range(tx, ty, 5 + i);
			if(!map.tile_valid(pos.x, pos.y) || !map.get_tile(pos.x, pos.y).is_passable())
				continue;
			
			auto campfire = (campfire_t*)interactable_object_t::make_new_object(OBJECT_CAMPFIRE);
			campfire->x = pos.x;
			campfire->y = pos.y;
			add_object_to_map(campfire, map);
		}
	}*/
	
	int i = 0;
	for(const auto& b : biomes) {
		if(b.player == PLAYER_NONE)
			continue;


		auto& pc = map.player_configurations[b.player - 1];
		pc.team = i;
		pc.allowed_player_type = PLAYER_TYPE_HUMAN_OR_COMPUTER;
		pc.player_number = (player_e)(i + 1);
		pc.color = (player_color_e)i;

		i++;
	}
	map.players = i;

	return RMG_RESULT_OK;
}
