#include "rmg.h"

#include "../../editor/src/perlin_noise.h"

#include <queue>

//const int TILE_SIZE = 128;
asset_id_t REPLACE_ME_get_tile_from_type(terrain_type_e terrain) {
	switch(terrain) {
		case TERRAIN_WATER: return 33;
		case TERRAIN_GRASS: return 15;
		case TERRAIN_DIRT: return 5;
		case TERRAIN_WASTELAND: return 7;
		case TERRAIN_LAVA: return 264;
		case TERRAIN_DESERT: return 24;
		case TERRAIN_BEACH: return 2;
		case TERRAIN_SNOW: return 272;
		case TERRAIN_SWAMP: return 29;
			
		default: return 0;
	}
}

void draw_road_between_object_and_point(adventure_map_t& map, interactable_object_t* obj_start, coord_t end);
void draw_road_between_objects(adventure_map_t& map, interactable_object_t* obj_start, interactable_object_t* obj_end);
void draw_road_between_points(adventure_map_t& map, coord_t start, coord_t end);


coord_t get_rand_coord_in_range(int x, int y, int dist, int range = 0) {
	int total_dist = dist + utils::rand_range(-range, range);
	int xoff = utils::rand_range(-total_dist, total_dist);
	int yoff = (xoff >= 0) ? total_dist - xoff : total_dist + xoff;
	
	if(rand() % 2 == 0)
		yoff = -yoff;
	
	return {x + xoff, y + yoff};
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



void draw_road_between_objects(adventure_map_t& map, interactable_object_t* obj_start, interactable_object_t* obj_end) {
	auto start_tile = map.get_interactable_coordinate_for_object(obj_start);
	auto mi = map.get_interactable_coordinate_for_object(obj_end);
	map.get_tile(mi.x, mi.y).road_type = ROAD_COBBLESTONE; map.get_tile(mi.x, mi.y).terrain_type = TERRAIN_WATER;
	map.get_tile(mi.x, mi.y).asset_id = REPLACE_ME_get_tile_from_type(TERRAIN_WATER);
	mi.y++;
	map.get_tile(mi.x, mi.y).road_type = ROAD_COBBLESTONE; map.get_tile(mi.x, mi.y).terrain_type = TERRAIN_WATER;
	map.get_tile(mi.x, mi.y).asset_id = REPLACE_ME_get_tile_from_type(TERRAIN_WATER);
	int ew_direction = (mi.x <= start_tile.x) ? 1 : -1;

	mi.x += ew_direction;
	map.get_tile(mi.x, mi.y).road_type = ROAD_COBBLESTONE; map.get_tile(mi.x, mi.y).terrain_type = TERRAIN_WATER;
	map.get_tile(mi.x, mi.y).asset_id = REPLACE_ME_get_tile_from_type(TERRAIN_WATER);
	mi.x += ew_direction;
	map.get_tile(mi.x, mi.y).road_type = ROAD_COBBLESTONE; map.get_tile(mi.x, mi.y).terrain_type = TERRAIN_WATER;
	map.get_tile(mi.x, mi.y).asset_id = REPLACE_ME_get_tile_from_type(TERRAIN_WATER);
	
	draw_road_between_object_and_point(map, obj_start, mi);
}

void draw_road_between_object_and_point(adventure_map_t& map, interactable_object_t* obj_start, coord_t end) {
	auto mi = map.get_interactable_coordinate_for_object(obj_start);
	map.get_tile(mi.x, mi.y).road_type = ROAD_COBBLESTONE; map.get_tile(mi.x, mi.y).terrain_type = TERRAIN_WATER;
	map.get_tile(mi.x, mi.y).asset_id = REPLACE_ME_get_tile_from_type(TERRAIN_WATER);
	mi.y++;
	map.get_tile(mi.x, mi.y).road_type = ROAD_COBBLESTONE; map.get_tile(mi.x, mi.y).terrain_type = TERRAIN_WATER;
	map.get_tile(mi.x, mi.y).asset_id = REPLACE_ME_get_tile_from_type(TERRAIN_WATER);
	int ew_direction = (mi.x <= end.x) ? 1 : -1;

	mi.x += ew_direction;
	map.get_tile(mi.x, mi.y).road_type = ROAD_COBBLESTONE; map.get_tile(mi.x, mi.y).terrain_type = TERRAIN_WATER;
	map.get_tile(mi.x, mi.y).asset_id = REPLACE_ME_get_tile_from_type(TERRAIN_WATER);
	mi.x += ew_direction;
	map.get_tile(mi.x, mi.y).road_type = ROAD_COBBLESTONE; map.get_tile(mi.x, mi.y).terrain_type = TERRAIN_WATER;
	map.get_tile(mi.x, mi.y).asset_id = REPLACE_ME_get_tile_from_type(TERRAIN_WATER);
	
	draw_road_between_points(map, mi, end);
}

void draw_road_between_points(adventure_map_t& map, coord_t start, coord_t end) {
	// Calculate the direction vector from start to end
	int dx = end.x - start.x;
	int dy = end.y - start.y;

	// Determine a perpendicular vector (dy, -dx) or (-dy, dx)
	int perpendicular_x = dy;
	int perpendicular_y = -dx;

	// Randomize the choice of perpendicular direction
	if (rand() % 2) {
		perpendicular_x = -dy;
		perpendicular_y = dx;
	}
	
	// Compute the magnitude of the deviation (can be tweaked)
	float min_deviation = -.5f;
	float max_deviation = .5f;
	float deviation_magnitude = utils::rand_rangef(min_deviation, max_deviation);
	float deviation_delta = .1f;
	float deviation_direction = (rand() % 2 == 0) ? 1.f : -1.f;

	coord_t virtual_end;
	virtual_end.x = (int)(end.x + perpendicular_x * deviation_magnitude);
	virtual_end.y = (int)(end.y + perpendicular_y * deviation_magnitude);
	
	int sx = start.x < virtual_end.x ? 1 : -1;
	int sy = start.y < virtual_end.y ? 1 : -1;
	int err = std::abs(virtual_end.x - start.x) - std::abs(virtual_end.y - start.y);

	coord_t pos = start;
	int e2;

	while(true) {
		if(map.tile_valid(pos.x, pos.y)) {
			map.get_tile(pos.x, pos.y).road_type = ROAD_COBBLESTONE;
			map.get_tile(pos.x, pos.y).terrain_type = TERRAIN_WATER;
			map.get_tile(pos.x, pos.y).asset_id = REPLACE_ME_get_tile_from_type(TERRAIN_WATER);
		}
		
		if(pos.x == end.x && pos.y == end.y)
			break;
		
		// Recalculate the virtual end to gradually adjust to actual end
		if(utils::mh_dist(pos.x, pos.y, end.x, end.y) > 15) {
			deviation_magnitude += utils::rand_rangef(0.f, deviation_delta) * deviation_direction;
			
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









void draw_road_between_points2(adventure_map_t& map, coord_t start, coord_t end) {
	int dx = std::abs(end.x - start.x);
	int sx = start.x < end.x ? 1 : -1;
	
	int dy = -std::abs(end.y - start.y);
	int sy = start.y < end.y ? 1 : -1;
	
	int err = dx + dy;
	int e2;

	coord_t pos = start;

	while(true) {
		map.get_tile(pos.x, pos.y).road_type = ROAD_COBBLESTONE;
		map.get_tile(pos.x, pos.y).terrain_type = TERRAIN_WATER;
		map.get_tile(pos.x, pos.y).asset_id = REPLACE_ME_get_tile_from_type(TERRAIN_WATER);
		
		if(pos.x == end.x && pos.y == end.y)
			break;
		
		e2 = 2 * err;
		
//		//random bend factor
//		if(utils::rand_chance(10)) {
//			int bendDirection = rand() % 2;  // Choose between two types of deviations
//			if(bendDirection == 0 && sx != 0) {
//				pos.y += sy;  // Deviate vertically
//			} else if (bendDirection == 1 && sy != 0) {
//				pos.x += sx;  // Deviate horizontally
//			}
//			continue;
//		}
//		
		if(e2 >= dy) {
			if(pos.x == end.x)
				break;
			
			err += dy;
			pos.x += sx;
		}
		if(e2 <= dx) {
			if(pos.y == end.y)
				break;
		
			err += dx;
			pos.y += sy;
		}
		
		// Introduce randomness
//		if(rand() % 10 < 2) { // Approximately 20% chance to deviate
//			if(rand() % 2) {
//				pos.x += (rand() % 2) ? 1 : -1; // Randomly adjust x by ±1
//			} else {
//				pos.y += (rand() % 2) ? 1 : -1; // Randomly adjust y by ±1
//			}
//		}
		
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

rmg_result_e generate_random_map(adventure_map_t& map, std::vector<biome_t>& biomes) {
	const siv::PerlinNoise::seed_type seed = std::random_device()();
	const siv::PerlinNoise perlin{seed};

	const float frequency = 4.0;
	const double fx = (frequency / map.width);
	const double fy = (frequency / map.height);

	map.clear(true, false);
	
	for(auto& b : biomes) {
		if(b.player != PLAYER_NONE) {
			b.faction = (hero_class_e)(rand() % 6);
			b.terrain_type = get_faction_native_terrain(b.faction);
		}
		else {
			if(b.terrain_type == TERRAIN_UNKNOWN)
				b.terrain_type = (terrain_type_e)(2 + rand() % 8);
		}
	}
	
	
	for (int y = 0; y < map.height; ++y) {
		for (int x = 0; x < map.width; ++x) {
			map_tile_t& tile = map.get_tile(x, y);

//			// Generate a noise value for this tile
//			auto noise = perlin.octave2D_01(x * fx, y * fy, 1);
//			int type = (noise * 10);
			int dist = INT_MAX;
			int i = 2;
			terrain_type_e nearest = TERRAIN_WATER;
			int nearest_zone_id = -1;
			
			for(auto& b : biomes) {
				float scale = b.scale;
				
				float d = utils::eu_dist(x, y, b.terrain_center.x, b.terrain_center.y);
				float dr = 4000.f;
				auto df = (dr/2.f) - (perlin.octave2D_01(x * fx * i, y * fy * i, 1) * dr);
				d += df;
				if(d < (dist * scale)) {
					//if(utils::rand_chance(80))
					//if()
					nearest = b.terrain_type;
					//nearest_zone_id = b.zone_id;
					nearest_zone_id = 100 + i;
					//else
					//	nearest = tile.terrain_type;
					
					dist = d;
				}
				if(x == b.terrain_center.x && y == b.terrain_center.y)
					nearest = TERRAIN_WATER;
				
				i++;
			}
			
			tile.terrain_type = nearest;//(terrain_type_e)(1 + type % 8);
			tile.asset_id = REPLACE_ME_get_tile_from_type(tile.terrain_type);
			tile.zone_id = nearest_zone_id;
			//clear any existing roads
			tile.road_type = ROAD_NONE;
		}
	}
	std::vector<town_t*> biome_towns;
	//add towns to each biome, if applicable
	for(auto& b : biomes) {
		auto town = (town_t*)interactable_object_t::make_new_object(OBJECT_MAP_TOWN);
		town->x = b.terrain_center.x;
		town->y = b.terrain_center.y;
		
		auto tx = town->x;
		auto ty = town->y;
		
		auto faction = (b.player != PLAYER_NONE ? b.faction : get_faction_from_native_terrain_type(b.terrain_type));
		town->town_type = town_t::hero_class_to_town_type(faction);
		
		town->player = b.player;
		
		map.objects.push_back(town);
		biome_towns.push_back(town);
		
		//add mines
		for(int n = 0; n < 2; n++) {
			for(int i = 0; i < 10; i++) {
				auto pos = get_rand_coord_in_range(tx, ty, 15 - i, 5);
				
				if(!map.tile_valid(pos.x, pos.y) || !map.get_tile(pos.x, pos.y).is_passable())
					continue;
				
				//todo: set maximum y value, given that it cannot be accessed when
				//at the very bottom of the map
				
				auto mine = (mine_t*)interactable_object_t::make_new_object(OBJECT_MINE);
				mine->x = pos.x;
				mine->y = pos.y;
				//fixme
				mine->asset_id = 1331;
				mine->width = 4;
				mine->height = 3;
				
				if(n == 0)
					mine->mine_type = RESOURCE_WOOD;
				else if(n == 1)
					mine->mine_type = RESOURCE_ORE;
				//else
				
				map.objects.push_back(mine);
				
				//put road between town and mines
				draw_road_between_objects(map, mine, town);
				break;
			}
		}
		
		
		
		//add resources
//		for(int i = 0; i < 50; i++) {
//			auto pos = get_rand_coord_in_range(tx, ty, 5 + i);
//			if(!map.tile_valid(pos.x, pos.y) || !map.get_tile(pos.x, pos.y).is_passable())
//				continue;
//			
//			auto res = (map_resource_t*)interactable_object_t::make_new_object(OBJECT_RESOURCE);
//			res->x = pos.x;
//			res->y = pos.y;
//			map.get_tile(pos.x, pos.y).passability = 0;
//			//res->type =
//			map.objects.push_back(res);
//		}
		
	}
	
	
	
	for(auto& b : biomes)
		flood_fill_biome(map, b);
	
	
//	for (int y = 0; y < map.height; ++y) {
//		for (int x = 0; x < map.width; ++x) {
//			auto& tile = map.get_tile(x, y);
//
//			for(auto& b : biomes) {
//				if(tile.zone_id == b.zone_id)
//					tile.terrain_type = TERRAIN_WATER;
//			}
//		}
//	}
	
	//draw_road_between_points(map, biomes[4].terrain_center, biomes[2].terrain_center);
	
	//find zone-connecting locations
	for(int i = 0; i < 4; i++) {
		//draw_road_between_points(map, biomes[4].terrain_center, biomes[i].terrain_center);
		draw_road_between_objects(map, biome_towns[4], biome_towns[i]);
		
		auto connection_pt = find_zone_connection_point(map, biomes[4].terrain_center, biomes[i].terrain_center);
		
		auto monster = (map_monster_t*)interactable_object_t::make_new_object(OBJECT_MAP_MONSTER);
		
		monster->unit_type = UNIT_TROLL; //UNIT_RED_DRAGON;
		monster->quantity = 5;
		monster->asset_id = 274;
		monster->x = connection_pt.x;
		monster->y = connection_pt.y;
		map.get_tile(connection_pt.x, connection_pt.y).passability = 2;
		map.objects.push_back(monster);
		
		//map.get_tile(connection_pt.x, connection_pt.y).terrain_type = TERRAIN_WATER;
	}
	
//	for (int y = 0; y < map.height; ++y) {
//		for (int x = 0; x < map.width; ++x) {
//			auto& tile = map.get_tile(x, y);
//
//			int dx[] = { 1, 0, -1, 0, 1, 1, -1, -1 };
//			int dy[] = { 0, 1, 0, -1, 1, -1, 1, -1 };
//
//			coord_t selected_tile = {-1, -1};
//			bool is_edge = false;
//
//			for (int i = 0; i < 8; i++) {
//				int nx = x + dx[i];
//				int ny = y + dy[i];
//				if(!map.tile_valid(nx, ny))
//					continue;
//
//				auto& neighbor = map.get_tile(nx, ny);
//				if(neighbor.zone_id != tile.zone_id) {
//				//if(neighbor.terrain_type != tile.terrain_type) {
//					is_edge = true;
//					break;
//				}
//			}
//
//			if(is_edge) {
//				doodad_t d;
//				if(tile.terrain_type == TERRAIN_DESERT)
//					d.asset_id = 300 + (rand() % 7);
//				else if(tile.terrain_type == TERRAIN_SNOW)
//					d.asset_id = 307 + (rand() % 10);
//				else
//					d.asset_id = 348;
//
//				int tr = 80;
//				d.x = (TILE_SIZE * x) + utils::rand_range(-tr, tr);
//				d.y = (TILE_SIZE * y) + utils::rand_range(-tr, tr);
////				d.width = 2;
////				d.height = 2;
//
//				map.doodads.push_back(d);
//			}
//		}
//	}
	
//	for (int y = 0; y < map.height; ++y) {
// 		for (int x = 0; x < map.width; ++x) {
// 			map_tile_t& tile = map.get_tile(x, y);
//
// 			// Generate a noise value for this tile
// 			auto noise = perlin.octave2D_01(x * fx, y * fy, 1);
// 			int type = (noise * 4);
// 			tile.terrain_type = (terrain_type_e)(2 + type % 4);
// 			tile.asset_id = get_tile_from_type(tile.terrain_type).asset_id;
// 		}
// 	}
	
	return RMG_RESULT_OK;
}
