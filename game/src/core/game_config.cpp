#include "core/game_config.h"
#include "core/creature.h"
#include "core/hero.h"
#include "core/town.h"
#include "core/utils.h"
#include "core/game.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>

#include "core/qt_headers.h"

std::vector<object_info_t> game_config::objects;
std::vector<creature_t> game_config::creatures;
std::vector<talent_t> game_config::talents;
std::vector<artifact_t> game_config::artifacts;
std::vector<skill_t> game_config::skills;
std::vector<hero_specialty_t> game_config::specialties;
std::vector<building_t> game_config::buildings;
std::vector<spell_t> game_config::spells;
std::vector<hero_t> game_config::heroes;
std::vector<buff_info_t> game_config::buff_info;
std::vector<std::pair<town_type_e, std::string>> game_config::town_names;

int game_config::load_game_data(const std::string& path_prefix) {
	objects.clear();
	creatures.clear();
	talents.clear();
	skills.clear();
	spells.clear();
	artifacts.clear();
	heroes.clear();
	buildings.clear();
	buff_info.clear();
	town_names.clear();
	specialties.clear();

	game_config::load_object_data(path_prefix);
	game_config::load_creatures(path_prefix);
	game_config::load_talents(path_prefix);
	game_config::load_skills(path_prefix);
	game_config::load_spells(path_prefix);
	game_config::load_artifacts(path_prefix);
	game_config::load_heroes(path_prefix);
	game_config::load_buildings(path_prefix);
	game_config::load_buffs(path_prefix);
	game_config::load_town_names(path_prefix);
	game_config::load_specialties(path_prefix);
	
	return 0;
}

using utils::get_enum_value;

template<typename T> T fill_multiplier(const QString& string) {
	T mult;

	QString str = string;
	auto plus = str.split('+');
	if(plus.count() == 2) {
		mult.fixed = plus.first().toInt();
		str = plus[1];
	}
	auto multiplier = str.split('*');
	if(multiplier.count() == 2) {
		mult.is_power_multiplied = 1;
		mult.multi = multiplier.first().toInt();
		return mult;
	}
	if(str == "Power" || str == "Level") {
		mult.multi = 1;
		mult.is_power_multiplied = 1;
	}
	else {
		mult.multi = str.toInt();
	}

	return mult;
}

int game_config::load_creatures(const std::string& path_prefix) {
	creatures.push_back(creature_t()); //dummy for 'unknown'/none //fixme
	QString creatures_file = QString::fromStdString(path_prefix) + "config/creatures.tsv";
	QFile file(creatures_file);
	if(!file.open(QFile::ReadOnly)) {
		std::cerr << "Could not open heroes file: " << file.errorString().toStdString() << std::endl;
		return -1;
	}
	
	QTextStream f(&file);
	
	while(!f.atEnd()) {
		auto line = f.readLine(4096);
		auto split = line.split('\t');
		if(split.length() != 17) //we got a problem
			continue;
		
		int pos = 0;
		creature_t creature;
		
		creature.unit_type = get_enum_value<unit_type_e>(split[pos++]);
		creature.asset_id_portrait = split[pos++].toUShort();
		creature.name = split[pos++].toStdString();
		creature.name_plural = split[pos++].toStdString();
		creature.faction = get_enum_value<hero_class_e>(split[pos++]);
		creature.tier = split[pos++].toUShort();
		creature.reanimates_as_unit_type = get_enum_value<unit_type_e>(split[pos++]);
		creature.two_hex = (split[pos++] == "1");
		creature.health = split[pos++].toUInt();
		creature.min_damage = split[pos++].toUInt();
		creature.max_damage = split[pos++].toUInt();
		creature.attack = split[pos++].toUInt();
		creature.defense = split[pos++].toUInt();
		creature.speed = split[pos++].toUInt();
		creature.initiative = split[pos++].toUInt();
		creature.cost.set_value_for_type(RESOURCE_GOLD, split[pos++].toUInt()); //fixme, allow other resources
		
		auto buffs = split[pos++].split(',');
		uint n = 0;
		for(auto& b : buffs) {
			if(b.isEmpty() || n >= game_config::MAX_INHERENT_BUFFS)
				break;
			
			creature.inherent_buffs[n++] = get_enum_value<buff_e>(b.trimmed());
		}
		
		creatures.push_back(creature);
	}
	
	return 0;
}

int game_config::load_object_data(const std::string& path_prefix) {
	QString objects_file = QString::fromStdString(path_prefix) + "config/objects_NEW.tsv";
	QFile file(objects_file);
	if(!file.open(QFile::ReadOnly)) {
		std::cerr << "Could not open objects file: " << file.errorString().toStdString() << std::endl;
		return -1;
	}
	
	QTextStream f(&file);
	
	while(!f.atEnd()) {
		auto line = f.readLine(1024);
		auto split = line.split('\t');
		if(split.length() != 10) //we got a problem
			continue;
		
		//more validation needed
		int pos = 0;
		object_info_t object_info;

		object_info.asset_id = split[pos++].toInt();
		object_info.name = split[pos++].toStdString();
		object_info.filename = split[pos++].toStdString();
		object_info.object_metatype = get_enum_value<object_metatype_e>(split[pos++]);
		object_info.editor_brush_category = get_enum_value<editor_brush_category_e>(split[pos++]);
		
		QString object_subtype_str = split[pos++];
		if(object_info.object_metatype == OBJECT_METATYPE_TERRAIN)
			object_info.terrain_type = get_enum_value<terrain_type_e>(object_subtype_str);
		else
			object_info.interactable_object_type = get_enum_value<interactable_object_e>(object_subtype_str);


		object_info.center_x = split[pos++].toUInt();
		object_info.center_y = split[pos++].toUInt();
		object_info.passability = split[pos++].toULongLong();
		object_info.interactability = split[pos++].toULongLong();

		if(object_info.object_metatype == OBJECT_METATYPE_INTERACTABLE)
			adventure_map_t::interactivity_map[object_info.asset_id] = object_info.interactability;


		objects.push_back(object_info);
	}

	return 0;
}

int game_config::load_buffs(const std::string& path_prefix) {
	//.push_back(creature_t()); //dummy for 'unknown'/none //fixme
	QString buffs_file = QString::fromStdString(path_prefix) + "config/buffs.tsv";
	QFile file(buffs_file);
	if(!file.open(QFile::ReadOnly)) {
		std::cerr << "Could not open buffs file: " << file.errorString().toStdString() << std::endl;
		return -1;
	}
	
	QTextStream f(&file);
	
	while(!f.atEnd()) {
		auto line = f.readLine(512);
		auto split = line.split('\t');
		if(split.length() != 6) //we got a problem
			continue;
		
		int pos = 0;
		buff_info_t buff;
		
		buff.buff_id = get_enum_value<buff_e>(split[pos++]);
		buff.asset_id = split[pos++].toUInt();
		/*buff.color*/auto color = split[pos++].toStdString();
		buff.type = get_enum_value<buff_type_e>(split[pos++]);
		buff.name = split[pos++].toStdString();
		buff.description = split[pos++].toStdString();
		
		buff_info.push_back(buff);
	}
	
	return 0;
}


int game_config::load_specialties(const std::string& path_prefix) {
	//.push_back(creature_t()); //dummy for 'unknown'/none //fixme
	QString specialties_file = QString::fromStdString(path_prefix) + "config/specialties.tsv";
	QFile file(specialties_file);
	if(!file.open(QFile::ReadOnly)) {
		std::cerr << "Could not open specialties file: " << file.errorString().toStdString() << std::endl;
		return -1;
	}
	
	QTextStream f(&file);
	
	while(!f.atEnd()) {
		auto line = f.readLine(512);
		auto split = line.split('\t');
		if(split.length() != 6) //we got a problem
			continue;
		
		int pos = 0;
		hero_specialty_t specialty;
		
		specialty.id = get_enum_value<hero_specialty_e>(split[pos++]);
		specialty.name = split[pos++].toStdString();
		specialty.description = split[pos++].toStdString();
		specialty.multiplier = fill_multiplier<specialty_multiplier_t>(split[pos++]);
		specialty.asset_string = split[pos++].toStdString();
		
		auto classes = split[pos++].split(',');
		for(auto& cl : classes)
			specialty.allowed_classes.push_back(get_enum_value<hero_class_e>(cl.trimmed()));
		
		
		specialties.push_back(specialty);
	}
	
	return 0;
}


int game_config::load_town_names(const std::string& path_prefix) {
	QString town_names_file = QString::fromStdString(path_prefix) + "config/town_names.tsv";
	QFile file(town_names_file);
	if(!file.open(QFile::ReadOnly)) {
		std::cerr << "Could not open spells file: " << file.errorString().toStdString() << std::endl;
		return -1;
	}
	
	QTextStream f(&file);
	
	while(!f.atEnd()) {
		auto line = f.readLine(256);
		auto split = line.split('\t');
		if(split.length() != 2) //we got a problem
			continue;
		
		auto town_type = get_enum_value<town_type_e>(split[0]);
		auto name = split[1].toStdString();
		town_names.push_back(std::make_pair(town_type, name));
	}
	
	return 0;
}

int game_config::load_heroes(const std::string& path_prefix) {
	QString heroes_file = QString::fromStdString(path_prefix) + "config/heroes.tsv";
	QFile file(heroes_file);
	if(!file.open(QFile::ReadOnly)) {
		std::cerr << "Could not open heroes file: " << file.errorString().toStdString() << std::endl;
		return -1;
	}
	
	QTextStream f(&file);
	
	while(!f.atEnd()) {
		auto line = f.readLine(4096);
		auto split = line.split('\t');
		if(split.length() != 22) //we got a problem
			continue;
		
		int pos = 0;
		hero_t hero;

		hero.name = split[pos++].toStdString();
		hero.portrait = split[pos++].toInt();
		hero.hero_class = get_enum_value<hero_class_e>(split[pos++]);
		hero.biography = split[pos++].toStdString();
		hero.specialty = get_enum_value<hero_specialty_e>(split[pos++]);
		auto stats = split[pos++].split(',');
		if(stats.length() != 4)
			return 2;
		
		hero.attack = stats[0].toInt();
		hero.defense = stats[1].toInt();
		hero.power = stats[2].toInt();
		hero.knowledge = stats[3].toInt();
		
		auto sks = split[pos++].split(','); //skills
		uint n = 0;
		for(auto& s : sks) {
			if(n >= game_config::HERO_SKILL_SLOTS)
				break;
			
			hero.skills[n].skill = get_enum_value<skill_e>(s.trimmed());
			hero.skills[n].level = 1;
			n++;
		}
		
		auto sps = split[pos++].split(','); //spells
		n = 0;
		for(auto& s : sps)
			hero.spellbook.push_back(get_enum_value<spell_e>(s.trimmed()));
		
		auto gtalents = split[pos++].split(',');
		for(auto& t : gtalents) {
			auto talent = get_enum_value<talent_e>(t.trimmed());
			if(talent != TALENT_NONE)
				hero.guaranteed_talents.push_back(talent);
		}
		
		auto starting_army = split[pos++].split(';');
		for(uint i = 0; i < 3; i++) {
			if(i >= starting_army.size()) {
				std::get<0>(hero.starting_army[i]) = UNIT_UNKNOWN;
				std::get<1>(hero.starting_army[i]) = 0;
				std::get<2>(hero.starting_army[i]) = 0;
				continue;
			}
			auto ainfo = starting_army[i].split(',');
			if(ainfo.size() != 3)
				continue;
			
			std::get<0>(hero.starting_army[i]) = get_enum_value<unit_type_e>(ainfo[0].trimmed());
			std::get<1>(hero.starting_army[i]) = ainfo[1].trimmed().toUShort();
			std::get<2>(hero.starting_army[i]) = ainfo[2].trimmed().toUShort();
			
		}

		hero.gender = (split[pos++] == "Male") ? HERO_GENDER_MALE : HERO_GENDER_FEMALE;
		hero.race = split[pos++].toStdString();
		hero.appereance.class_appearance = get_enum_value<hero_class_appearance_e>(split[pos++]);
		hero.appereance.body = get_enum_value<hero_body_appearance_e>(split[pos++]);
		hero.appereance.face = get_enum_value<hero_face_appearance_e>(split[pos++]);
		hero.appereance.eye_color = get_enum_value<hero_eyes_appearance_e>(split[pos++]);
		hero.appereance.beard = get_enum_value<hero_beard_appearance_e>(split[pos++]);
		hero.appereance.eyebrows = get_enum_value<hero_eyebrow_appearance_e>(split[pos++]);
		hero.appereance.ears = get_enum_value<hero_ears_appearance_e>(split[pos++]);
		hero.appereance.skin = get_enum_value<hero_skin_appearance_e>(split[pos++]);
		hero.appereance.hair = get_enum_value<hero_hair_appearance_e>(split[pos++]);
		hero.appereance.hair_color = get_enum_value<hero_hair_color_appearance_e>(split[pos++]);
		
		hero.stat_distribution_pre10 = hero_t::get_stat_distribution_for_class(hero.hero_class, 1);
		hero.stat_distribution_post10 = hero_t::get_stat_distribution_for_class(hero.hero_class, 20);
		
		heroes.push_back(hero);
	}
	
	return 0;
}

int game_config::load_spells(const std::string& path_prefix) {
	QString spells_file = QString::fromStdString(path_prefix) + "config/spells.tsv";
	QFile file(spells_file);
	if(!file.open(QFile::ReadOnly)) {
		std::cerr << "Could not open spells file: " << file.errorString().toStdString() << std::endl;
		return -1;
	}

	QTextStream f(&file);

	while(!f.atEnd()) {
		spell_t spell;

		auto line = f.readLine(2048);
		auto split = line.split('\t');
		if(split.length() != 13) //we got a problem
			continue;

		//more validation needed
		int pos = 0;
		
		spell.school = get_enum_value<spell_school_e>(split[pos++]);
		spell.name = split[pos++].toStdString();
		spell.asset_id = split[pos++].toInt();
		spell.level = split[pos++].toInt();
		spell.id = get_enum_value<spell_e>(split[pos++]);

		if(spell.id == SPELL_UNKNOWN)
			continue;

		auto description = split[pos++];
		spell.description = description.toStdString();
		auto damage_types = split[pos++].split(',');
		if(damage_types.count() == 1) {
			if(damage_types[0].trimmed() == "MAGIC_DAMAGE_HEALING")
				spell.damage_type = MAGIC_DAMAGE_HEALING;
			else
				spell.damage_type = get_enum_value<magic_damage_e>(damage_types[0]);
		}
		else {
			spell.damage_type = get_enum_value<magic_damage_e>(damage_types[0]);
			if(damage_types[1].trimmed() == "MAGIC_DAMAGE_HEALING")
				spell.damage_type = (magic_damage_e)((int)spell.damage_type | (int)MAGIC_DAMAGE_HEALING);
		}

		spell.mana_cost = split[pos++].toInt();
		spell.multiplier[0] = fill_multiplier<spell_multiplier_t>(split[pos++]);
		spell.multiplier[1] = fill_multiplier<spell_multiplier_t>(split[pos++]);
		spell.multiplier[2] = fill_multiplier<spell_multiplier_t>(split[pos++]);
		//spell.targets =
		
		QRegularExpression re("\\{([^\\}]+)\\}");
		auto results = re.globalMatch(description);
		int p = 0;
		for(auto& res : results) {
			if(p > 2)
				break;
			
			spell.multiplier[p++].is_damage = res.captured(1).contains('d', Qt::CaseInsensitive);
		}

		spell.spell_type = get_enum_value<spell_type_e>(split[pos++]);
		spell.target = get_enum_value<spell_target_e>(split[pos++]);

		spells.push_back(spell);
	}

	return 0;
}

int game_config::load_artifacts(const std::string& path_prefix) {
	QString artifacts_file = QString::fromStdString(path_prefix) + "config/artifacts.tsv";
	QFile file(artifacts_file);
	if(!file.open(QFile::ReadOnly)) {
		std::cerr << "Could not open artifacts file: " << file.errorString().toStdString() << std::endl;
		return -1;
	}

	QTextStream f(&file);

	while(!f.atEnd()) {
		artifact_t artifact;

		auto line = f.readLine(4096);
		auto split = line.split('\t');
		if(split.length() != 10) //we got a problem
			continue;

		//more validation needed
		int pos = 0;
		artifact.asset_id = split[pos++].toInt();
		
		auto str = split[pos++].toStdString();
		auto id = magic_enum::enum_cast<artifact_e>(str);
		if(!id.has_value()) {
			if(str == "ARTIFACT_SPELL_SCROLL")
				id = ARTIFACT_SPELL_SCROLL;
			else
				continue;
		}
		
		artifact.id = id.value();
		
		str = split[pos++].toStdString();
		auto slot = magic_enum::enum_cast<artifact_slot_e>(str);
		if(!slot.has_value())
			continue;
		
		artifact.slot = slot.value();
		
		str = split[pos++].toStdString();
		auto rarity = magic_enum::enum_cast<artifact_rarity_e>(str);
		if(!rarity.has_value())
			continue;
		
		artifact.rarity = rarity.value();
		
		artifact.name = split[pos++].toStdString();
		artifact.description = split[pos++].toStdString();
		
		auto effects = split[pos++].split(';');
		for(auto& e : effects) {
			auto einfo = e.trimmed().split(',');
			if(einfo.size() < 1)
				continue;
			
			artifact_effect_t effect;
			effect.type = get_enum_value<artifact_effect_e>(einfo[0].trimmed());

			if(effect.type == EFFECT_SPELL_CAST_ON_BATTLE_START) {
				if(einfo.size() > 1)
					effect.property1.spell = get_enum_value<spell_e>(einfo[1].trimmed());
				if(einfo.size() > 2)
					effect.magnitude_1 = einfo[2].trimmed().toInt();
				if(einfo.size() > 3)
					effect.magnitude_2 = einfo[3].trimmed().toInt();
			}
			else if(effect.type == EFFECT_RESOURCE_GENERATION) {
				if(einfo.size() > 1)
					effect.property1.resource = get_enum_value<resource_e>(einfo[1].trimmed());
				if(einfo.size() > 2)
					effect.magnitude_1 = einfo[2].trimmed().toInt();
				if(einfo.size() > 3)
					effect.property2.frequency = get_enum_value<frequency_e>(einfo[3].trimmed());
			}
			else if(effect.type == EFFECT_INCREASED_SPELL_EFFECTIVENESS) {
				if(einfo.size() > 1)
					effect.property1.spell_school = get_enum_value<spell_school_e>(einfo[1].trimmed());
				if(einfo.size() > 2)
					effect.magnitude_1 = einfo[2].trimmed().toInt();
			}
			else {
				if(einfo.size() > 1)
					effect.magnitude_1 = einfo[1].trimmed().toInt();
				if(einfo.size() > 2)
					effect.magnitude_2 = einfo[2].trimmed().toInt();
			}

			artifact.effects.push_back(effect);
		}
		
		
		artifact.flavor = split[pos++].toStdString();
		artifact.pickup_story = split[pos++].toStdString();
		artifact.inventory_sound = get_enum_value<artifact_inventory_sound_e>(split[pos++].trimmed());
		
		artifacts.push_back(artifact);
	}
	
	return 0;
}


int game_config::load_buildings(const std::string& path_prefix) {
	QString buildings_file = QString::fromStdString(path_prefix) + "config/buildings.tsv";
	QFile file(buildings_file);
	if(!file.open(QFile::ReadOnly)) {
		std::cerr << "Could not open buildings file: " << file.errorString().toStdString() << std::endl;
		return -1;
	}

	QTextStream f(&file);

	while(!f.atEnd()) {
		building_t building;

		auto line = f.readLine(2048);
		auto split = line.split('\t');
		if(split.length() != 16) //we got a problem
			continue;

		//more validation needed
		int pos = 0;
		building.asset_id = split[pos++].toInt();
		//building.slot = split[pos++].toInt();
		building.row = split[pos++].toInt();
		//building.town_xpos = split[pos++].toFloat();
		//building.town_ypos = split[pos++].toFloat();
		//building.town_zpos = split[pos++].toUInt();
		building.type = get_enum_value<building_e>(split[pos++]);
		building.subtype = get_enum_value<building_base_type_e>(split[pos++]);
		building.matching_faction = get_enum_value<hero_class_e>(split[pos++]);
		building.generated_creature = get_enum_value<unit_type_e>(split[pos++]);
		building.weekly_growth = split[pos++].toUInt();
		
		building.name = split[pos++].toStdString();
		building.description = split[pos++].toStdString();
		
		//building.prerequisites =
		auto prereqs = split[pos++].split(',');
		for(auto r : prereqs) {
			auto p = get_enum_value<building_e>(r);
			building.prerequisites.push_back(p);
		}
		
		resource_group_t res;
		res.set_value_for_type(RESOURCE_GOLD, split[pos++].toUShort());
		res.set_value_for_type(RESOURCE_WOOD, split[pos++].toUShort());
		res.set_value_for_type(RESOURCE_ORE, split[pos++].toUShort());
		res.set_value_for_type(RESOURCE_GEMS, split[pos++].toUShort());
		res.set_value_for_type(RESOURCE_MERCURY, split[pos++].toUShort());
		res.set_value_for_type(RESOURCE_CRYSTALS, split[pos++].toUShort());
		//RESOURCE_SULFUR //todo
		
		building.cost = res;
		buildings.push_back(building);
	}
	
	return 0;
}

int game_config::load_talents(const std::string& path_prefix) {
	QString talents_file = QString::fromStdString(path_prefix) + "config/talents.tsv";
	QFile file(talents_file);
	if(!file.open(QFile::ReadOnly)) {
		std::cerr << "Could not open talents file: " << file.errorString().toStdString() << std::endl;
		return -1;
	}

	QTextStream f(&file);

	while(!f.atEnd()) {
		talent_t talent;

		auto line = f.readLine(2048);
		auto split = line.split('\t');
		if(split.length() != 6) //we got a problem
			continue;

		//more validation needed
		int pos = 0;
		talent.hero_class = get_enum_value<hero_class_e>(split[pos++]);
		talent.asset_id = split[pos++].toInt();
		talent.type = get_enum_value<talent_e>(split[pos++]);
		talent.name = split[pos++].toStdString();
		talent.description = split[pos++].toStdString();
		talent.tier = split[pos++].toInt();
		talents.push_back(talent);
	}
	
	return 0;
}


int game_config::load_skills(const std::string& path_prefix) {
	QString skills_file = QString::fromStdString(path_prefix) + "config/skills.tsv";
	QFile file(skills_file);
	if(!file.open(QFile::ReadOnly)) {
		std::cerr << "Could not open skills file: " << file.errorString().toStdString() << std::endl;
		return -1;
	}

	QTextStream f(&file);

	while(!f.atEnd()) {
		skill_t skill;

		auto line = f.readLine(2048);
		auto split = line.split('\t');
		if(split.length() != 10) //we got a problem
			continue;

		//more validation needed
		int pos = 0;
		skill.asset_id = split[pos++].toInt();
		auto str = split[pos++].toStdString();
		auto cl = magic_enum::enum_cast<skill_e>(str);
		if(!cl.has_value())
			continue;
		
		skill.skill_id = cl.value();
		
		skill.name = split[pos++].toStdString();
		skill.description = split[pos++].toStdString();
		
		for(int i = 0; i < 6; i++)
			skill.class_learnrate[i] = split[pos++].toInt();
		
		skills.push_back(skill);
	}
	
	return 0;
}
//todo:  what we should do is sort these arrays so that the position matches
//the value of the enum, reducing this to a constant time function

const artifact_t& game_config::get_artifact(artifact_e artifact_id) {
	auto adjusted_id = artifact_id;
	if(artifact_t::is_spell_scroll(artifact_id))
		adjusted_id = ARTIFACT_SPELL_SCROLL;

	for(auto& a : artifacts) {
		if(a.id == adjusted_id)
			return a;
	}
	
	return artifacts[0];
}


void game_config::add_or_update_custom_artifact(const artifact_t& artifact) {
	for(auto& a : artifacts) {
		if(a.id == artifact.id) {
			a = artifact;
			return;
		}
	}
	
	artifacts.push_back(artifact);
}


artifact_e game_config::get_next_custom_artifact_id() {
	auto id = ARTIFACT_NONE;
	for(auto& a : artifacts) {
		if(a.id > id)
			id = a.id;
	}
	
	if(id < ARTIFACT_CUSTOM)
		return ARTIFACT_CUSTOM_1;
	
	return (artifact_e)(id + 1);
}

const talent_t& game_config::get_talent(talent_e talent) {
	for(auto& t : talents) {
		if(t.type == talent)
			return t;
	}
	
	return talents[0];
}

const buff_info_t& game_config::get_buff_info(buff_e buff_id) {
	for(auto& b : buff_info) {
		if(b.buff_id == buff_id)
			return b;
	}
	
	return buff_info[0];
}

const hero_specialty_t& game_config::get_specialty(hero_specialty_e specialty_id) {
	for(auto& s : specialties) {
		if(s.id == specialty_id)
			return s;
	}
	
	return specialties[0];
}

const skill_t& game_config::get_skill(skill_e skill) {
	for(auto& s : skills) {
		if(s.skill_id == skill)
			return s;
	}
	
	return skills[0];
}

const building_t& game_config::get_building(building_e building_id) {
	for(auto& b : buildings) {
		if(b.type == building_id)
			return b;
	}
	
	return buildings[0];
}

const spell_t& game_config::get_spell(spell_e spell_id) {
	for(auto& s : spells) {
		if(s.id == spell_id)
			return s;
	}

	return spells[0];
}

const creature_t& game_config::get_creature(unit_type_e unit_id) {
	assert(creatures.size() != 0);

	if(unit_id >= creatures.size() || unit_id == 0) {
		//throw 
		//log_error("creature with id:" + utils::to_str(unit_id.type_id) + " not found.");
		for(const auto& cr : creatures) {
			if(cr.unit_type == unit_id)
				return cr;
		}

		return creatures[0];
	}

	return creatures[unit_id];
}

const std::string game_config::get_random_town_name(town_type_e town_type) {
	std::vector<std::string> names;
	for(auto& tn : town_names) {
		if(tn.first == town_type)
			names.push_back(tn.second);
	}
	
	if(!names.size())
		return "FIXME RANDOM";
	//assert(names.size());
	
	
	return names[rand() % names.size()];
}

float game_config::get_attack_bonus_multiplier() {
	return 0.1f;
}

float game_config::get_defense_bonus_multiplier() {
	return 0.05f;
}

uint game_config::get_max_attack_bonus_difference() {
	return 30;
}

uint game_config::get_max_defense_bonus_difference() {
	return 15;
}
