#include "core/town.h"
#include "core/magic_enum.hpp"
#include "core/game.h"
#include "core/qt_headers.h"

#include <random>
#include <algorithm>

QDataStream& operator<<(QDataStream &stream, const building_t& building) {
	stream << building.type;
	return stream;
}

QDataStream& operator>>(QDataStream &stream, building_t& building) {
	stream >> building.type;
	return stream;
}

void town_t::write_data(QDataStream& stream) const {
	stream << reveal_area << observation_area << town_type << player;
	stream_write_vector(stream, available_buildings, 32);
	stream_write_vector(stream, built_buildings, 32);
	stream << has_built_today;
	
	stream_write_vector(stream, allowed_spells);
	stream_write_vector(stream, mage_guild_spells);
	stream << has_researched_today;
	
	stream_write_string(stream, name, 32);
	stream_write_string(stream, title, 32);
	stream_write_string(stream, description, 256);
	stream_write_string(stream, town_history, 256);
	
	stream_write_vector(stream, available_troops);

	uint8_t garrisoned_hero_index = 0;
	stream << garrisoned_hero_index;
	
	for(auto& t : garrison_troops)
		stream << t;

	stream << icon;
	stream << town_id;
}

void town_t::read_data(QDataStream& stream) {
	stream >> reveal_area >> observation_area >> town_type >> player;
	stream_read_vector(stream, available_buildings, 32);
	stream_read_vector(stream, built_buildings, 32);
	stream >> has_built_today;
	
	stream_read_vector(stream, allowed_spells);
	stream_read_vector(stream, mage_guild_spells);
	stream >> has_researched_today;
	
	name = stream_read_string(stream, 32);
	title = stream_read_string(stream, 32);
	description = stream_read_string(stream, 256);
	town_history = stream_read_string(stream, 256);
	
	stream_read_vector(stream, available_troops);
	
	uint8_t garrisoned_hero_index = 0;
	stream >> garrisoned_hero_index;
	
	for(auto& t : garrison_troops)
		stream >> t;
	
	stream >> icon;
	stream >> town_id;
}

int town_t::read_data_json(QJsonObject json) {
	reveal_area = json["reveal_area"].toInt();
	observation_area = json["observation_area"].toInt();
	town_type = utils::get_enum_value<town_type_e>(json["town_type"].toString());
	player = utils::get_enum_value<player_e>(json["player"].toString());
	
	available_buildings.clear();
	QJsonArray availableBuildingsArray = json["available_buildings"].toArray();
	for (const auto& buildingValue : availableBuildingsArray) {
		available_buildings.push_back(utils::get_enum_value<building_e>(buildingValue.toString()));
	}
	
	built_buildings.clear();
	QJsonArray builtBuildingsArray = json["built_buildings"].toArray();
	for (const auto& buildingValue : builtBuildingsArray) {
		built_buildings.push_back(utils::get_enum_value<building_e>(buildingValue.toString()));
	}
	
	has_built_today = json["has_built_today"].toBool();
	
	allowed_spells.clear();
	QJsonArray allowedSpellsArray = json["allowed_spells"].toArray();
	for (const auto& spellValue : allowedSpellsArray) {
		allowed_spells.push_back(utils::get_enum_value<spell_e>(spellValue.toString()));
	}
	
	mage_guild_spells.clear();
	QJsonArray mageGuildSpellsArray = json["mage_guild_spells"].toArray();
	for (const auto& spellValue : mageGuildSpellsArray) {
		mage_guild_spells.push_back(utils::get_enum_value<spell_e>(spellValue.toString()));
	}
	
	has_researched_today = json["has_researched_today"].toBool();
	
	name = json["name"].toString().toStdString();
	title = json["title"].toString().toStdString();
	description = json["description"].toString().toStdString();
	town_history = json["town_history"].toString().toStdString();
	
	available_troops.clear();
	QJsonArray availableTroopsArray = json["available_troops"].toArray();
	for (const auto& troopValue : availableTroopsArray) {
		QJsonObject troopJson = troopValue.toObject();
		troop_t troop;
		troop.unit_type = utils::get_enum_value<unit_type_e>(troopJson["unit_type"].toString());
		troop.stack_size = troopJson["stack_size"].toInt();
		available_troops.push_back(troop);
	}
	
	// Assuming garrisoned_hero_index is always 0 and can be ignored in JSON
	
	int i = 0;
	QJsonArray garrisonTroopsArray = json["garrison_troops"].toArray();
	for(const auto& troopValue : garrisonTroopsArray) {
		if(i >= (int)garrison_troops.size())
			break;
		
		QJsonObject troopJson = troopValue.toObject();
		troop_t troop;
		troop.unit_type = utils::get_enum_value<unit_type_e>(troopJson["unit_type"].toString());
		troop.stack_size = troopJson["stack_size"].toInt();
		garrison_troops[i] = troop;
		i++;
	}
	
	icon = json["icon"].toInt();
	town_id = json["town_id"].toInt();
	
	return 0;
}

QJsonObject town_t::write_data_json() const {
	QJsonObject json;
	json["reveal_area"] = reveal_area;
	json["observation_area"] = observation_area;
	json["town_type"] = utils::name_from_enum(town_type);
	json["player"] = utils::name_from_enum(player);
	
	QJsonArray availableBuildingsArray;
	for (const auto& building : available_buildings) {
		availableBuildingsArray.append(utils::name_from_enum(building));
	}
	json["available_buildings"] = availableBuildingsArray;
	
	QJsonArray builtBuildingsArray;
	for (const auto& building : built_buildings) {
		builtBuildingsArray.append(utils::name_from_enum(building));
	}
	json["built_buildings"] = builtBuildingsArray;
	
	json["has_built_today"] = has_built_today;
	
	QJsonArray allowedSpellsArray;
	for (const auto& spell : allowed_spells) {
		allowedSpellsArray.append(utils::name_from_enum(spell));
	}
	json["allowed_spells"] = allowedSpellsArray;
	
	QJsonArray mageGuildSpellsArray;
	for (const auto& spell : mage_guild_spells) {
		mageGuildSpellsArray.append(utils::name_from_enum(spell));
	}
	json["mage_guild_spells"] = mageGuildSpellsArray;
	
	json["has_researched_today"] = has_researched_today;
	
	json["name"] = QString::fromStdString(name).toStdString().c_str();
	json["title"] = QString::fromStdString(title).toStdString().c_str();
	json["description"] = QString::fromStdString(description).toStdString().c_str();
	json["town_history"] = QString::fromStdString(town_history).toStdString().c_str();
	
	QJsonArray availableTroopsArray;
	for (const auto& troop : available_troops) {
		QJsonObject troopJson;
		troopJson["unit_type"] = utils::name_from_enum(troop.unit_type);
		troopJson["stack_size"] = troop.stack_size;
		availableTroopsArray.append(troopJson);
	}
	json["available_troops"] = availableTroopsArray;
	
	json["garrisoned_hero_index"] = 0; // Assuming garrisoned_hero_index is always 0
	
	QJsonArray garrisonTroopsArray;
	for (const auto& troop : garrison_troops) {
		QJsonObject troopJson;
		troopJson["unit_type"] = utils::name_from_enum(troop.unit_type);
		troopJson["stack_size"] = troop.stack_size;
		garrisonTroopsArray.append(troopJson);
	}
	json["garrison_troops"] = garrisonTroopsArray;
	
	json["icon"] = icon;
	json["town_id"] = town_id;
	
	return json;
}

void town_t::new_day(int day) {
	has_built_today = false;
	if(day % 7 == 0) {
		//generate_creatures();
	}
}

void town_t::new_week(bool affected_by_freelancer, bool affected_by_call_to_arms) {
	for(auto b : built_buildings) {
		const auto& building = game_config::get_building(b);
		if(building.generated_creature == UNIT_UNKNOWN)
			continue;
		
		const auto& cr = game_config::get_creature(building.generated_creature);
		int growth = building.weekly_growth;
		if(affected_by_freelancer) {
			if(cr.tier == 1 || cr.tier == 2)
				growth = (int)std::round(growth * 1.15f);
		}
		if(affected_by_call_to_arms) {
			if(cr.tier == 1)
				growth += 5;
			else if(cr.tier == 2)
				growth += 3;
			else if(cr.tier == 3)
				growth += 2;
		}
		
		bool added = false;
		for(auto& atr : available_troops) {
			if(atr.unit_type == cr.unit_type) {
				atr.stack_size += growth;
				added = true;
			}
		}
		
		if(!added)
			available_troops.push_back(troop_t(cr.unit_type, growth));
	}
	
}

resource_group_t town_t::get_daily_income(int day) {	
	resource_group_t income;
	income.set_value_for_type(RESOURCE_GOLD, 1000);
	
	return income;
}

bool town_t::is_guarded() const {
	if(garrisoned_hero)
		return true;

	if(visiting_hero)
		return true;

	for(const auto& tr : garrison_troops) {
		if(!tr.is_empty())
			return true;
	}

	return false;
}

bool town_t::will_visiting_hero_defend_in_castle() const {
	if(!visiting_hero)
		return false;

	//if we have a garrisoned hero, the visiting hero will fight outside
	if(garrisoned_hero)
		return false;

	return true;
}

int town_t::get_castle_level() const {
	if(is_building_built(BUILDING_TURRET_RIGHT))
		return 5;
	else if(is_building_built(BUILDING_TURRET_LEFT))
		return 4;
	else if(is_building_built(BUILDING_CAPTAINS_QUARTERS))
		return 3;
	else if(is_building_built(BUILDING_CASTLE))
		return 2;
	else if(is_building_built(BUILDING_FORT))
		return 1;

	return 0;
}

float town_t::get_turret_damage_multiplier() const {
	float multi = 1.0f;

	for(const auto bb : built_buildings) {
		if(bb == BUILDING_CASTLE)
			multi += .5f;
		else if(bb == BUILDING_CAPTAINS_QUARTERS)
			multi += .3f;
		else
			multi += .1f;
	}

	return multi;
}

void town_t::setup_default_spells() {
	for(auto& spell : game_config::get_spells()) {
		if(spell.school == SCHOOL_WARCRY) {
			if(town_type == TOWN_BARBARIAN)
				allowed_spells.push_back(spell.id);

			continue;
		}

		if(spell.level < 3) {
			allowed_spells.push_back(spell.id);
			continue;
		}

		switch(town_type) {
			case TOWN_KNIGHT:
				if(spell.school != SCHOOL_HOLY)
					continue;
				break;
			case TOWN_SORCERESS:
				if(spell.school != SCHOOL_NATURE)
					continue;
				break;
			case TOWN_WIZARD:
				if(spell.school == SCHOOL_DEATH || spell.school == SCHOOL_DESTRUCTION)
					continue;
				break;
			case TOWN_WARLOCK:
				if(spell.school == SCHOOL_NATURE || spell.school == SCHOOL_HOLY)
					continue;
				break;
			case TOWN_NECROMANCER:
				if(spell.school != SCHOOL_DEATH && spell.school != SCHOOL_DESTRUCTION)
					continue;
				break;
			case TOWN_BARBARIAN:
			case TOWN_CUSTOM:
			//case TOWN_NEUTRAL:
			case TOWN_UNKNOWN:
				break;
		}
		
		allowed_spells.push_back(spell.id);
	}	
}

bool town_t::is_building_built(building_e building_type) const {
	for(auto& b : built_buildings) {
		if(b == building_type)
			return true;
	}
	
	return false;
}

bool town_t::is_building_enabled(building_e building_type) const {
	for(auto& b : available_buildings) {
		if(b == building_type)
			return true;
	}
	
	return false;
}

int town_t::get_mage_guild_level() const {
	int maxlevel = 0;

	for(const auto building_type : built_buildings) {
		if(building_type == BUILDING_MAGE_GUILD_1)
			maxlevel = std::max(maxlevel, 1);
		else if(building_type == BUILDING_MAGE_GUILD_2)
			maxlevel = std::max(maxlevel, 2);
		else if(building_type == BUILDING_MAGE_GUILD_3)
			maxlevel = std::max(maxlevel, 3);
		else if(building_type == BUILDING_MAGE_GUILD_4)
			maxlevel = std::max(maxlevel, 4);
		else if(building_type == BUILDING_MAGE_GUILD_5)
			maxlevel = std::max(maxlevel, 5);
	}

	return maxlevel;
}

bool town_t::are_building_prerequisites_satisfied(building_e building) const {
	auto& b = game_config::get_building(building);
	for(auto pre : b.prerequisites) {
		if(pre != BUILDING_NONE && !is_building_built(pre))
			return false;
	}
	
	return true;
}

bool town_t::can_build_building(building_e building_type, const game_t& gamestate) const {
	if(has_built_today || !is_building_enabled(building_type))
		return false;
	
	//todo
	auto& b = game_config::get_building(building_type);
	
	if(!gamestate.get_player(player).resources.covers_cost(b.cost))
		return false;

	if(!are_building_prerequisites_satisfied(building_type))
		return false;
	
	return true;
}

bool town_t::build_building(building_e building_type, game_t& gamestate) {
	if(!is_building_enabled(building_type) || !can_build_building(building_type, gamestate))
		return false;
	
	//todo
	auto& b = game_config::get_building(building_type);
	
	if(!gamestate.get_player(player).resources.covers_cost(b.cost))
		return false;

	
	gamestate.get_player(player).resources -= b.cost;
	
	built_buildings.push_back(building_type);
	
	has_built_today = true;
	
	if(building_type == BUILDING_MAGE_GUILD_1)
		populate_available_spells(1);
	else if(building_type == BUILDING_MAGE_GUILD_2)
		populate_available_spells(2);
	else if(building_type == BUILDING_MAGE_GUILD_3)
		populate_available_spells(3);
	else if(building_type == BUILDING_MAGE_GUILD_4)
		populate_available_spells(4);
	else if(building_type == BUILDING_MAGE_GUILD_5)
		populate_available_spells(5);
	
	if(b.generated_creature != UNIT_UNKNOWN) {
		auto& cr = game_config::get_creature(b.generated_creature);
		if(cr.tier != 0) {
			troop_t tr(b.generated_creature);
			tr.stack_size = b.weekly_growth / 2;
			available_troops.push_back(tr);
		}
	}
	
	return true;
}

hero_class_e town_t::town_type_to_hero_class(town_type_e town_type) {
	switch(town_type) {
		case TOWN_UNKNOWN: return HERO_CUSTOM;
		//case TOWN_NEUTRAL: return HERO_CUSTOM;
		case TOWN_CUSTOM: return HERO_CUSTOM;
		case TOWN_KNIGHT: return HERO_KNIGHT;
		case TOWN_BARBARIAN: return HERO_BARBARIAN;
		case TOWN_WIZARD: return HERO_WIZARD;
		case TOWN_WARLOCK: return HERO_WARLOCK;
		case TOWN_NECROMANCER: return HERO_NECROMANCER;
		case TOWN_SORCERESS: return HERO_SORCERESS;
	}
	
	return HERO_CUSTOM;
}

town_type_e town_t::hero_class_to_town_type(hero_class_e hero_class) {
	switch(hero_class) {
		case HERO_KNIGHT: return TOWN_KNIGHT;
		case HERO_BARBARIAN: return TOWN_BARBARIAN;
		case HERO_WIZARD: return TOWN_WIZARD;
		case HERO_WARLOCK: return TOWN_WARLOCK;
		case HERO_NECROMANCER: return TOWN_NECROMANCER;
		case HERO_SORCERESS: return TOWN_SORCERESS;
		case HERO_CUSTOM:
		default: return TOWN_CUSTOM;
	}
	
}

void town_t::populate_available_spells(uint level) {
	std::vector<spell_e> spells;

	for(auto spell_id : allowed_spells) {
		if(spell_id == SPELL_UNKNOWN)
			continue;

		const auto& spell = game_config::get_spell(spell_id);
		if(spell.level != level)
			continue;
		
		if(!utils::contains(spells, spell_id))
			spells.push_back(spell_id);
	}

	std::random_device rd;
	std::mt19937 g(rd());

	// Shuffle spells
	std::shuffle(spells.begin(), spells.end(), g);
	
	int count = 6 - level;
	// Select required number of spells for each tier
	if(spells.size() < count)
		return;
	
	int i = 0;
	for(auto sp : spells) {
		if(i++ >= count)
			break;

		mage_guild_spells.push_back(sp);
	}
}


troop_t* town_t::get_available_troop_by_type(unit_type_e unit_type) {
	for(auto& tr : available_troops) {
		if(tr.unit_type == unit_type)
			return &tr;
	}
	
	return nullptr;
}

building_e get_generator_for_town_type(town_type_e type, uint tier) {
	for(const auto& b : game_config::get_buildings()) {
		if(b.generated_creature == UNIT_UNKNOWN)
			continue;
		
		const auto& cr = game_config::get_creature(b.generated_creature);
		if(cr.tier != tier || cr.faction != town_t::town_type_to_hero_class(type))
			continue;
		
		return b.type;
	}
	
	return BUILDING_NONE;
}

void town_t::setup_buildings() {
	//std::erase(available_buildings, BUILDING_SHIPYARD);//std::remove(available_buildings.begin(), available_buildings.end(), BUILDING_SHIPYARD);
	
	if(!built_buildings.size()) {
		built_buildings.push_back(BUILDING_FORT);
		built_buildings.push_back(BUILDING_TAVERN);
		built_buildings.push_back(get_generator_for_town_type(town_type, 1));
		if(utils::rand_chance(50))
			built_buildings.push_back(get_generator_for_town_type(town_type, 2));
	}
	
	for(auto b : built_buildings) {
		const auto& building = game_config::get_building(b);
		if(building.generated_creature != UNIT_UNKNOWN)
			available_troops.push_back(troop_t(building.generated_creature, building.weekly_growth / 2));
		
		if(building.type == BUILDING_MAGE_GUILD_1)
			populate_available_spells(1);
		if(building.type == BUILDING_MAGE_GUILD_2)
			populate_available_spells(2);
		if(building.type == BUILDING_MAGE_GUILD_3)
			populate_available_spells(3);
		if(building.type == BUILDING_MAGE_GUILD_4)
			populate_available_spells(4);
		if(building.type == BUILDING_MAGE_GUILD_5)
			populate_available_spells(5);
	}
	
	if(!available_buildings.size()) {
		magic_enum::enum_for_each<building_e>([this] (auto val) {
			constexpr building_e type = val;
			//auto name = magic_enum::enum_name(type).data();
	//		building_t b;
	//		b.type = type;
			auto& building = game_config::get_building(type);
			bool match = false;

			if(type == BUILDING_SHIPYARD) //todo: implement me
				return;

			auto hc = town_type_to_hero_class(town_type);
			if(building.matching_faction == HERO_CLASS_ALL)
				available_buildings.push_back(type);
			else if(building.matching_faction == hc)
				available_buildings.push_back(type);
			else if(building.matching_faction == HERO_CLASS_NON_NECRO && hc != HERO_NECROMANCER)
				available_buildings.push_back(type);
			else if(building.matching_faction == HERO_CLASS_NON_BARBARIAN && hc != HERO_BARBARIAN)
				available_buildings.push_back(type);
		});
	}

	//we shouldn't have duplicate spells in allowed_spells, but just in case we do, remove them
	std::sort(allowed_spells.begin(), allowed_spells.end(), [](const spell_e lhs, const spell_e rhs) {
		return (int)lhs < (int)rhs;
	});
	auto it = std::unique(allowed_spells.begin(), allowed_spells.end());
	allowed_spells.erase(it, allowed_spells.end());
}

std::string building_t::get_name(building_e type) {
	return "TODO";
}

std::string building_t::get_description(building_e type) {
	return "TODO";
}

building_e building_t::get_parent_building(building_e type) {
	if(type == BUILDING_MAGE_GUILD_5)
		return BUILDING_MAGE_GUILD_4;
	if(type == BUILDING_MAGE_GUILD_4)
		return BUILDING_MAGE_GUILD_3;
	if(type == BUILDING_MAGE_GUILD_3)
		return BUILDING_MAGE_GUILD_2;
	if(type == BUILDING_MAGE_GUILD_2)
		return BUILDING_MAGE_GUILD_1;
	
	if(type == BUILDING_CASTLE)
		return BUILDING_FORT;
	
	if(type == BUILDING_AUCTION_HOUSE)
		return BUILDING_MARKETPLACE;
	
	if(type == BUILDING_LIGHTHOUSE)
		return BUILDING_SHIPYARD;
	
	return BUILDING_NONE;
}


//std::vector<building_t> building_t::get_prerequisites(building_e type) {
//	std::vector<building_t> reqs;
//	
//	if(type == BUILDING_MAGE_GUILD_2)
//		reqs.push_back(building_t(BUILDING_MAGE_GUILD_1));
//	
//	return reqs;
//}

resource_group_t building_t::get_cost(building_e type) {
	resource_group_t res;
	res.set_value_for_type(RESOURCE_GOLD, 2000);
	
	return res;
}

