#include "core/adventure_map.h"
#include "core/game.h"

#include <queue>
#include <map>
#include <cmath>
#include <set>

#include "core/qt_headers.h"

std::map<int, std::bitset<64>> adventure_map_t::interactivity_map;

QDataStream& operator<<(QDataStream& stream, const doodad_t& doodad) {
	stream << doodad.asset_id << doodad.x << doodad.y << doodad.z << doodad.width << doodad.height;
	return stream;
}

QDataStream& operator>>(QDataStream& stream, doodad_t& doodad) {
	stream >> doodad.asset_id >> doodad.x >> doodad.y >> doodad.z >> doodad.width >> doodad.height;
	return stream;
}

QDataStream& operator<<(QDataStream& stream, const map_tile_t& tile) {
	stream << tile.asset_id;
	stream << tile.terrain_type;
	stream << tile.passability;
	stream << tile.interactable_object;
	stream << tile.road_type;
	stream << tile.zone_id;
	
	return stream;
}

QDataStream& operator>>(QDataStream& stream, map_tile_t& tile) {
	stream >> tile.asset_id;
	stream >> tile.terrain_type;
	stream >> tile.passability;
	stream >> tile.interactable_object;
	stream >> tile.road_type;
	stream >> tile.zone_id;
	
	return stream;
}

QDataStream& operator<<(QDataStream& stream, const troop_t& troop) {
	stream << troop.unit_type << troop.stack_size;
	return stream;
}

QDataStream& operator>>(QDataStream& stream, troop_t& troop){
	stream >> troop.unit_type >> troop.stack_size;
	return stream;
}

QDataStream& operator<<(QDataStream& stream, const buff_t& buff) {
	stream << buff.buff_id;
	stream << buff.magnitude;
	stream << buff.duration;
	
	return stream;
}

QDataStream& operator>>(QDataStream& stream, buff_t& buff) {
	stream >> buff.buff_id;
	stream >> buff.magnitude;
	stream >> buff.duration;
	
	return stream;
}

//
//bool operator==(const adventure_map_t& lhs, const adventure_map_t& rhs) {
//	if (lhs.width != rhs.width) {
//		qDebug() << "Width mismatch:" << lhs.width << "!=" << rhs.width;
//		return false;
//	}
//	if (lhs.height != rhs.height) {
//		qDebug() << "Height mismatch:" << lhs.height << "!=" << rhs.height;
//		return false;
//	}
//	if (lhs.name != rhs.name) {
//		qDebug() << "Name mismatch:" << QString::fromStdString(lhs.name) << "!=" << QString::fromStdString(rhs.name);
//		return false;
//	}
//	if (lhs.description != rhs.description) {
//		qDebug() << "Description mismatch:" << QString::fromStdString(lhs.description) << "!=" << QString::fromStdString(rhs.description);
//		return false;
//	}
//	if (lhs.players != rhs.players) {
//		qDebug() << "Players mismatch:" << lhs.players << "!=" << rhs.players;
//		return false;
//	}
//	if (lhs.difficulty != rhs.difficulty) {
//		qDebug() << "Difficulty mismatch:" << lhs.difficulty << "!=" << rhs.difficulty;
//		return false;
//	}
//	if (lhs.win_condition != rhs.win_condition) {
//		qDebug() << "Win condition mismatch:" << lhs.win_condition << "!=" << rhs.win_condition;
//		return false;
//	}
//	if (lhs.loss_condition != rhs.loss_condition) {
//		qDebug() << "Loss condition mismatch:" << lhs.loss_condition << "!=" << rhs.loss_condition;
//		return false;
//	}
//	if (lhs.passability_map != rhs.passability_map) {
//		qDebug() << "Passability map mismatch";
//		return false;
//	}
//	if (lhs.tiles != rhs.tiles) {
//		qDebug() << "Tiles mismatch";
//		return false;
//	}
//	if (lhs.doodads != rhs.doodads) {
//		qDebug() << "Doodads mismatch";
//		return false;
//	}
//	if (lhs.heroes != rhs.heroes) {
//		qDebug() << "Heroes mismatch";
//		return false;
//	}
//	
//	if (lhs.player_configurations != rhs.player_configurations) {
//		qDebug() << "Player configurations mismatch";
//		return false;
//	}
//	
//	if (lhs.objects.size() != rhs.objects.size()) {
//		qDebug() << "Objects size mismatch:" << lhs.objects.size() << "!=" << rhs.objects.size();
//		return false;
//	}
//	for (size_t i = 0; i < lhs.objects.size(); ++i) {
//		if (!lhs.objects[i] && rhs.objects[i]) {
//			qDebug() << "Object mismatch at index" << i << ": lhs object is null, rhs object is not null";
//			return false;
//		}
//		if (lhs.objects[i] && !rhs.objects[i]) {
//			qDebug() << "Object mismatch at index" << i << ": lhs object is not null, rhs object is null";
//			return false;
//		}
//		if (lhs.objects[i] && rhs.objects[i]) {
//			if(lhs.objects[i]->type != rhs.objects[i]->type ||
//			   lhs.objects[i]->asset_id != rhs.objects[i]->asset_id ||
//			   lhs.objects[i]->x != rhs.objects[i]->x ||
//			   lhs.objects[i]->y != rhs.objects[i]->y ||
//			   lhs.objects[i]->z != rhs.objects[i]->z ||
//			   lhs.objects[i]->width != rhs.objects[i]->width ||
//			   lhs.objects[i]->height != rhs.objects[i]->height ) {
//				qDebug() << "Object mismatch at index" << i;
//				return false;
//			}
//		}
//	}
//	
//	return true;
//}


static const char* terrain_names[] = { "Unknown", "Water", "Grass", "Dirt", "Wasteland", "Lava", "Desert", "Beach", "Snow", "Swamp" };
const std::string map_tile_t::get_name() const {
	//if(interactable_object)
	//	return interactable_object->get_name(interactable_object->type);
	//fixme - magic_enum
	return QObject::tr(terrain_names[terrain_type]).toStdString();
}

terrain_type_e get_faction_native_terrain(hero_class_e faction) {
	switch(faction) {
		case HERO_KNIGHT: return TERRAIN_DIRT;
		case HERO_BARBARIAN: return TERRAIN_WASTELAND;
		case HERO_NECROMANCER: return TERRAIN_SWAMP;
		case HERO_SORCERESS: return TERRAIN_GRASS;
		case HERO_WARLOCK: return TERRAIN_LAVA;
		case HERO_WIZARD: return TERRAIN_SNOW;
	}
	
	return TERRAIN_UNKNOWN;
}

hero_class_e get_faction_from_native_terrain_type(terrain_type_e terrain_type) {
	switch(terrain_type) {
		case TERRAIN_DIRT: return HERO_KNIGHT;
		case TERRAIN_WASTELAND: return HERO_BARBARIAN;
		case TERRAIN_SWAMP: return HERO_NECROMANCER;
		case TERRAIN_GRASS: return HERO_SORCERESS;
		case TERRAIN_LAVA: return HERO_WARLOCK;
		case TERRAIN_SNOW: return HERO_WIZARD;
		
		default: return HERO_CLASS_NONE;
	}
}

QColor get_qcolor_for_player_color(player_color_e pcolor) {
	switch (pcolor) {
		case PLAYER_COLOR_BLUE: return QColor(4, 51, 255);
		case PLAYER_COLOR_RED: return QColor(148, 17, 0);
		case PLAYER_COLOR_GREEN: return QColor(0, 143, 0);
		case PLAYER_COLOR_ORANGE: return QColor(255, 147, 0);
		case PLAYER_COLOR_TEAL: return QColor(115, 252, 214);
		case PLAYER_COLOR_PINK: return QColor(255, 85, 194);
		case PLAYER_COLOR_PURPLE: return QColor(83, 27, 147);
		case PLAYER_COLOR_YELLOW: return QColor(255, 251, 0);
		case PLAYER_COLOR_NEUTRAL: return QColor(180, 180, 180);
	}
	
	return QColor(255, 255, 255);
}

std::string get_color_string_for_player_color(player_color_e pcolor) {
	return get_qcolor_for_player_color(pcolor).name(QColor::NameFormat::HexArgb).remove(0,1).toStdString();
}

std::string get_color_name_for_player_color(player_color_e pcolor) {
	switch (pcolor) {
		case PLAYER_COLOR_BLUE: return QObject::tr("Blue").toStdString();
		case PLAYER_COLOR_RED: return QObject::tr("Red").toStdString();
		case PLAYER_COLOR_GREEN: return QObject::tr("Green").toStdString();
		case PLAYER_COLOR_ORANGE: return QObject::tr("Orange").toStdString();
		case PLAYER_COLOR_TEAL: return QObject::tr("Teal").toStdString();
		case PLAYER_COLOR_PINK: return QObject::tr("Pink").toStdString();
		case PLAYER_COLOR_PURPLE: return QObject::tr("Purple").toStdString();
		case PLAYER_COLOR_YELLOW: return QObject::tr("Yellow").toStdString();
		case PLAYER_COLOR_NEUTRAL: return QObject::tr("Neutral").toStdString();
	}
	
	return QObject::tr("Unknown").toStdString();
}

std::array<int, game_config::CREATURE_TIERS> base_growth_by_tier = {15, 8, 5, 3, 2, 1};

void adventure_map_t::populate_refugee_camp(refugee_camp_t* camp) {
	unit_type_e selected_creature = (unit_type_e)utils::rand_range((int)UNIT_SKELETON, (int)UNIT_TITAN);
	camp->available_troops.unit_type = selected_creature;
	auto& cr = game_config::get_creature(selected_creature);
	//set a default size
	uint tier = cr.tier - 1;
	assert(tier < base_growth_by_tier.size());
	int growth = base_growth_by_tier[tier];
	for(auto& b : game_config::get_buildings()) {
		if(b.generated_creature == selected_creature) {
			growth = b.weekly_growth;
			break;
		}
	}
	
	camp->available_troops.stack_size = growth;
}

adventure_map_t::adventure_map_t(uint width, uint height) : width(width), height(height) {
	tiles.resize(width * height);
}

bool adventure_map_t::tile_valid(int x, int y) const {
	return !(x < 0 || x >= width || y < 0 || y >= height);
}

#include <random>

uint32_t get_morton_code(uint16_t x, uint16_t y) {
	uint32_t code = 0;
	for(int bit = 0; bit < 16; bit++) {
		uint32_t xb = (uint32_t)((x >> bit) & 1);
		uint32_t yb = (uint32_t)((y >> bit) & 1);
		code |= xb << (2 * bit);
		code |= yb << (2 * bit + 1);
	}

	return code;
}

int adventure_map_t::initialize_map(const game_configuration_t& game_config, int seed) {
	srand(seed);
	
	std::map<artifact_rarity_e, std::vector<artifact_e>> artifacts_by_rarity;
	for(const auto& art : game_config::get_artifacts())
		artifacts_by_rarity[art.rarity].push_back(art.id);
	
	for(auto& artifacts : artifacts_by_rarity)
		std::shuffle(artifacts.second.begin(), artifacts.second.end(), std::mt19937_64(seed));

	std::map<artifact_rarity_e, std::vector<std::pair<uint32_t, map_artifact_t*>>> map_artifacts_by_rarity;

	std::vector<spell_e> level_5_spells;
	int level_5_spells_index = 0;
	for(const auto& sp : game_config::get_spells()) {
		if(sp.level == 5)
			level_5_spells.push_back(sp.id);
	}
	std::shuffle(level_5_spells.begin(), level_5_spells.end(), std::mt19937_64(seed));

	for(auto o : objects) {
		if(!o)
			continue;
		
		if(o->object_type == OBJECT_RESOURCE) {
			auto res = (map_resource_t*)o;

			res->min_quantity = utils::rand_range(res->min_quantity, std::max(res->min_quantity, res->max_quantity));
			if(res->resource_type == RESOURCE_RANDOM) {
				res->resource_type = (resource_e)utils::rand_range((int)RESOURCE_GOLD, (int)RESOURCE_MERCURY);
				if(res->resource_type == RESOURCE_GOLD)
					res->min_quantity *= 100;
				else if(res->resource_type == RESOURCE_WOOD || res->resource_type == RESOURCE_ORE)
					res->min_quantity *= 2;
			}
		}
		else if(o->object_type == OBJECT_MINE) {
			auto mine = (mine_t*)o;
			if(mine->mine_type == RESOURCE_RANDOM) {
				mine->mine_type = (resource_e)utils::rand_range((int)RESOURCE_GOLD, (int)RESOURCE_MERCURY);
			}
		}
		else if(o->object_type == OBJECT_ARTIFACT) {
			auto artifact = (map_artifact_t*)o;

			if(artifact->artifact_id == ARTIFACT_NONE) {
				//compute morton code for artifact location
				auto morton_code = get_morton_code(artifact->x, artifact->y);
				//if artifact_rarity = RARITY_UNKNOWN, that means pick a random rarity for it
				if(artifact->rarity == RARITY_UNKNOWN)
					artifact->rarity = (artifact_rarity_e)(utils::rand_range((int)RARITY_COMMON, (int)RARITY_EXCEPTIONAL));

				map_artifacts_by_rarity[artifact->rarity].push_back({morton_code, artifact});
			}
			
		}
		else if(o->object_type == OBJECT_TREASURE_CHEST) {
			auto chest = (treasure_chest_t*)o;
			chest->value = utils::rand_range(1, 3);
		}
		else if(o->object_type == OBJECT_MAGIC_SHRINE) {
			auto shrine = (shrine_t*)o;
			if(shrine->available_spells.size()) {
				shrine->spell = shrine->available_spells[rand() % shrine->available_spells.size()];
			}
			else {  //should not be the case
				shrine->spell = game_config::get_spells()[1 + (rand() % (game_config::get_spells().size() - 1))].id;
			}
		}
		else if(o->object_type == OBJECT_WARRIORS_TOMB) {
			auto tomb = (warriors_tomb_t*)o;
			auto chance = utils::rand_range(0, 99);
			if(chance < 30)
				tomb->artifact = get_random_artifact_of_rarity(RARITY_COMMON);
			else if(chance < 80)
				tomb->artifact = get_random_artifact_of_rarity(RARITY_UNCOMMON);
			else if(chance < 95)
				tomb->artifact = get_random_artifact_of_rarity(RARITY_RARE);
			else
				tomb->artifact = get_random_artifact_of_rarity(RARITY_EXCEPTIONAL);
				
		}
		else if(o->object_type == OBJECT_REFUGEE_CAMP) {
			populate_refugee_camp((refugee_camp_t*)o);
		}
		else if(o->object_type == OBJECT_WITCH_HUT) {
			auto hut = (witch_hut_t*)o;
			if(hut->available_skills.size()) {
				hut->skill = hut->available_skills[rand() % hut->available_skills.size()];
			}
			else {  //should not be the case
				hut->skill = game_config::get_skills()[1 + (rand() % (game_config::get_skills().size() - 1))].skill_id;
			}
		}
		else if(o->object_type == OBJECT_PYRAMID) {
			auto pyramid = (pyramid_t*)o;
			if(pyramid->spell_id == SPELL_UNKNOWN) { //spell is not set, so pick a random one from available
				if(pyramid->available_spells.size()) {
					pyramid->spell_id = pyramid->available_spells[rand() % pyramid->available_spells.size()];
				}
				else { //all level 5 spells available
					pyramid->spell_id = level_5_spells[level_5_spells_index++ % level_5_spells.size()];
				}
			}
		}
		else if(o->object_type == OBJECT_WINDMILL) {
			auto mill = (flaggable_object_t*)o;
			//bool visited = 0;
			auto resource = (resource_e)(utils::rand_range(2, 6));
			auto quantity = utils::rand_range(3, 7);
			
			mill->visited |= (resource << 4);
			mill->visited |= quantity;
		}
		else if(o->object_type == OBJECT_CAMPFIRE) {
			auto campfire = (campfire_t*)o;
			if(campfire->resource_type == RESOURCE_RANDOM)
				campfire->resource_type = (resource_e)(utils::rand_range(2, 6));
			if(campfire->resource_value == 0)
				campfire->resource_value = utils::rand_range(3, 7);
			if(campfire->gold_value == 0)
				campfire->gold_value = utils::rand_range(4, 8);
		}
		else if(o->object_type == OBJECT_MAP_TOWN) {
			auto t = (town_t*)o;

			//at this point, game.setup() handles setting the player's main town to their chosen class
			//so we only need to handle towns that have a type of unknown/random
			if(t->town_type == TOWN_UNKNOWN) { //random
				t->town_type = (town_type_e)(TOWN_UNKNOWN + (rand() % 6) + 1);
				
				/*if(t->player == PLAYER_NONE || t->player == PLAYER_NEUTRAL) {
				}
				else if(t->player <= game_config.player_configs.size()) {
					const auto& config = game_config.player_configs[t->player - 1];
					t->town_type = town_t::hero_class_to_town_type((hero_class_e)config.selected_class);
					if(t->town_type == )
					t->town_type = (town_type_e)(TOWN_UNKNOWN + (rand() % 6) + 1);
				}*/
			}

			if(t->name.empty())
				t->name = game_config::get_random_town_name(t->town_type);
			
			t->setup_buildings();
		}
		else if(o->object_type == OBJECT_MAP_MONSTER) {
			//todo: make monster numbers/tiers higher for higher difficulty settings?
			auto monster = (map_monster_t*)o;
			if(monster->tier == 0)
				monster->tier = 1 + (rand() % 6);
			
			if(monster->unit_type == UNIT_UNKNOWN) { //select a random creature
				std::vector<unit_type_e> potential_unit_types;
				for(auto& cr : game_config::get_creatures()) {
					troop_t troop;
					troop.unit_type = cr.unit_type;
					if(cr.unit_type == UNIT_UNKNOWN || troop.is_turret_or_war_machine())
						continue;

					if(monster->tier == 0 || cr.tier == monster->tier) {
						if(monster->ranged_melee == 0) //any
							potential_unit_types.push_back(cr.unit_type);
						else if(monster->ranged_melee == 1 && cr.has_inherent_buff(BUFF_SHOOTER)) //ranged only
							potential_unit_types.push_back(cr.unit_type);
						else if(monster->ranged_melee == 2 && !cr.has_inherent_buff(BUFF_SHOOTER)) //melee only
							potential_unit_types.push_back(cr.unit_type);
					}
				}
				assert(potential_unit_types.size());
				monster->unit_type = potential_unit_types[rand() % potential_unit_types.size()];
			}
			if(monster->quantity == 0) {
				int size = (13 - (monster->tier * 2)) * 2;
				monster->quantity = utils::rand_range((int)(size * .6), (int)(size * 1.2));
				if(monster->quantity <= 4 && monster->tier == 1)
					throw;
			}
			
		}
	}
	for(auto& h : heroes) {
		auto& hero = h.second;
		hero.init_hero(0, get_tile(hero.x, hero.y).terrain_type);

		//for now, ignore any hero appearance data in the map file (there is none) and use the loaded data from heroes.csv
		for(const auto& hero_info : game_config::get_heroes()) {
			if(hero_info.portrait == hero.portrait) {
				hero.gender = hero_info.gender;
				hero.race = hero_info.race;
				hero.appereance = hero_info.appereance;
			}
		}
		
		for(auto obj : objects) {
			if(!obj || obj->object_type != OBJECT_MAP_TOWN)
				continue;
			
			auto town = (town_t*)obj;
			if(hero.player != town->player)
				continue;
			
			auto gate_tile = get_interactable_coordinate_for_object(town);
			if(hero.x == gate_tile.x && hero.y == gate_tile.y)
				hero_visit_town(&hero, town);
		}
		
	}

	//handle a "uniform" distribution of random artifacts
	std::map<artifact_rarity_e, int> map_artifact_rarity_index;
	
	for(auto& art_list : map_artifacts_by_rarity) {
		std::sort(art_list.second.begin(), art_list.second.end());

		for(auto& art : art_list.second) {
			int offset = map_artifact_rarity_index[art_list.first]++;

			auto artifact = art.second;
			artifact->artifact_id = artifacts_by_rarity[art_list.first][offset % artifacts_by_rarity[art_list.first].size()];
			const auto & artifact_info = game_config::get_artifact(artifact->artifact_id);
			artifact->rarity = artifact_info.rarity;
			artifact->asset_id = artifact_info.asset_id;
		}
	}
	
	
	return 0;
}

artifact_e adventure_map_t::get_random_artifact_of_rarity(artifact_rarity_e rarity) {
	const auto& artifacts = game_config::get_artifacts();
	
	if(!artifacts.size())
		return ARTIFACT_NONE;
	
	//todo: weight this?
	if(rarity == RARITY_UNKNOWN) //random rarity
		return artifacts[rand() % artifacts.size()].id;
	
	std::vector<artifact_e> artifacts_of_rarity;
	
	for(auto& a : artifacts) {
		if(a.rarity == rarity)
			artifacts_of_rarity.push_back(a.id);
	}
	
	if(!artifacts_of_rarity.size())
		return ARTIFACT_NONE;
	
	return artifacts_of_rarity[rand() % artifacts_of_rarity.size()];
}

void adventure_map_t::clear(bool delete_objects, bool clear_tiles) {
	if(clear_tiles)
		tiles.clear();

	doodads.clear();
	heroes.clear();
	//towns.clear();
	if(delete_objects) {
		for(auto obj : objects)
			delete obj;
	}

	objects.clear();
	
	monster_guarded_cache_valid = false;
}

bool adventure_map_t::remove_hero(hero_t* hero) {
	//TODO: handle case where hero is on a ship
	//FIXME: have to remove any reference to this hero (towns, scripts, etc)
	for(auto obj : objects) {
		if(!obj || obj->object_type != OBJECT_MAP_TOWN)
			continue;
		
		auto town = (town_t*)obj;
		if(town->garrisoned_hero == hero)
			town->garrisoned_hero = nullptr;
		else if(town->visiting_hero == hero)
			town->visiting_hero = nullptr;
	}
	
	for(auto it = heroes.begin(); it != heroes.end(); it++) {
		if(&(it->second) == hero) {
			heroes.erase(it);
			return true;
		}
	}
	
	return false;
}

bool adventure_map_t::remove_interactable_object(interactable_object_t* object) {
	for(uint i = 0; i < objects.size(); i++) {
		if(objects[i] == object) {
			auto& tile = get_tile_for_interactable_object(object);
			tile.interactable_object = 0;
			delete objects[i];
			objects[i] = nullptr;
			monster_guarded_cache_valid = false;
			
			return true;
		}
	}
	return false;
}

const map_tile_t& adventure_map_t::get_tile(int x, int y) const {
	assert(x >= 0 && x < width && y >= 0 && y < height);
	assert(y * width + x < tiles.size());
	return tiles[y * width + x];
}

map_tile_t& adventure_map_t::get_tile(int x, int y) {
	assert(x >= 0 && x < width && y >= 0 && y < height);
	assert(y * width + x < tiles.size());
	return tiles[y * width + x];
}

map_tile_t& adventure_map_t::get_tile_for_interactable_object(interactable_object_t* object) {
	assert(object);
	auto coord = get_interactable_coordinate_for_object(object);
	return get_tile(coord.x, coord.y);
}

map_monster_t* adventure_map_t::get_monster_guarding_tile(int x, int y) const {
	interactable_object_t* object = nullptr;
	if(!tile_valid(x, y))
		return nullptr;
		
	//todo: fixme, horribly? inefficient
	for(auto obj : objects) {
		if(!obj || obj->object_type != OBJECT_MAP_MONSTER)
			continue;
		
		if(abs(x - obj->x) <= 1 && abs(y - obj->y) <= 1) {
			object = obj;
			break;
		}
	}
	
	return (map_monster_t*)object;
}

void adventure_map_t::update_guarded_monster_cache() const {
	if(monster_guarded_cache.size() != (width * height)) {
		monster_guarded_cache.resize(width * height);
	}
	
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			int offset = x + (y * height);
			bool guarded = (get_monster_guarding_tile(x, y) != nullptr);
			monster_guarded_cache.setBit(offset, guarded);
		}
	}
	
	monster_guarded_cache_valid = true;
}

bool adventure_map_t::is_tile_guarded_by_monster(int x, int y) const {
	if(!monster_guarded_cache_valid)
		update_guarded_monster_cache();
	
	int offset = x + (y * height);
	return monster_guarded_cache.testBit(offset);
	
	//return get_monster_guarding_tile(x, y) != nullptr;
}

float adventure_map_t::get_terrain_movement_base_cost(const hero_t* hero, int x, int y) const {
	float base_cost = 10.f;
	
	if(!tile_valid(x, y))
		return base_cost;
	
	const auto& tile = get_tile(x, y);
	
	if(tile.road_type == ROAD_DIRT)
		return 8.f;
	else if(tile.road_type == ROAD_GRAVEL)
		return 7.f;
	else if(tile.road_type == ROAD_COBBLESTONE)
		return 5.f;
	
	switch(tile.terrain_type) {
		case TERRAIN_WATER:
			base_cost = 5.f; break;
		case TERRAIN_GRASS:
			base_cost = 10.f; break;
		case TERRAIN_DIRT:
			base_cost = 10.f; break;
		case TERRAIN_WASTELAND:
			base_cost = 12.f; break;
		case TERRAIN_LAVA:
			base_cost = 12.f; break;
		case TERRAIN_DESERT:
			base_cost = 18.f; break;
		case TERRAIN_BEACH:
			base_cost = 12.f; break;
		case TERRAIN_SNOW:
			base_cost = 13.f; break;
		case TERRAIN_SWAMP:
			base_cost = 17.f; break;
		case TERRAIN_JUNGLE:
			base_cost = 15.f; break;
		default:
			base_cost = 10.f; break;
	}
	
	if(!hero)
		return base_cost;

	if(tile.terrain_type != TERRAIN_WATER) {
		if(hero->is_artifact_equipped(ARTIFACT_SWAMPWADERS))
			return 10.f;
		
		float pathfinding_multi = 1.f - get_skill_value(0.4f, .15f, hero->get_secondary_skill_level(SKILL_PATHFINDING));
		float penalty = (base_cost - 10) * pathfinding_multi;
		float cost = 10 + penalty;
		
		return cost;
	}
	
	return base_cost;
}

int adventure_map_t::get_terrain_movement_cost(const hero_t* hero, int x, int y, bool is_diagonal) const {
	int direction_multi = (is_diagonal ? 14 : 10);
	
	return (int)(get_terrain_movement_base_cost(hero, x, y) * direction_multi);
}

//heuristic function for A* pathfinding
int adventure_map_t::heuristic(int x1, int y1, int x2, int y2) const {
	return 1 * (abs(x1 - x2) + abs(y1 - y2)); //manhattan distance
}

route_t adventure_map_t::get_route(const hero_t* hero, int x2, int y2, const game_t* game) const {
	route_t route;
	if(!hero || !tile_valid(x2, y2))
		return route;
	
	std::priority_queue<std::tuple<int, int, int>, std::vector<std::tuple<int, int, int>>, std::greater<std::tuple<int, int, int>>> open_set;
	std::map<coord_t, coord_t> came_from;
	
	std::map<coord_t, int> f_score;
	std::map<coord_t, int> penalty_score;

	//cannot make these static as it is would not be thread-safe
	std::vector<int> g_score;
	g_score.resize(width * height);
	std::fill(g_score.begin(), g_score.end(), -1);
	
	std::vector<float> movement_base_cost_cache;
	movement_base_cost_cache.resize(width * height);
	std::fill(movement_base_cost_cache.begin(), movement_base_cost_cache.end(), -1);
	
	auto get_terrain_movement_base_cost_cached = [this, hero, &movement_base_cost_cache](int x, int y) {
		int index = x + (y * width);
		auto value = movement_base_cost_cache[index];
		if(value <= -1) {
			auto cost = get_terrain_movement_base_cost(hero, x, y);
			movement_base_cost_cache[index] = cost;
			return cost;
		}
		
		return value;
	};

	int x1 = hero->x;
	int y1 = hero->y;
	coord_t start = {x1, y1};
	coord_t goal = {x2, y2};

	//g_score[start.x + (start.y * width)]; //g_score[start] = 0;
	f_score[start] = heuristic(x1, y1, x2, y2);

	open_set.push(std::make_tuple(f_score[start], x1, y1));

	bool on_land = get_tile(x1, y1).terrain_type != TERRAIN_WATER;
	
	std::set<coord_t> open_set_coords;
	
	while(!open_set.empty()) {
		int current_x = std::get<1>(open_set.top());
		int current_y = std::get<2>(open_set.top());
		coord_t current = {current_x, current_y};
		open_set.pop();

		if(current == goal) {
			int max_steps = 10000;
			int s = 0;
			while(current != start) {
				int step_cost = g_score[current.x + (current.y * width)];//g_score[current];
				int penalty = penalty_score[current];
				route.push_front({current, step_cost, penalty});
				current = came_from[current];
				
				if(s++ > max_steps)
					break;
			}
			break;
		}

		int dx[] = { 1, 0, -1, 0, 1, 1, -1, -1 };
		int dy[] = { 0, 1, 0, -1, 1, -1, 1, -1 };

		for (int i = 0; i < 8; i++) {
			int x = current_x + dx[i];
			int y = current_y + dy[i];
			coord_t neighbor = {x, y};

			if (!tile_valid(x, y) || !get_tile(x, y).passability || get_tile(x, y).terrain_type == TERRAIN_UNKNOWN) {
				continue;
			}

			if (!game->is_tile_visible(x, y, hero->player)) {
				continue;
			}

			//the only case where we can go from land to water is if:
			//we are already on land, and the destination tile has a boat
			if(on_land && get_tile(x, y).terrain_type == TERRAIN_WATER) {
				if(x != x2 || y != y2)
					continue;
				
				auto obj = get_interactable_object_for_tile(x, y);
				if(!obj || obj->object_type != OBJECT_SHIP)
					continue;
			}
			
			//we can only disembark onto land as the last step in a route
			if(!on_land && get_tile(x, y).terrain_type != TERRAIN_WATER) {
				if(x != x2 || y != y2)
					continue;
				
				//we cannot interact with objects on land from a boat
				if(get_tile(x, y).is_interactable())
					continue;
			}

			if (get_tile(x, y).is_interactable() && !(x == x2 && y == y2)) {
				continue;
			}
			
			//we cannot move north (or NE/NW) 'through' an object if we are standing on it
			if((current_x == x1 && current_y == y1) && get_tile(x1, y1).is_interactable() &&
			   (	((x == x1) && ((y+1) == y1))
				||  ((x+1 == x1) && ((y+1) == y1))
				||  ((x == x1+1) && ((y+1) == y1))
			   )) {
				auto obj = get_interactable_object_for_tile(current_x, current_y);
				
				if(obj && obj->object_type != OBJECT_SHIP)
					continue;
			}
			
			if(get_tile(x, y).is_interactable() && !is_tile_guarded_by_monster(x, y)) {
				auto obj = get_interactable_object_for_tile(x, y);
				//if the movement is from North or North-West or North-East
				if(dy[i] == 1 && (!interactable_object_t::is_pickupable(obj) && (obj->object_type != OBJECT_SHIP)))
					continue;
			}
			
			if(get_hero_at_tile(x, y) && !(x == x2 && y == y2))
				continue;

			if(is_tile_guarded_by_monster(x, y) && !(x == x2 && y == y2)) {
				auto target_interactable = get_interactable_object_for_tile(x2, y2);
				bool target_monster = target_interactable && target_interactable->object_type == OBJECT_MAP_MONSTER;
				map_monster_t* current_tile_monster = get_monster_guarding_tile(x, y);

				if (!target_monster || (target_monster && current_tile_monster != static_cast<map_monster_t*>(target_interactable))) {
					continue;
				}
			}
			
			//determine if the current move is diagonal (i >= 4) or not (i < 4)
			bool diagonal = (i >= 4);
			float base_cost = get_terrain_movement_base_cost_cached(x, y);
			int move_cost = (int)(base_cost * (diagonal ? 14.f : 10.f));
			int adjusted_g_score = g_score[current.x + (current.y * width)];
			if(adjusted_g_score == -1)
				adjusted_g_score = 0;

			int tentative_g_score = adjusted_g_score + move_cost;// g_score[current] + move_cost;

			if(g_score[neighbor.x + (neighbor.y * width)] == -1 || tentative_g_score < g_score[neighbor.x + (neighbor.y * width)]) {
			//if(g_score.find(neighbor) == g_score.end() || tentative_g_score < g_score[neighbor]) {
				came_from[neighbor] = current;
				g_score[neighbor.x + (neighbor.y * width)] = tentative_g_score; //g_score[neighbor] = tentative_g_score;
				f_score[neighbor] = tentative_g_score + heuristic(x, y, x2, y2);
				penalty_score[neighbor] = (int)(base_cost * 10.f);
				
				if(open_set_coords.find(neighbor) == open_set_coords.end()) {
					open_set.push(std::make_tuple(f_score[neighbor], x, y));
					open_set_coords.insert(neighbor);
				}
			}
		}
	}

	return route;
}

route_t adventure_map_t::get_route_ignoring_blockables(const hero_t* hero, int x2, int y2, const game_t* game) const {
	route_t route;
	if(!hero || !tile_valid(x2, y2))
		return route;
	
	std::priority_queue<std::tuple<int, int, int>, std::vector<std::tuple<int, int, int>>, std::greater<std::tuple<int, int, int>>> open_set;
	std::map<coord_t, coord_t> came_from;
	
	std::map<coord_t, int> f_score;
	std::map<coord_t, int> penalty_score;

	//cannot make these static as it is would not be thread-safe
	std::vector<int> g_score;
	g_score.resize(width * height);
	std::fill(g_score.begin(), g_score.end(), -1);
	
	std::vector<float> movement_base_cost_cache;
	movement_base_cost_cache.resize(width * height);
	std::fill(movement_base_cost_cache.begin(), movement_base_cost_cache.end(), -1);
	
	auto get_terrain_movement_base_cost_cached = [this, hero, &movement_base_cost_cache](int x, int y) {
		int index = x + (y * width);
		auto value = movement_base_cost_cache[index];
		if(value <= -1) {
			auto cost = get_terrain_movement_base_cost(hero, x, y);
			movement_base_cost_cache[index] = cost;
			return cost;
		}
		
		return value;
	};

	int x1 = hero->x;
	int y1 = hero->y;
	coord_t start = {x1, y1};
	coord_t goal = {x2, y2};

	//g_score[start.x + (start.y * width)]; //g_score[start] = 0;
	f_score[start] = heuristic(x1, y1, x2, y2);

	open_set.push(std::make_tuple(f_score[start], x1, y1));

	bool on_land = get_tile(x1, y1).terrain_type != TERRAIN_WATER;
	
	std::set<coord_t> open_set_coords;
	
	while(!open_set.empty()) {
		int current_x = std::get<1>(open_set.top());
		int current_y = std::get<2>(open_set.top());
		coord_t current = {current_x, current_y};
		open_set.pop();

		if(current == goal) {
			int max_steps = 10000;
			int s = 0;
			while(current != start) {
				int step_cost = g_score[current.x + (current.y * width)];//g_score[current];
				int penalty = penalty_score[current];
				route.push_front({current, step_cost, penalty});
				current = came_from[current];
				
				if(s++ > max_steps)
					break;
			}
			break;
		}

		int dx[] = { 1, 0, -1, 0, 1, 1, -1, -1 };
		int dy[] = { 0, 1, 0, -1, 1, -1, 1, -1 };

		for (int i = 0; i < 8; i++) {
			int x = current_x + dx[i];
			int y = current_y + dy[i];
			coord_t neighbor = {x, y};

			if (!tile_valid(x, y) || !get_tile(x, y).passability || get_tile(x, y).terrain_type == TERRAIN_UNKNOWN) {
				continue;
			}

			//ignore visibility for now
			/*if (!game->is_tile_visible(x, y, hero->player)) {
				continue;
			}*/

			//the only case where we can go from land to water is if:
			//we are already on land, and the destination tile has a boat
			if(on_land && get_tile(x, y).terrain_type == TERRAIN_WATER) {
				if(x != x2 || y != y2)
					continue;
				
				auto obj = get_interactable_object_for_tile(x, y);
				if(!obj || obj->object_type != OBJECT_SHIP)
					continue;
			}
			
			//we can only disembark onto land as the last step in a route
			if(!on_land && get_tile(x, y).terrain_type != TERRAIN_WATER) {
				if(x != x2 || y != y2)
					continue;
				
				//we cannot interact with objects on land from a boat
				if(get_tile(x, y).is_interactable())
					continue;
			}

			/*if (get_tile(x, y).is_interactable() && !(x == x2 && y == y2)) {
				continue;
			}*/
			
			//we cannot move north (or NE/NW) 'through' an object if we are standing on it
			if((current_x == x1 && current_y == y1) && get_tile(x1, y1).is_interactable() &&
			   (	((x == x1) && ((y+1) == y1))
				||  ((x+1 == x1) && ((y+1) == y1))
				||  ((x == x1+1) && ((y+1) == y1))
			   )) {
				auto obj = get_interactable_object_for_tile(current_x, current_y);
				
				if(obj && obj->object_type != OBJECT_SHIP)
					continue;
			}
			
			if(get_tile(x, y).is_interactable() && !is_tile_guarded_by_monster(x, y)) {
				auto obj = get_interactable_object_for_tile(x, y);
				//if the movement is from North or North-West or North-East
				if(dy[i] == 1 && (!interactable_object_t::is_pickupable(obj) && (obj->object_type != OBJECT_SHIP)))
					continue;
			}
			
			if(get_hero_at_tile(x, y) && !(x == x2 && y == y2))
				continue;

			/*if(is_tile_guarded_by_monster(x, y) && !(x == x2 && y == y2)) {
				auto target_interactable = get_interactable_object_for_tile(x2, y2);
				bool target_monster = target_interactable && target_interactable->object_type == OBJECT_MAP_MONSTER;
				map_monster_t* current_tile_monster = get_monster_guarding_tile(x, y);

				if (!target_monster || (target_monster && current_tile_monster != static_cast<map_monster_t*>(target_interactable))) {
					continue;
				}
			}*/
			
			//determine if the current move is diagonal (i >= 4) or not (i < 4)
			bool diagonal = (i >= 4);
			float base_cost = get_terrain_movement_base_cost_cached(x, y);
			int move_cost = (int)(base_cost * (diagonal ? 14.f : 10.f));
			int adjusted_g_score = g_score[current.x + (current.y * width)];
			if(adjusted_g_score == -1)
				adjusted_g_score = 0;

			int tentative_g_score = adjusted_g_score + move_cost;// g_score[current] + move_cost;

			if(g_score[neighbor.x + (neighbor.y * width)] == -1 || tentative_g_score < g_score[neighbor.x + (neighbor.y * width)]) {
			//if(g_score.find(neighbor) == g_score.end() || tentative_g_score < g_score[neighbor]) {
				came_from[neighbor] = current;
				g_score[neighbor.x + (neighbor.y * width)] = tentative_g_score; //g_score[neighbor] = tentative_g_score;
				f_score[neighbor] = tentative_g_score + heuristic(x, y, x2, y2);
				penalty_score[neighbor] = (int)(base_cost * 10.f);
				
				if(open_set_coords.find(neighbor) == open_set_coords.end()) {
					open_set.push(std::make_tuple(f_score[neighbor], x, y));
					open_set_coords.insert(neighbor);
				}
			}
		}
	}

	return route;
}

int adventure_map_t::direction_to_offset(adventure_map_direction_e direction) {
	if(direction == DIRECTION_NORTHWEST) return 0;
	if(direction == DIRECTION_NORTH) return 1;
	if(direction == DIRECTION_NORTHEAST) return 2;
	if(direction == DIRECTION_EAST) return 4;
	if(direction == DIRECTION_WEST) return 3;
	if(direction == DIRECTION_SOUTHEAST) return 7;
	if(direction == DIRECTION_SOUTH) return 6;
	if(direction == DIRECTION_SOUTHWEST) return 5;
	
	//assert(0);
	return -1;
}

adventure_map_direction_e adventure_map_t::get_direction(int startx, int starty, int endx, int endy) {
	//no meaningful direction between the same coordinates
	if(startx == endx && starty == endy)
		return DIRECTION_NONE;


	if(endx > startx) { //east 
		if(endy > starty)
			return DIRECTION_SOUTHEAST;
		else if(endy < starty)
			return DIRECTION_NORTHEAST;
		else
			return DIRECTION_EAST;
	}
	else if(endx < startx) { //west
		if(endy > starty)
			return DIRECTION_SOUTHWEST;
		else if(endy < starty)
			return DIRECTION_NORTHWEST;
		else
			return DIRECTION_WEST;
	}
	else { //north/south
		if(endy > starty)
			return DIRECTION_SOUTH;
		else
			return DIRECTION_NORTH;
	}

	//we shouldn't get here
	//assert(0);
	//return DIRECTION_NONE;
}

bool adventure_map_t::are_tiles_adjacent(int x1, int y1, int x2, int y2) const {
	if(!tile_valid(x1, y1) || !tile_valid(x2, y2))
		return false;

	int dx = abs(x1 - x2);
	int dy = abs(y1 - y2);

	return (dx == 1 && dy == 0) ||  //horizontal
		   (dx == 0 && dy == 1) ||  //vertical
		   (dx == 1 && dy == 1);    //diagonal
}

bool adventure_map_t::are_tiles_diagonal(int x1, int y1, int x2, int y2) const {
	if(!tile_valid(x1, y1) || !tile_valid(x2, y2))
		return false;

	int dx = abs(x1 - x2);
	int dy = abs(y1 - y2);

	return (dx == 1 && dy == 1);	//diagonal
}

interactable_object_t* adventure_map_t::get_interactable_object_for_tile(int x, int y) const {
	if(!tile_valid(x, y) || !get_tile(x, y).is_interactable())
		return nullptr;
	
	auto ind = get_tile(x, y).interactable_object - 1;
	assert(ind < (uint16_t)objects.size());
	return objects[ind];
}

//homm2/3:
//Few = 1-4
//Several = 5-9
//Pack = 10-19
//Lots = 20-49
//Horde = 50-99
//Throng = 100-249
//Swarm = 250-499
//Zounds = 500-999
//Legion = 1000+
std::pair<std::string, std::string> adventure_map_t::get_troop_count_strings(uint troop_count, uint scouting_level, bool use_prefix) const {
	std::string text = "???";
	std::string count = "?";
	uint min = 0;
	uint max = 0;
	
	auto set_text = [troop_count, &text, &min, &max] (const char* _name, uint minval, uint maxval) {
		if(troop_count >= minval && troop_count <= maxval) {
			text = QObject::tr(_name).toStdString();
			min = minval;
			max = maxval;
		}
	};
	
	set_text(use_prefix ? "A few" : "Few", 1, 4);
	set_text("Several", 5, 9);
	set_text(use_prefix ? "A pack of" : "Pack", 10, 19);
	set_text(use_prefix ? "Lots of" : "Lots", 20, 49);
	set_text(use_prefix ? "A horde of" : "Horde", 50, 99);
	set_text(use_prefix ? "A throng of" : "Throng", 100, 249);
	set_text(use_prefix ? "A swarm of" : "Swarm", 250, 499);
	set_text(use_prefix ? "Zounds of" : "Zounds", 500, 999);
	
	if(troop_count >= 1000) {
		count = "1k+";
		text = use_prefix ? "A legion of" : "Legion";
	}
	else {
		count = std::to_string(min) + "-" + std::to_string(max);
	}
	
	return std::make_pair(text, count);
}

bool adventure_map_t::can_hero_move_to_tile(hero_t* hero, int x, int y, game_t* game) {
	if(!hero || !game)
		return false;

	//need more sanity checks
	if(!tile_valid(x, y))
		return false;

	if(x == hero->x && y == hero->y) //hero is already on tile
		return false;

	if(!are_tiles_adjacent(hero->x, hero->y, x, y))
		return false;

	if(!get_passability(x, y))
		return false;

	//get movement penalty
	auto& destination_tile = get_tile(x, y);
	auto diagonal = are_tiles_diagonal(hero->x, hero->y, x, y);
	auto movement_cost = get_terrain_movement_cost(hero, x, y, diagonal);

	if(movement_cost > hero->movement_points)
		return false;

	return true;
}

bool adventure_map_t::zero_hero_movement_points_if_low(hero_t* hero, game_t& game) {
	if(!hero)
		return false;

	//look at all available directions a hero might move in from this point.
	//if there is no move that would cost less than or equal to the hero's remaining movement points,
	//we zero out the hero's movement points.
	if(hero->movement_points < 252) { //1.8x penalty for desert * 1.4x penalty for diagonal move
		int min_movement_cost = 10000;
		int dx[] = { 1, 0, -1, 0, 1, 1, -1, -1 };
		int dy[] = { 0, 1, 0, -1, 1, -1, 1, -1 };
		for(int i = 0; i < 8; i++) {
			int ax = hero->x + dx[i];
			int ay = hero->y + dy[i];
			if(!tile_valid(ax, ay))
				continue;

			auto mcost = get_terrain_movement_cost(hero, ax, ay, are_tiles_diagonal(hero->x, hero->y, ax, ay));
			if(mcost < min_movement_cost)
				min_movement_cost = mcost;
		}

		if(hero->movement_points < min_movement_cost) {
			hero->movement_points = 0;
			return true;
		}
	}

	return false;
}

map_action_e adventure_map_t::move_hero_to_tile(hero_t& hero, int x, int y, game_t& game) {
	if(!can_hero_move_to_tile(&hero, x, y, &game))
		return MAP_ACTION_NONE;
	
	//get movement penalty
	auto& destination_tile = get_tile(x, y);
	auto diagonal = are_tiles_diagonal(hero.x, hero.y, x, y);
	auto movement_cost = get_terrain_movement_cost(&hero, x, y, diagonal);
		
	hero.movement_points -= movement_cost;

	game.get_player(hero.player).player_stats.movement.total_hero_steps_taken++;
	game.get_player(hero.player).player_stats.movement.total_hero_movement_points_spent += movement_cost;
	switch(destination_tile.terrain_type) {
		case TERRAIN_WATER: game.get_player(hero.player).player_stats.movement.total_hero_steps_taken_on_water++; break;
		case TERRAIN_GRASS: game.get_player(hero.player).player_stats.movement.total_hero_steps_taken_on_grass++; break;
		case TERRAIN_DIRT: game.get_player(hero.player).player_stats.movement.total_hero_steps_taken_on_dirt++; break;
		case TERRAIN_WASTELAND: game.get_player(hero.player).player_stats.movement.total_hero_steps_taken_on_wasteland++; break;
		case TERRAIN_LAVA: game.get_player(hero.player).player_stats.movement.total_hero_steps_taken_on_lava++; break;
		case TERRAIN_DESERT: game.get_player(hero.player).player_stats.movement.total_hero_steps_taken_on_desert++; break;
		case TERRAIN_BEACH: game.get_player(hero.player).player_stats.movement.total_hero_steps_taken_on_beach++; break;
		case TERRAIN_SNOW: game.get_player(hero.player).player_stats.movement.total_hero_steps_taken_on_snow++; break;
		case TERRAIN_SWAMP: game.get_player(hero.player).player_stats.movement.total_hero_steps_taken_on_swamp++; break;
		case TERRAIN_JUNGLE: game.get_player(hero.player).player_stats.movement.total_hero_steps_taken_on_jungle++; break;
	}

	//todo
	auto& previous_tile = get_tile(hero.x, hero.y);
	if(previous_tile.is_interactable()) {
		auto obj = get_interactable_object_for_tile(hero.x, hero.y);
		if(obj && obj->object_type == OBJECT_MAP_TOWN) {
			auto town = (town_t*)obj;			
			town->visiting_hero = nullptr;
			hero.in_town = false;
		}
	}
	
	hero.x = x;
	hero.y = y;

	//must be called AFTER we update the hero's position above
	zero_hero_movement_points_if_low(&hero, game);

	auto obj = get_interactable_object_for_tile(x, y);
	
	//if we are on a ship, 'drag' it with us to the next (water) tile
	if(hero.is_on_ship()) {
		if(destination_tile.terrain_type == TERRAIN_WATER) {
			destination_tile.interactable_object = previous_tile.interactable_object;
			previous_tile.interactable_object = 0;
//			hero.boarded_ship->x = hero.x;
//			hero.boarded_ship->y = hero.y;
		}
		else if(!obj) { //moving from water to land
			hero_unboard_ship(&hero);
			return MAP_ACTION_HERO_UNBOARDED_SHIP;
		}
	}
	
	if(!obj)
		return MAP_ACTION_VALID_MOVE;
	
	
	if(obj->object_type == OBJECT_SHIP) {
		hero_board_ship(&hero, obj);
		return MAP_ACTION_HERO_BOARDED_SHIP;
	}
	
	return MAP_ACTION_VALID_MOVE;
}
	
map_action_e adventure_map_t::interact_with_object(hero_t* hero, interactable_object_t* object, game_t& game) {
//	if(&hero == nullptr)
//		return MAP_ACTION_NONE;
	
//	auto x = hero.x;
//	auto y = hero.y;
//	//is target tile interactive?
//	auto& tile = get_tile(x, y);
//	if(!tile.is_interactable())
//		return MAP_ACTION_NONE;
//	
//	if(!object)
//		return MAP_ACTION_NONE;
//	
//	//fixme
////	if(!are_tiles_adjacent(hero.x, hero.y, x, y))
////		return MAP_ACTION_NONE;
//		
////return tile.interact_with_object(hero, obj);
//
//	if(object->type == OBJECT_WINDMILL) {
//		auto mill = (flaggable_object_t*)object;
//
//		bool visited = (mill->visited & 0x80) > 0;
//		auto resource = (resource_e)((mill->visited & 0x70) >> 4);
//		auto quantity = mill->visited & 0x0F;
//
//		if(visited)
//			return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
//
//		resource_group_t res;
//		res.set_value_for_type(resource, quantity);
//		game.get_player(hero.player).resources += res;
//
//		mill->owner = hero.player;
//		mill->visited = 0x80;
//		
//		return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
//	}
//	else if(object->type == OBJECT_WATERWHEEL) {
//		auto wheel = (flaggable_object_t*)object;
//		if(wheel->visited)
//			return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
//
//		auto goldval = (game.get_week() == 1 && game.get_month() == 1) ? 500 : 1000;
//
//		resource_group_t res;
//		res.set_value_for_type(RESOURCE_GOLD, goldval);
//		game.get_player(hero.player).resources += res;
//
//		wheel->visited = 1;
//		
//		return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
//	}
////	else if(obj->type == OBJECT_BLACKSMITH) {
////		auto smith = (blacksmith_t*)obj;
////		if(smith->visited)
////			return true;
////
////		if(smith->choice1 != ARTIFACT_NONE) { //artifacts haven't been selected
////
////		}
////
////		app.show_client_dialog(OBJECT_BLACKSMITH, obj, -1);
////	}
//	else if(object->type == OBJECT_MAGIC_SHRINE) {
//		//app.show_client_dialog(OBJECT_MAGIC_SHRINE, object, -1);
//		hero.set_visited_object(object);
//		auto shrine = (shrine_t*)object;
//		if(hero.can_learn_spell(shrine->spell))
//			hero.learn_spell(shrine->spell);
//
//		return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
//	}
////	if(obj->type == OBJECT_MINE) {
////		auto mine = (mine_t*)obj;
////		app.show_client_dialog(obj->type, obj, -1);
////		mine->owner = hero->player;
////		app.update_flaggable_object(mine);
////	}
//	else if(object->type == OBJECT_WATCHTOWER) {
//		game.reveal_area(hero.player, x, y, 20, 20);
//		auto tower = (flaggable_object_t*)object;
//		tower->date_visited = game.date;
//		tower->owner = hero.player;
//		tower->visited = true;
//		//app.update_ui();
//		return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
//	}
//	else if(object->type == OBJECT_WELL) {
//		if(hero.has_object_been_visited(object)) {
//			//app.show_client_dialog(OBJECT_WELL, obj, -1);
//			return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
//		}
//		else {
//			//todo: don't mark object as visited if mana is full
//			hero.mana = std::min(hero.get_maximum_mana(),(uint16_t)(hero.mana + (hero.get_maximum_mana() / 2)));
//			//app.show_client_dialog(OBJECT_WELL, obj, -1);
//			hero.set_visited_object(object);
//			return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
//		}
//	}
//	else if(object->type == OBJECT_SIGN) {
//		//app.show_client_dialog(OBJECT_SIGN, obj, -1);
//		((sign_t*)object)->visited[hero.player - 1] = true; //fixme
//		return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
//	}
//	else if(object->type == OBJECT_SCHOLAR) {
//		auto tomb = (warriors_tomb_t*)object;
//		hero.pickup_artifact(tomb->artifact);
//		return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
//	}
//	else if(object->type == OBJECT_STANDING_STONES) {
//		//app.show_client_dialog(OBJECT_STANDING_STONES, obj, -1);
//		if(!hero.has_object_been_visited(object)) {
//			hero.power++;
//			hero.set_visited_object(object);
//		}
//		return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
//	}
//	else if(object->type == OBJECT_MERCENARY_CAMP) {
//		//app.show_client_dialog(obj->type, obj, -1);
//		if(!hero.has_object_been_visited(object)) {
//			hero.attack++;
//			hero.set_visited_object(object);
//		}
//		return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
//	}
//	else if(object->type == OBJECT_REFUGEE_CAMP) {
//		
//		return MAP_ACTION_SHOW_SECONDARY_INFO_DIALOG;
//	}
//	else if(object->type == OBJECT_TELEPORTER) {
//		auto tele = (teleporter_t*)object;
//		if(tele->type == teleporter_t::TELEPORTER_TYPE_ONE_WAY_EXIT) //nothing to do at an exit-only teleporter
//			return MAP_ACTION_NONE;
//
//		std::vector<teleporter_t*> destinations;
//		for(auto o : objects) {
//			if(!o || o->type != OBJECT_TELEPORTER || o == tele)
//				continue;
//
//			auto dst = (teleporter_t*)o;
//			//check teleporter type/color compatibility
//			if(dst->color != tele->color || dst->type == teleporter_t::TELEPORTER_TYPE_ONE_WAY_ENTRANCE)
//				continue;
//			
//			//check to see if destination is occupied or not
//			auto dest_tile = get_interactable_coordinate_for_object(dst);
//			bool occupied = false;
//			for(auto& h : heroes) {
//				if(h.second.x == dest_tile.x && h.second.y == dest_tile.y)
//					occupied = true;
//			}
//			
//			if(occupied)
//				continue;
//			
//			destinations.push_back(dst);
//		}
//
//		if(!destinations.size()) //no valid destinations
//			return MAP_ACTION_NONE;
//
//		auto dest = destinations.at(rand() % destinations.size());
//		auto teleport_dest_tile = get_interactable_coordinate_for_object(dest);
//		hero.x = teleport_dest_tile.x;
//		hero.y = teleport_dest_tile.y;
//		
//		return MAP_ACTION_HERO_TELEPORTED;
//	}
//	
//	switch(object->type) {
//		case OBJECT_ARENA:
//		case OBJECT_GAZEBO:
//		case OBJECT_WITCH_HUT:
//		case OBJECT_SCHOOL_OF_MAGIC:
//		case OBJECT_SCHOOL_OF_WAR:
//		case OBJECT_WATERWHEEL:
//		case OBJECT_WINDMILL:
//		case OBJECT_PYRAMID:
//		case OBJECT_WARRIORS_TOMB:
//			return MAP_ACTION_SHOW_PRIMARY_INFO_DIALOG;
//			
//	}
	return MAP_ACTION_NONE;
}

bool adventure_map_t::move_troops_between_heroes(hero_t* source_hero, uint source_slot, hero_t* dest_hero, uint dest_slot) {
	if(!source_hero || !dest_hero)
		return false;
	
	if(source_slot >= game_config::HERO_TROOP_SLOTS || dest_slot >= game_config::HERO_TROOP_SLOTS)
		return false;
	
	if(source_hero == dest_hero)
		return false; //this should not happen
	
	if(source_hero->troops[source_slot].is_empty())
		return false;
	
	//todo:  verify that heroes are able to transfer troops (adjacent, same player, etc.)
	
	if(dest_hero->troops[dest_slot].is_empty()) {
		dest_hero->troops[dest_slot] = source_hero->troops[source_slot];
		source_hero->troops[source_slot].clear();
		return true;
	}
	else if(dest_hero->troops[dest_slot].unit_type == source_hero->troops[source_slot].unit_type) {
		dest_hero->troops[dest_slot].stack_size += source_hero->troops[source_slot].stack_size;
		source_hero->troops[source_slot].clear();
		return true;
	}
	else {
		std::swap(source_hero->troops[source_slot], dest_hero->troops[dest_slot]);
		return true;
	}
		
	return false;
}

bool adventure_map_t::sort_hero_backpack(hero_t* hero) {
	if(!hero)
		return false;
	
	//sort by slot, then by rarity, then by name
	std::sort(hero->backpack.begin(), hero->backpack.end(), [](artifact_e l, artifact_e r) {
		if(l == ARTIFACT_NONE) return false;
		if(r == ARTIFACT_NONE) return true;
		
		const auto& left = game_config::get_artifact(l);
		const auto& right = game_config::get_artifact(r);
		
		//compare by slot, then by rarity, then by name
		if(left.slot != right.slot)
			return left.slot < right.slot;
		if(left.rarity != right.rarity)
			return left.rarity < right.rarity;
		
		return left.name < right.name;
	});
	
	return true;
}

bool adventure_map_t::swap_hero_artifacts(hero_t* hero1, hero_t* hero2, bool include_backpack, bool backpack_only) {
	if(!hero1 || !hero2)
		return false;
	
	//validation
	//bug here where swapping between a barbarian with a weapon in shield slot swapping
	//with a non-barbarian will put weapon in the non-barbarian's offhand
	if(!backpack_only)
		std::swap(hero1->artifacts, hero2->artifacts);
	
	if(include_backpack)
		std::swap(hero1->backpack, hero2->backpack);
	
	return true;
}

bool adventure_map_t::move_all_artifacts_from_hero_to_hero(hero_t* hero_from, hero_t* hero_to, bool include_backpack, bool backpack_only) {
	//need some checks here
	if(!hero_from || !hero_to)
		return false;

	for(int i = 0; i < hero_from->artifacts.size(); i++) {
		if(hero_to->pickup_artifact(hero_from->artifacts[i]))
			hero_from->artifacts[i] = ARTIFACT_NONE;
	}

	for(int i = 0; i < hero_from->backpack.size(); i++) {
		if(hero_to->pickup_artifact(hero_from->backpack[i]))
			hero_from->backpack[i] = ARTIFACT_NONE;
	}

	return true;
}

bool adventure_map_t::move_artifact_from_hero_to_hero(hero_t* hero_from, hero_t* hero_to, uint from_slot, uint to_slot) {
	if(!hero_from || !hero_to)
		return false;
	if(from_slot == 0 || from_slot > game_config::HERO_ARTIFACT_SLOTS || to_slot == 0 || to_slot > game_config::HERO_ARTIFACT_SLOTS)
		return false;
	
	if(from_slot == to_slot) {
		std::swap(hero_from->artifacts[from_slot-1], hero_to->artifacts[to_slot-1]);
		return true;
	}
	
	//need to check viability of transfer
	auto& src_artifact = game_config::get_artifact(hero_from->artifacts[from_slot-1]);
	if(!hero_to->does_artifact_fit_in_slot(src_artifact.slot, to_slot))
		return false;
	
	if(hero_to->artifacts[to_slot-1] == ARTIFACT_NONE) { //artifact to an empty slot
		std::swap(hero_from->artifacts[from_slot-1], hero_to->artifacts[to_slot-1]);
		return true;
	}
	else { //swap artifacts (artifacts in both slots) if they are compatible
		auto& dest_artifact = game_config::get_artifact(hero_to->artifacts[to_slot-1]);
		if(!hero_from->does_artifact_fit_in_slot(dest_artifact.slot, from_slot))
			return false;
		
		std::swap(hero_from->artifacts[from_slot-1], hero_to->artifacts[to_slot-1]);
		return true;
	}
	
	return false;
}

bool adventure_map_t::move_artifact_from_hero_backpack_to_hero_backpack(hero_t* hero_from, hero_t* hero_to, uint from_slot, uint to_slot) {
	if(!hero_from || !hero_to)
		return false;
	if(from_slot > game_config::HERO_BACKPACK_SLOTS || to_slot > game_config::HERO_BACKPACK_SLOTS)
		return false;

	std::swap(hero_from->backpack[from_slot], hero_to->backpack[to_slot]);

	return true;
}

bool adventure_map_t::move_artifact_from_hero_slot_to_hero_backpack(hero_t* hero_from, hero_t* hero_to, uint from_slot, uint to_slot) {
	if(!hero_from || !hero_to)
		return false;
	if(from_slot == 0 || from_slot > game_config::HERO_BACKPACK_SLOTS || to_slot > game_config::HERO_BACKPACK_SLOTS)
		return false;
	
	auto& dest_artifact = game_config::get_artifact(hero_to->backpack[to_slot]);
	//if there is an artifact in the dest slot, and it fits in the slot we're taking the source artifact from,
	//then we can just swap the artifacts
	if(hero_from->does_artifact_fit_in_slot(dest_artifact.slot, from_slot)) {
		std::swap(hero_from->artifacts[from_slot - 1], hero_to->backpack[to_slot]);
		return true;
	}

	if(hero_to->is_backpack_full())
		return false;

	//first try to put the artifact in the desired backpack slot, if it is empty
	if(hero_to->backpack[to_slot] == ARTIFACT_NONE) {
		hero_to->backpack[to_slot] = hero_from->artifacts[from_slot - 1];
		hero_from->artifacts[from_slot - 1] = ARTIFACT_NONE;
		return true;
	}
	
	//if there was an artifact in that slot, then we move the artifact to the next available slot
	for(int i = 0; hero_to->backpack.size(); i++) {
		if(hero_to->backpack[i] == ARTIFACT_NONE) {
			hero_to->backpack[i] = hero_from->artifacts[from_slot - 1];
			hero_from->artifacts[from_slot - 1] = ARTIFACT_NONE;
			return true;
		}
	}
	
	return false;
}

bool adventure_map_t::move_artifact_from_hero_backpack_to_hero_slot(hero_t* hero_from, hero_t* hero_to, uint from_slot, uint to_slot) {
	if(!hero_from || !hero_to)
		return false;
	if(from_slot > game_config::HERO_BACKPACK_SLOTS || to_slot == 0 || to_slot > game_config::HERO_BACKPACK_SLOTS)
		return false;
	
	auto& src_artifact = game_config::get_artifact(hero_from->backpack[from_slot]);
	if(!hero_to->does_artifact_fit_in_slot(src_artifact.slot, to_slot))
		return false;
	
	std::swap(hero_from->backpack[from_slot], hero_to->artifacts[to_slot - 1]);
	
	return true;
}

bool adventure_map_t::swap_hero_everything(hero_t* hero1, hero_t* hero2) {
	//need some checks here
	if(!hero1 || !hero2)
		return false;

	std::swap(hero1->troops, hero2->troops);
	std::swap(hero1->artifacts, hero2->artifacts);
	std::swap(hero1->backpack, hero2->backpack);

	return true;
}

bool adventure_map_t::swap_hero_armies(hero_t* hero1, hero_t* hero2) {
	//need some checks here
	if(!hero1 || !hero2)
		return false;
	
	std::swap(hero1->troops, hero2->troops);
	
	return true;
}

int adventure_map_t::get_next_open_troop_slot(const troop_group_t& troops) {
	int i = 0;
	for(auto& s : troops) {
		if(s.is_empty())
			return i;

		i++;
	}

	return -1;
}

bool adventure_map_t::dismiss_troops(hero_t* hero, int troop_index) {
	if(!hero || troop_index < 0 || troop_index > game_config::HERO_TROOP_SLOTS)
		return false;

	hero->troops[troop_index].clear();
	return true;
}

void adventure_map_t::clear_troop_slots(troop_group_t& troops) {
	for(auto& t : troops)
		t.clear();
}

bool adventure_map_t::swap_troop_slots(troop_group_t& troops, uint slot1, uint slot2) {
	if(slot1 >= game_config::HERO_TROOP_SLOTS || slot2 >= game_config::HERO_TROOP_SLOTS || slot1 == slot2)
		return false;

	std::swap(troops[slot1], troops[slot2]);
	return true;
}

bool adventure_map_t::combine_troop_slots(troop_group_t& troops, uint slot1, uint slot2) {
	if(slot1 >= game_config::HERO_TROOP_SLOTS || slot2 >= game_config::HERO_TROOP_SLOTS || slot1 == slot2)
		return false;
	
	//todo: validation / overflow
	if(troops[slot1].unit_type != troops[slot2].unit_type)
		return false;

	troops[slot2].stack_size += troops[slot1].stack_size;
	troops[slot1].clear();
	return true;
}

bool adventure_map_t::split_troops(troop_group_t& troops, uint slot, split_mode_e mode, int count) {
	if(slot >= game_config::HERO_TROOP_SLOTS || troops[slot].stack_size == 0 || count > UINT16_MAX)
		return false;

	auto type = troops[slot].unit_type;

	if(mode == SPLIT_ONE) {
		if(troops[slot].stack_size < 2)
			return false;

		auto pos = get_next_open_troop_slot(troops);
		if(pos == -1)
			return false;

		troops[pos].unit_type = type;
		troops[pos].stack_size = 1;
		troops[slot].stack_size--;

		return true;
	}

	//case 1: we have N stacks where N is 2 or more of the same unit type split unevenly:
	//outcome: split the troops evenly between N stacks
	//case 2: we have N stacks (including N == 1) of the same unit type split evenly:
	//outcome: split the troops evenly between N+1 stacks
	//	!ignore any 1-size stacks when splitting!

	if(mode == SPLIT_EVEN) {
		if(troops[slot].stack_size < 2)
			return false;

		//are existing units split evenly?
		bool even = true;
		int groups = 0;
		int total = 0;
		for(auto t : troops) {
			if(t.unit_type == type && t.stack_size > 1) {
				groups++;
				total += t.stack_size;
			}
		}
		int avg = total / groups;
		for(auto t : troops) {
			if(t.unit_type == type && t.stack_size > 1 && abs((int)t.stack_size - avg) > 1)
				even = false;
		}

		//if they are NOT split evenly, split them evenly
		//given N and M, put {N/M} + 1 in N(mod M) groups and {N/M} in the rest
		if(even == false) {
			int m = 0;
			for(auto& t : troops) {
				if(t.unit_type == type && t.stack_size > 1) {
					t.stack_size = m < (total % groups) ? (avg + 1) : (total / groups);
					m++;
				}
			}
			return true;
		}
		//if they ARE evenly split, split into N+1 groups
		auto pos = get_next_open_troop_slot(troops);
		if(pos == -1)
			return false;

		groups += 1;
		avg = total / groups;
		troops[pos].unit_type = type;
		troops[pos].stack_size = 0;
		int m = 0;
		int i = 0;
		for(auto& t : troops) {
			if(t.unit_type == type && (t.stack_size > 1 || i == pos)) {
				t.stack_size = m < (total% groups) ? (avg + 1) : (total / groups);
				m++;
			}
			i++;
		}
		return true;
	}

	return true;
}

bool adventure_map_t::merge_troops(troop_group_t& troops, uint slot) {
	bool moved = false;
	for(uint i = 0; i < game_config::HERO_TROOP_SLOTS; i++) {
		if(i == slot)
			continue;

		if(troops[i].unit_type == troops[slot].unit_type) {
			troops[slot].stack_size += troops[i].stack_size;
			troops[i].clear();
			moved = true;
		}
	}

	return moved;
}

bool adventure_map_t::move_troop_stack_from_hero_to_hero(hero_t* hero_from, hero_t* hero_to, uint stack, bool move_only_one_troop) {
	if(!hero_from || !hero_to || stack >= game_config::HERO_TROOP_SLOTS)
		return false;
	
	auto& src_troop = hero_from->troops[stack];
	if(src_troop.is_empty())
		return false;

	if(move_only_one_troop) {
		auto open_slot = get_next_open_troop_slot(hero_to->troops);
		if(open_slot != -1) {
			hero_to->troops[open_slot].stack_size = 1;
			hero_to->troops[open_slot].unit_type = src_troop.unit_type;
			src_troop.stack_size--;
			return true;
		}
	}

	bool moved = false;
	for(uint n = 0; n < game_config::HERO_TROOP_SLOTS; n++) {
		if(src_troop.unit_type == hero_to->troops[n].unit_type) {
			if(move_only_one_troop) {
				hero_to->troops[n].stack_size++;
				src_troop.stack_size--;
			}
			else {
				hero_to->troops[n].stack_size += src_troop.stack_size;
				src_troop.clear();
			}
			moved = true;
			break;
		}
	}
	if(!moved) {
		auto open_slot = get_next_open_troop_slot(hero_to->troops);
		if(open_slot == -1)
			return false;
		
		hero_to->troops[open_slot] = src_troop;
		src_troop.clear();
	}
	
	return true;
}

bool adventure_map_t::move_army_from_hero_to_hero(hero_t* hero_from, hero_t* hero_to) {
	//need some checks here
	if(!hero_from || !hero_to)
		return false;
	
	int result = 0;
	//todo: use function for this
	for(uint i = 0; i < game_config::HERO_TROOP_SLOTS; i++)
		result += move_troop_stack_from_hero_to_hero(hero_from, hero_to, i);
	
	return result;
}

bool adventure_map_t::move_troops_from_army_to_army(troop_group_t& source, uint source_slot, troop_group_t& dest, uint dest_slot) {
	if(source_slot >= game_config::HERO_TROOP_SLOTS || dest_slot >= game_config::HERO_TROOP_SLOTS)
		return false;
	
	auto& src_troop = source[source_slot];
	auto& dest_troop = dest[dest_slot];
	
	if(dest_troop.is_empty()) {
		dest_troop = src_troop;
		src_troop.clear();
		return true;
	}
	else if(src_troop.unit_type == dest_troop.unit_type) {
		dest_troop.stack_size += src_troop.stack_size;
		src_troop.clear();
		return true;
	}
	else { //swap troops
		std::swap(src_troop, dest_troop);
		return true;
	}
	
	return false;
}

bool adventure_map_t::move_troops_to_garrison(hero_t* source_hero, uint source_slot, town_t* dest_town, uint dest_slot) {
	//assert(source_hero && dest_town);
	if(!source_hero || !dest_town)
		return false;
	
	if(dest_town->garrisoned_hero) //move_troops_between_heroes() needs to be called if hero is garrisoned
		return false;
	
	return move_troops_from_army_to_army(source_hero->troops, source_slot, dest_town->garrison_troops, dest_slot);
//	if(source_slot >= game_config::HERO_TROOP_SLOTS || dest_slot >= game_config::HERO_TROOP_SLOTS)
//		return false;
//
//
//	auto& src_troop = source_hero->troops[source_slot];
//	auto& dest_troop = dest_town->garrison_troops[dest_slot];
//
//	if(dest_troop.is_empty()) {
//		dest_troop = src_troop;
//		src_troop.clear();
//		return true;
//	}
//	else if(src_troop.unit_type == dest_troop.unit_type) {
//		dest_troop.stack_size += src_troop.stack_size;
//		src_troop.clear();
//		return true;
//	}
//
//	//fall through to failure if destination is not empty or does not match unit type
//	return false;
}

bool adventure_map_t::move_troops_from_garrison(town_t* source_town, uint source_slot, hero_t* dest_hero, uint dest_slot) {
	//assert(source_town && dest_hero);
	if(!source_town || !dest_hero)
		return false;
	
	if(source_slot >= game_config::HERO_TROOP_SLOTS || dest_slot >= game_config::HERO_TROOP_SLOTS)
		return false;
	
	if(source_town->garrisoned_hero) //move_troops_between_heroes() needs to be called if hero is garrisoned
		return false;
	
	auto& src_troop = source_town->garrison_troops[source_slot];
	auto& dest_troop = dest_hero->troops[dest_slot];

	if(dest_troop.is_empty()) {
		dest_troop = src_troop;
		src_troop.clear();
		return true;
	}
	else if(src_troop.unit_type == dest_troop.unit_type) {
		dest_troop.stack_size += src_troop.stack_size;
		src_troop.clear();
		return true;
	}
	else { //swap garrison troops with hero troop
		std::swap(src_troop, dest_troop);
		return true;
	}
	
	//fall through to failure if destination is not empty or does not match unit type
	return false;
}

bool adventure_map_t::move_troops_within_army(troop_group_t& troops, uint source_slot, uint dest_slot) {
	if(source_slot >= game_config::HERO_TROOP_SLOTS || dest_slot >= game_config::HERO_TROOP_SLOTS)
		return false;
	
	if(source_slot == dest_slot)
		return false;
	
	auto& src_troop = troops[source_slot];
	auto& dest_troop = troops[dest_slot];
	if(src_troop.unit_type == dest_troop.unit_type) {
		dest_troop.stack_size += src_troop.stack_size;
		src_troop.clear();
		return true;
	}
	else {
		std::swap(src_troop, dest_troop);
		return true;
	}
	
	return false;
}

bool adventure_map_t::move_troops_within_garrison(town_t* town, uint source_slot, uint dest_slot) {
	if(!town)
		return false;
	
	return move_troops_within_army(town->garrison_troops, source_slot, dest_slot);
//	if(source_slot >= game_config::HERO_TROOP_SLOTS || dest_slot >= game_config::HERO_TROOP_SLOTS)
//		return false;
//
//	if(source_slot == dest_slot)
//		return false;
//
//	auto& src_troop = town->garrison_troops[source_slot];
//	auto& dest_troop = town->garrison_troops[dest_slot];
//	if(src_troop.unit_type == dest_troop.unit_type) {
//		dest_troop.stack_size += src_troop.stack_size;
//		src_troop.clear();
//		return true;
//	}
//	else {
//		std::swap(src_troop, dest_troop);
//		return true;
//	}
//
//	return false;
}

bool adventure_map_t::move_all_troops_from_garrison_to_visiting_hero(town_t* town) {
	//assert(town && town->visiting_hero)
	if(!town || !town->visiting_hero)
		return false;
	
	auto& troops = town->garrisoned_hero ? town->garrisoned_hero->troops : town->garrison_troops;
	
	//todo: use _move_and_combine_troops()
	
	for(uint i = 0; i < game_config::HERO_TROOP_SLOTS; i++) {
		auto& src_troop = troops[i];
		bool moved = false;
		for(uint n = 0; n < game_config::HERO_TROOP_SLOTS; n++) {
			if(src_troop.unit_type == town->visiting_hero->troops[n].unit_type) {
				town->visiting_hero->troops[n].stack_size += src_troop.stack_size;
				src_troop.clear();
				moved = true;
				break;
			}
		}
		if(!moved) {
			auto open_slot = get_next_open_troop_slot(town->visiting_hero->troops);
			if(open_slot == -1)
				continue;
			
			town->visiting_hero->troops[open_slot] = src_troop;
			src_troop.clear();
		}
	}
	
	return true;
}

//not sure where this should go

//int get_next_open_slot(const troop_t troops) {
//	int i = 0;
//	for(auto& s : troops) {
//		if(s.stack_size == 0)
//			return i;
//
//		i++;
//	}
//
//	return -1;
//}

bool adventure_map_t::move_all_troops_from_visiting_hero_to_garrison(town_t* town) {
	if(!town || !town->visiting_hero)
		return false;
	
	auto& dest_troops = town->garrisoned_hero ? town->garrisoned_hero->troops : town->garrison_troops;
	
	for(uint i = 0; i < game_config::HERO_TROOP_SLOTS; i++) {
		auto& src_troop = town->visiting_hero->troops[i];
		bool moved = false;
		for(uint n = 0; n < game_config::HERO_TROOP_SLOTS; n++) {
			if(src_troop.unit_type == dest_troops[n].unit_type) {
				dest_troops[n].stack_size += src_troop.stack_size;
				src_troop.clear();
				moved = true;
				break;
			}
		}
		if(!moved) {
			//fix this crap
			int open_slot = -1;
			int n = 0;
			for(auto& s : dest_troops) {
				if(s.is_empty()) {
					open_slot = n;
					break;
				}
		
				n++;
			}
			
			//auto open_slot = get_next_open_slot();
			if(open_slot == -1)
				continue;
			
			dest_troops[open_slot] = src_troop;
			src_troop.clear();
		}
	}
	
	return true;
}

bool adventure_map_t::can_add_troop_to_group(const troop_t& troop, const troop_group_t& group) {
	auto open_slot = adventure_map_t::get_next_open_troop_slot(group);
	if(open_slot != -1)
		return true;
	
	for(const auto& tr : group) {
		if(tr.unit_type == troop.unit_type) {
			//todo: check for overflow
			return true;
		}
	}
	
//	std::array<unit_type_e, game_config::HERO_TROOP_SLOTS> unit_types;
//	std::fill(unit_types.begin(), unit_types.end(), UNIT_UNKNOWN);
//
	
	std::set<unit_type_e> unit_types;
	for(const auto& tr : group) {
		if(tr.is_empty()) //should never happen due to check at function start
			continue;
		
		if(unit_types.count(tr.unit_type)) {
			//todo: check for overflow
			return true;
		}
		
		unit_types.insert(tr.unit_type);
	}
	
	return false;
}

bool adventure_map_t::add_troop_to_group_combining(const troop_t& troop, troop_group_t& group) {
	if(!can_add_troop_to_group(troop, group))
		return false;
	
	for(auto& tr : group) {
		if(tr.unit_type == troop.unit_type) {
			//todo: check for overflow
			tr.stack_size += troop.stack_size;
			return true;
		}
	}
	
	auto open_slot = adventure_map_t::get_next_open_troop_slot(group);
	if(open_slot != -1) {
		group[open_slot] = troop;
		return true;
	}
	
	//fixme: handle overflow
	for(uint i = 0; i < group.size(); i++) {
		for(uint n = 0; n < group.size(); n++) {
			if(i == n)
				continue;
			
			if(group[i].unit_type == group[n].unit_type) {
				group[i].stack_size += group[n].stack_size;
				group[n] = troop;
				return true;
			}
		}
	}
	
	return false;
}

bool adventure_map_t::garrison_hero(hero_t* hero, town_t* town) {
	if(!hero || !town || town->garrisoned_hero)
		return false;
	
	bool can_combine = true; //fixme can_combine_troops(hero->troops, town.garrison_troops);
	if(!can_combine)
		return false;
	
	//moving hero with troops to garrison with troops is equivalent to moving garrison troops down, then moving hero up
	if(!move_all_troops_from_garrison_to_visiting_hero(town))
		return false;
	
	town->visiting_hero = nullptr;
	town->garrisoned_hero = hero;
	//todo: need more checks here
	auto pos = get_interactable_coordinate_for_object(town);
	hero->x = pos.x;
	hero->y = pos.y;
	hero->garrisoned = true;
	
	return true;
}

bool adventure_map_t::ungarrison_hero(hero_t* hero, town_t* town) {
	if(!hero || !town || town->visiting_hero)
		return false;
	
	town->garrisoned_hero = nullptr;
	town->visiting_hero = hero;
	//todo: need more checks here
	auto pos = get_interactable_coordinate_for_object(town);
	hero->x = pos.x;
	hero->y = pos.y;
	hero->garrisoned = false;
	
	return true;
}


bool adventure_map_t::hero_visit_town(hero_t* hero, town_t* town) {
	if(!hero || !town)
		return false;
	
	//if(hero->x != town->x || hero->y != town->y)
	//return false;
	
//	if(town->visiting_hero)
//		return false;
	
	town->visiting_hero = hero;
	hero->in_town = true;
	
	for(auto sp : town->mage_guild_spells)
		hero->learn_spell(sp);
	
	return true;
}

bool adventure_map_t::hero_board_ship(hero_t* hero, interactable_object_t* ship) {
	if(!hero || !ship || ship->object_type != OBJECT_SHIP)
		return false;
	
	hero->movement_points = std::clamp((int16_t)((int)hero->movement_points - (hero->movement_points / 2)), (int16_t)0, hero->movement_points);
	hero->boarded_ship = ship;
	return true;
}

bool adventure_map_t::hero_unboard_ship(hero_t* hero) {
	if(!hero || !hero->boarded_ship)
		return false;
	
	hero->movement_points = std::clamp((int16_t)((int)hero->movement_points - (hero->movement_points / 2)), (int16_t)0, hero->movement_points);
	hero->boarded_ship = nullptr;
	return true;
}

hero_t* adventure_map_t::get_hero_on_ship(interactable_object_t* ship) {
	if(ship->object_type != OBJECT_SHIP)
		return nullptr;

	for(const auto& h : heroes) {
		if(h.second.boarded_ship == ship)
			return &(heroes[h.first]);
	}

	return nullptr;
}

bool adventure_map_t::swap_visiting_and_garrisoned_heroes(town_t* town) {
	if(!town)
		return false;
	
	auto visiting = town->visiting_hero;
	auto garrisoned = town->garrisoned_hero;
	if(!visiting && !garrisoned)
		return false;
	
	town->visiting_hero = garrisoned;
	if(town->visiting_hero)
		town->visiting_hero->garrisoned = false;
	
	town->garrisoned_hero = visiting;
	if(town->garrisoned_hero)
		town->garrisoned_hero->garrisoned = true;
	
	return true;
}

coord_t adventure_map_t::get_interactable_offset_for_object(const interactable_object_t* object) const {
	if(!object)
		return {0, 0};
	
	auto it = interactivity_map.find(object->asset_id);
	if (it == interactivity_map.end())
		return {0, 0};
	
	const auto& set = it->second;
	int obj_width = object->width;

	if(obj_width == 0)
		return {0, 0};

	for(int i = 0; i < (int)set.size(); i++) {
		if(set[i]) {
			int x = i % obj_width;
			int y = i / obj_width;
			return {x, y};
		}
	}
	
	return {0, 0};
}

//todo combine these functions
bool adventure_map_t::is_offset_passable(const doodad_t* doodad, int x, int y) {
	if(!passability_map.count(doodad->asset_id))
		return false;
	
	auto& set = passability_map[doodad->asset_id];
	int pos = doodad->width * y + x;

	if(pos < 0 || pos > set.size())
		return true;
	
	return set[pos];
}

bool adventure_map_t::is_offset_passable(const interactable_object_t* object, int x, int y) {
	if(!passability_map.count(object->asset_id))
		return false;
	
	auto& set = passability_map[object->asset_id];
	
	int obj_width = object->width;
	int obj_height = object->height;
	
	int pos = obj_width * y + x;
	//assert(pos >= 0 && pos < set.size());
	if(pos < 0 || pos > set.size())
		return true;
	
	return set[pos];
}

bool adventure_map_t::is_offset_interactable(const interactable_object_t* object, int x, int y) {
	if(!object)
		return false;
	
	auto offset = get_interactable_offset_for_object(object);
	
	return (x == offset.x) && (y == offset.y);
}

coord_t adventure_map_t::get_interactable_coordinate_for_object(const interactable_object_t* object) const {
	if(!object)
		return {-1, -1};
	
	auto offset = get_interactable_offset_for_object(object);
	return {object->x + offset.x, object->y + offset.y};
}

interactable_object_t* adventure_map_t::get_object_by_id(uint16_t object_id, interactable_object_e match_type) {
	for(uint16_t i = 0; i < objects.size(); i++) {
		if(!objects[i] || i != object_id)
			continue;
		
		if(match_type == OBJECT_UNKNOWN || objects[i]->object_type == match_type)
			return objects[i];
	}
	
	return nullptr;
}

uint16_t adventure_map_t::get_object_id(interactable_object_t* object) const {
	if(!object)
		return 0;

	//assert(objects.size() < MAXIMUM_OBJECTS_COUNT);
	for(uint16_t i = 0; i < objects.size(); i++) {
		if(object == objects[i])
			return i;
	}
	
	return 0;
}

const hero_t* adventure_map_t::get_hero_at_tile(int x, int y) const {
	for(auto& h : heroes) {
		if(h.second.x == x && h.second.y == y && !h.second.garrisoned)
			return &h.second;
	}

	return nullptr;
};

hero_t* adventure_map_t::get_hero_at_tile(int x, int y) {
	for(auto& h : heroes) {
		if(h.second.x == x && h.second.y == y && !h.second.garrisoned)
			return &h.second;
	}

	return nullptr;
}

int adventure_map_t::get_owned_mines_count(resource_e mine_type, player_e player) {
	int count = 0;
	
	for(auto obj : objects) {
		if(!obj || obj->object_type != OBJECT_MINE)
			continue;
		
		auto mine = (mine_t*)obj;
		
		if(mine->mine_type != mine_type)
			continue;
		
		//if we pass in PLAYER_NONE, return all mines that are owned by some player
		if(player == PLAYER_NONE && mine->owner != PLAYER_NONE)
			count++;
		
		if(mine->owner != PLAYER_NONE && mine->owner == player)
			count++;
	}
	
	return count;
}

bool adventure_map_t::get_passability(int x, int y) const {
	if(!tile_valid(x, y))
		return false;
	
	return get_tile(x, y).passability;
}

bool adventure_map_t::is_route_passable(hero_t* hero, int x2, int y2, game_t* game) const {
	auto route = get_route(hero, x2, y2, game);
	return route.size() != 0;
}

int adventure_map_t::get_time_to_tile(const hero_t* hero, int x, int y, game_t* game) const {
	if(!hero)
		return -1;

	//todo
	return 1;
}

int adventure_map_t::get_route_cost_to_tile(const hero_t* hero, int x, int y, const game_t* game) const {
	if(!hero)
		return -1;

	auto route = get_route(hero, x, y, game);
	if(!route.size())
		return -1;
	
	return route.back().total_cost;
}


//
////use a* to find the route
//route_t adventure_map_t::get_route(int x1, int y1, int x2, int y2)
//{
//	std::vector<map_tile_t*> open_list;
//	std::vector<map_tile_t*> closed_list;
//	std::vector<map_tile_t*> route;
//	
//	map_tile_t* start_tile = &get_tile(x1, y1);
//	map_tile_t* end_tile = &get_tile(x2, y2);
//	open_list.push_back(start_tile);
//	
//	while(open_list.size() > 0) {
//		map_tile_t* current_tile = open_list[0];
//		if(current_tile == end_tile) {
//			while(current_tile != start_tile) {
//				route.push_back(current_tile);
//				current_tile = current_tile->parent;
//			}
//			std::reverse(route.begin(), route.end());
//			return route;
//		}
//		open_list.erase(open_list.begin());
//		closed_list.push_back(current_tile);
//		for(auto xoff = -1; xoff <= 1; xoff++) {
//			for(auto yoff = -1; yoff <= 1; yoff++) {
//				if(xoff == 0 && yoff == 0)
//					continue;
//				
//				int x = current_tile->x + xoff;
//				int y = current_tile->y + yoff;
//				if(x < 0 || x >= width || y < 0 || y >= height)
//					continue;
//				
//				map_tile_t* neighbor_tile = &get_tile(x, y);
//				if(!neighbor_tile->passability.passable)
//					continue;
//				if(std::find(closed_list.begin(), closed_list.end(), neighbor_tile) != closed_list.end()) {
//					continue;
//				}
//				if(std::find(open_list.begin(), open_list.end(), neighbor_tile) != open_list.end()) {
//					continue;
//				}
//				neighbor_tile->parent = current_tile;
//				open_list.push_back(neighbor_tile);
//			}
//		}
//		for(auto i = 0; i < 8; i++) {
//			map_tile_t* neighbor = current_tile->neighbors[i];
//			
//			if(std::find(closed_list.begin(), closed_list.end(), neighbor) != closed_list.end()) {
//				continue;
//			}
//			if(std::find(open_list.begin(), open_list.end(), neighbor) == open_list.end()) {
//				open_list.push_back(neighbor);
//			}
//			int g_score = current_tile->g_score + 1;
//			if(g_score < neighbor->g_score) {
//				neighbor->g_score = g_score;
//				neighbor->parent = current_tile;
//			}
//		}
//	}
//	
//	return route;
//}



QColor get_color_for_tile(terrain_type_e type) {
	switch(type) {
	case TERRAIN_WATER: return QColor(20, 85, 170);
	case TERRAIN_GRASS: return QColor(25, 95, 15);
	case TERRAIN_DIRT: return QColor(115, 60, 20);
	case TERRAIN_WASTELAND: return QColor(185, 100, 25);
	case TERRAIN_LAVA: return QColor(40, 40, 40);
	case TERRAIN_DESERT: return QColor(180, 140, 25);
	case TERRAIN_BEACH: return QColor(200, 160, 125);
	case TERRAIN_SNOW: return QColor(200, 200, 200);
	case TERRAIN_SWAMP: return QColor(30, 65, 50);

	default: return QColor(100, 100, 100);
	}
}

//todo: move this

void draw_minimap(QImage& image, uint pixel_size, const adventure_map_t& map, const game_t* game, player_e player, const std::string& path_prefix, bool reveal_map) {
	auto mapsize = pixel_size;
	if(!map.width || !map.height)
		return;

	assert(std::max(map.width, map.height));

	auto scale = std::max(4u, mapsize / std::max(map.width, map.height));

	QImage terrain_img(map.width, map.height, QImage::Format_RGBA8888);
	for(int y = 0; y < map.height; y++) {
		for(int x = 0; x < map.width; x++) {
			auto tile_type = map.tiles[y*map.width + x].terrain_type;
			auto color = get_color_for_tile(tile_type);

			if(!map.get_passability(x, y)) {
				float factor = .7f;
				color = QColor(color.red() * factor, color.green() * factor, color.blue() * factor);
			}
			terrain_img.setPixelColor(x, y, color);
		}
	}
	
	QImage fog_img(map.width, map.height, QImage::Format_RGBA8888);
	for(int y = 0; y < map.height; y++) {
		for(int x = 0; x < map.width; x++) {
			if(reveal_map) {
				fog_img.setPixelColor(x, y, QColor(0, 0, 0, 0));
				continue;
			}

			if(game && !game->is_tile_visible(x, y, player))
				fog_img.setPixelColor(x, y, QColor(0, 0, 0, 255));
			else if(game && !game->is_tile_observable(x, y, player))
				fog_img.setPixelColor(x, y, QColor(0, 0, 0, 80));
			else
				fog_img.setPixelColor(x, y, QColor(0, 0, 0, 0));
		}
	}

	image = map.width > map.height ? terrain_img.scaledToWidth(mapsize) : terrain_img.scaledToHeight(mapsize);
	QPainter painter1(&image);
	
	const QString prefix = QString::fromStdString(path_prefix);
	static auto town_color = QImage(prefix + "assets/castle2.png");
	static auto town_outline = QImage(prefix + "assets/castle_outline2.png");
	static auto hero_color = QImage(prefix + "assets/hero_star.png");
	static auto hero_outline = QImage(prefix + "assets/hero_star_outline.png");
	static auto mine_color = QImage(prefix + "assets/mine_color.png");
	static auto mine_outline = QImage(prefix + "assets/mine_outline.png");
	
	QColor neutral_color(230, 230, 230);
	
	//cached images for color variations
	QMap<player_color_e, QImage> colored_town_images;
	
	for(auto obj : map.objects) {
		if(!obj)
			continue;
		
		if(obj->object_type == OBJECT_MAP_TOWN) {
			auto town = (town_t*)obj;
			
			if(game && !game->is_tile_visible(town->x, town->y, player))
				continue;
			
			float sz = scale * 5;
			auto outline = town_outline.scaledToWidth(sz, Qt::SmoothTransformation);
			auto icon = town_color.scaledToWidth(sz, Qt::SmoothTransformation);
			
			QColor color = neutral_color;
			if(town->player != PLAYER_NONE) {
				if(town->player < map.player_configurations.size())
					color = QColor(get_qcolor_for_player_color(map.player_configurations[town->player - 1].color));
			}
			//QPixmap pixmap = QPixmap::fromImage(icon);
			QImage colored = icon.copy();
			QPainter painter(&colored);
			painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
			painter.setBrush(color);
			painter.setPen(color);
			painter.drawRect(colored.rect());
			painter.end();
			auto xpos = ((town->x + (town->width / 2.f)) / (float)map.width) * mapsize;
			auto ypos = ((town->y + (town->height / 2.f)) / (float)map.height) * mapsize;
			
			auto rect = QRectF(xpos - sz/2.f, ypos - sz/2.f, sz, sz);
			painter1.drawImage(rect, outline);
			//painter1.drawImage(rect, icon);
			//painter1.drawPixmap(rect, pixmap, pixmap.rect());
			painter1.drawImage(rect, colored, colored.rect());
		}
		
		
		if(obj->object_type == OBJECT_MINE) {
			auto mine = (mine_t*)obj;
			
			if(game && !game->is_tile_visible(mine->x, mine->y, player))
				continue;
			
			
			float sz = scale * 4;
			auto outline = mine_outline.scaledToWidth(sz, Qt::SmoothTransformation);
			auto icon = mine_color.scaledToWidth(sz, Qt::SmoothTransformation);
			
			
			QColor color = neutral_color;
			if(mine->owner != PLAYER_NONE) {
				if(mine->owner < map.player_configurations.size())
					color = QColor(get_qcolor_for_player_color(map.player_configurations[mine->owner - 1].color));
			}
			//QPixmap pixmap = QPixmap::fromImage(icon);
			QImage colored = icon.copy();
			QPainter painter(&colored);
			painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
			painter.setBrush(color);
			painter.setPen(color);
			painter.drawRect(colored.rect());
			painter.end();
			
			//mine->width
			auto xpos = ((mine->x + (mine->width / 2.f)) / (float)map.width) * mapsize;
			auto ypos = ((mine->y + (mine->height / 2.f)) / (float)map.height) * mapsize;
			auto rect = QRectF(xpos - sz/2.f, ypos - sz/2.f, sz, sz);
			painter1.drawImage(rect, outline);
			//painter1.drawImage(rect, icon);
			//painter1.drawPixmap(rect, pixmap, pixmap.rect());
			painter1.drawImage(rect, colored, colored.rect());
		}
	}
	
	for(auto& h : map.heroes) {
		if(h.second.garrisoned)
			continue;
		
		if(game && !game->is_tile_visible(h.second.x, h.second.y, player))
			continue;
		
		auto outline = hero_outline.scaledToWidth(scale * 2, Qt::SmoothTransformation);
		auto icon = hero_color.scaledToWidth(scale * 2, Qt::SmoothTransformation);
		
		QColor color = neutral_color;
		if(h.second.player != PLAYER_NONE) {
			auto pc = map.player_configurations[h.second.player - 1];
			color = QColor(get_qcolor_for_player_color(pc.color));
		}
		QImage colored = icon.copy();
		QPainter painter(&colored);
		painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		painter.setBrush(color);
		painter.setPen(color);
		painter.drawRect(colored.rect());
		auto xpos = (h.second.x / (float)map.width) * mapsize;
		auto ypos = (h.second.y / (float)map.height) * mapsize;
		auto rect = QRectF(xpos - scale/2, ypos - scale/2, scale*2, scale*2);
		//painter1.drawImage(rect, icon);
		painter1.drawImage(rect, outline);
		painter1.drawImage(rect, colored, colored.rect());
	}
	
	
	//draw fog on top of all objects and terrain
	auto fog_overlay_image = map.width > map.height ? fog_img.scaledToWidth(mapsize) : fog_img.scaledToHeight(mapsize);
	
	painter1.setCompositionMode(QPainter::CompositionMode_SourceOver); // Set composition mode for alpha blending

	// Draw the fog overlay image on top of the terrain image
	painter1.drawImage(0, 0, fog_overlay_image);

	painter1.end();
}
