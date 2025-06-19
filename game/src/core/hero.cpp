#include "core/hero.h"
#include "core/interactable_object.h"
#include "core/adventure_map.h"
#include "core/qt_headers.h"
#include "core/magic_enum.hpp"

#include <random>
#include <chrono>
#include <vector>

QDataStream& operator<<(QDataStream& stream, const hero_appearance_t& appearance) {
	stream << (uint8_t)appearance.class_appearance
		<< (uint8_t)appearance.body
		<< (uint8_t)appearance.face
		<< (uint8_t)appearance.eye_color
		<< (uint8_t)appearance.beard
		<< (uint8_t)appearance.eyebrows
		<< (uint8_t)appearance.ears
		<< (uint8_t)appearance.skin
		<< (uint8_t)appearance.hair
		<< (uint8_t)appearance.hair_color
		<< (uint8_t)appearance.mount_type
		<< (uint8_t)appearance.pet_type;

	return stream;
}

QDataStream& operator>>(QDataStream& stream, hero_appearance_t& appearance) {
	uint8_t class_appearance, body, face, eye_color, beard, eyebrows,
		ears, skin, hair, hair_color, mount_type, pet_type;

	stream >> class_appearance >> body >> face >> eye_color
		>> beard >> eyebrows >> ears >> skin >> hair
		>> hair_color >> mount_type >> pet_type;

	appearance.class_appearance = (hero_class_appearance_e)class_appearance;
	appearance.body = (hero_body_appearance_e)body;
	appearance.face = (hero_face_appearance_e)face;
	appearance.eye_color = (hero_eyes_appearance_e)eye_color;
	appearance.beard = (hero_beard_appearance_e)beard;
	appearance.eyebrows = (hero_eyebrow_appearance_e)eyebrows;
	appearance.ears = (hero_ears_appearance_e)ears;
	appearance.skin = (hero_skin_appearance_e)skin;
	appearance.hair = (hero_hair_appearance_e)hair;
	appearance.hair_color = (hero_hair_color_appearance_e)hair_color;
	appearance.mount_type = (hero_mount_type_e)mount_type;
	appearance.pet_type = (hero_pet_type_e)pet_type;

	return stream;
}

//write hero data
QDataStream& operator<<(QDataStream& stream, const hero_t& hero) {
	stream << hero.id << hero.player << hero.x << hero.y
			<< QString::fromStdString(hero.name) << QString::fromStdString(hero.title) << QString::fromStdString(hero.biography)
			<< hero.hero_class << hero.portrait << hero.specialty
			<< hero.uncontrolled_by_human << hero.garrisoned
			<< hero.level << hero.attack << hero.defense << hero.power << hero.knowledge
			<< (quint64)hero.experience << hero.mana << hero.movement_points;
	
	for(auto& s : hero.skills)
		stream << s.skill << s.level;
	
	for(auto& a : hero.artifacts)
		stream << a;
	
	for(auto& a : hero.backpack)
		stream << a;
	
	for(auto& t : hero.troops)
		stream << t;
	
	for(auto& t : hero.available_talents)
		stream << t;
	
	for(auto& t : hero.talents)
		stream << t;
	
	stream_write_vector(stream, hero.spellbook);
	stream_write_vector(stream, hero.scripts);
	stream_write_vector(stream, hero.visited_objects);

	stream << hero.outlaw_bonus;
	stream << hero.appereance;

	stream_write_vector(stream, hero.enabled_skills);

	stream << hero.has_ballista;

	stream << hero.start_of_day_x;
	stream << hero.start_of_day_y;
	stream << hero.wormhole_casts_today;
	stream << hero.mana_battery_casts_today;

	return stream;
}

//read hero data
QDataStream& operator>>(QDataStream& stream, hero_t& hero) {
	QString name, title, biography;
	stream >> hero.id >> hero.player >> hero.x >> hero.y
			>> name >> title >> biography
			>> hero.hero_class >> hero.portrait >> hero.specialty
			>> hero.uncontrolled_by_human >> hero.garrisoned
			>> hero.level >> hero.attack >> hero.defense >> hero.power >> hero.knowledge
			>> (quint64&)hero.experience >> hero.mana >> hero.movement_points;
	
	for(auto& s : hero.skills)
		stream >> s.skill >> s.level;
	
	for(auto& a : hero.artifacts)
		stream >> a;
	
	for(auto& a : hero.backpack)
		stream >> a;
	
	for(auto& t : hero.troops)
		stream >> t;
	
	for(auto& t : hero.available_talents)
		stream >> t;
	
	for(auto& t : hero.talents)
		stream >> t;
		
	stream_read_vector(stream, hero.spellbook, SPELL_UNKNOWN, max_enum_val<spell_e>());
	stream_read_vector(stream, hero.scripts);
	stream_read_vector(stream, hero.visited_objects);
	
	stream >> hero.outlaw_bonus;
	stream >> hero.appereance;

	stream_read_vector(stream, hero.enabled_skills);

	stream >> hero.has_ballista;

	stream >> hero.start_of_day_x;
	stream >> hero.start_of_day_y;
	stream >> hero.wormhole_casts_today;
	stream >> hero.mana_battery_casts_today;

	hero.name = name.toStdString();
	hero.title = title.toStdString();
	hero.biography = biography.toStdString();

	return stream;
}

std::string get_temp_morale_effect_string(temp_morale_effect_e type, int magnitude) {
	auto magnitude_str = QString(magnitude > 0 ? "+%1" : "%1").arg(magnitude);

	switch(type) {
		case MORALE_EFFECT_WARRIORS_TOMB: return QObject::tr("Tomb of the Fallen visited (%1)").arg(magnitude_str).toStdString();
		case MORALE_EFFECT_GRAVEYARD: return QObject::tr("Graveyard visited (%1)").arg(magnitude_str).toStdString();
		case MORALE_EFFECT_HOMETOWN_HERO: return QObject::tr("Hometown Hero (%1)").arg(magnitude_str).toStdString();
		default: return QObject::tr("Unknown (%1)").arg(magnitude_str).toStdString();
	}
}

std::string get_temp_luck_effect_string(temp_luck_effect_e type, int magnitude) {
	auto magnitude_str = QString(magnitude > 0 ? "+%1" : "%1").arg(magnitude);

	switch(type) {
		case LUCK_EFFECT_NONE: return QObject::tr("None").toStdString();
		case LUCK_EFFECT_PYRAMID_VISIT: return QObject::tr("Pyramid visited (%1)").arg(magnitude_str).toStdString();
		default: return QObject::tr("Unknown (%1)").arg(magnitude_str).toStdString();
	}
}

stat_distribution_t hero_t::get_stat_distribution_for_class(hero_class_e hero_class, int level) {
	constexpr int switch_level = 10;
	
	switch(hero_class) {
		case HERO_KNIGHT:
			if(level < switch_level)
				return {35,45,10,10};
			else
				return {30,30,20,20};
		case HERO_BARBARIAN:
		  if(level < switch_level)
			  return {55, 25, 10, 10};
		  else
			  return {40, 30, 15, 15};
		case HERO_NECROMANCER:
		  if(level < switch_level)
			  return {30, 30, 20, 20};
		  else
			  return {25, 25, 25, 25};
		case HERO_SORCERESS:
		  if(level < switch_level)
			  return {20, 20, 30, 30};
		  else
			  return {25, 25, 25, 25};
		case HERO_WARLOCK:
		  if(level < switch_level)
			  return {10, 10, 60, 20};
		  else
			  return {15, 15, 45, 25};
		case HERO_WIZARD:
		  if(level < switch_level)
			  return {15, 15, 30, 40};
		  else
			  return {20, 20, 30, 30};
		
		case HERO_CUSTOM:
			break;
	}
	
	
	return {25,25,25,25};
}

stat_e get_random_stat(hero_class_e hero_class, int level) {
	auto distribution = hero_t::get_stat_distribution_for_class(hero_class, level);

	std::array<uint8_t, 4> distribution_array = {distribution.attack, distribution.defense, distribution.power, distribution.knowledge};

	std::random_device rd;
	std::mt19937 gen(rd());
	std::discrete_distribution<> stat_distribution(distribution_array.begin(), distribution_array.end());

	return (stat_e)(stat_distribution(gen));
}

template<typename T> T get_skill_value(const T& base, const T& per_level, int skill_level);
//{
//	if(skill_level == 0)
//		return (T)0;
//	
//	return (T)(base + (per_level * (skill_level-1)));
//}

static const char* class_names[] = { "Paladin", "Barbarian", "Necromancer", "Enchantress", "Warlock", "Wizard", "Unknown" };

const std::string hero_t::get_class_name(hero_class_e hero_class) {
	assert(hero_class < sizeof(class_names));
	return QObject::tr(class_names[(int)hero_class]).toStdString();
}

const std::string hero_t::get_secondary_skill_level_name(int level) {
	switch(level) {
		case 0: return QObject::tr("Unlearned").toStdString();
		case 1: return QObject::tr("Apprentice").toStdString();
		case 2: return QObject::tr("Intermediate").toStdString();
		case 3: return QObject::tr("Expert").toStdString();
		case 4: return QObject::tr("Master").toStdString();
		case 5: return QObject::tr("Grandmaster").toStdString();
	}
	
	return "???";
}

void hero_t::increase_attack(uint8_t amount) {
	if(amount > 99)
		amount = 99;
	
	attack = std::min(99, attack + amount);
}

void hero_t::increase_defense(uint8_t amount) {
	if(amount > 99)
		amount = 99;
	
	defense = std::min(99, defense + amount);
}

void hero_t::increase_power(uint8_t amount) {
	if(amount > 99)
		amount = 99;
	
	power = std::min(99, power + amount);
}

void hero_t::increase_knowledge(uint8_t amount) {
	if(amount > 99)
		amount = 99;
	
	knowledge = std::min(99, knowledge + amount);
}

float hero_t::get_specialty_multiplier() const {
	auto& sp_info = game_config::get_specialty(specialty);
	if(sp_info.multiplier.is_power_multiplied)
		return 1.0f + ((float)sp_info.multiplier.get_value(level) / 100.f);

	return sp_info.multiplier.fixed;
}

uint8_t hero_t::get_effective_attack(bool ignore_percentage_reductions) const {
	//move this elsewhere
	int modifier = 0;
	
	if(has_talent(TALENT_OFFENSE))
		modifier++;
	
	for(auto a : artifacts) {
		if(a == ARTIFACT_NONE)
			continue;
		
		const auto& artifact = game_config::get_artifact(a);
		modifier += artifact.get_attack_bonus();
	}
	
	if(has_talent(TALENT_WEAPON_MASTER))
		modifier += (modifier * .4f);
	
	if(is_artifact_in_effect(ARTIFACT_LEGENDARY_CROWN))
		modifier += (modifier * .25f);
	
	//must be calculated last
	if(!ignore_percentage_reductions && has_talent(TALENT_UNBRIDLED_POWER))
		modifier -= (attack + modifier) / 4;
		
	//all primary stats are capped at 99, and cannot be negative
	return std::min(99, std::max(0, attack + modifier));
}

uint8_t hero_t::get_effective_defense(bool ignore_percentage_reductions) const {
	int modifier = 0;
	
	for(auto a : artifacts) {
		if(a == ARTIFACT_NONE)
			continue;
		
		const auto& artifact = game_config::get_artifact(a);
		modifier += artifact.get_defense_bonus();
	}
	
	if(has_talent(TALENT_ARMOURER))
		modifier += (modifier * .4f);
	
	if(is_artifact_in_effect(ARTIFACT_LEGENDARY_CROWN))
		modifier += (modifier * .25f);
	
	//must be calculated last
	if(!ignore_percentage_reductions && has_talent(TALENT_UNBRIDLED_POWER))
		modifier -= (defense + modifier) / 4;
	
	return defense + modifier;
}

uint8_t hero_t::get_effective_power() const {
	int modifier = 0;
	
	if(has_talent(TALENT_POWER_HUNGRY))
		modifier++;
	
	for(auto a : artifacts) {
		if(a == ARTIFACT_NONE)
			continue;
		
		const auto& artifact = game_config::get_artifact(a);
		modifier += artifact.get_power_bonus();
	}

	if(has_talent(TALENT_UNBRIDLED_POWER))
		modifier += (get_effective_attack() + get_effective_defense()) / 4;
	
	
	if(is_artifact_in_effect(ARTIFACT_LEGENDARY_CROWN))
		modifier += (modifier * .25f);
	
	return power + modifier;
}

uint8_t hero_t::get_effective_knowledge() const {
	int modifier = 0;
	
	if(has_talent(TALENT_WIZARDRY))
		modifier++;
	
	for(auto a : artifacts) {
		if(a == ARTIFACT_NONE)
			continue;
		
		const auto& artifact = game_config::get_artifact(a);
		modifier += artifact.get_knowledge_bonus();
	}
	
	if(is_artifact_in_effect(ARTIFACT_LEGENDARY_CROWN))
		modifier += (modifier * .25f);
	
	return knowledge + modifier;
}

int hero_t::get_luck() const {
	int luck = 0;
	luck += get_secondary_skill_level(SKILL_LUCK);
	
	for(auto a : artifacts) {
		if(a == ARTIFACT_NONE)
			continue;
		
		const auto& artifact = game_config::get_artifact(a);
		luck += artifact.get_luck_bonus();
	}
	
	if(has_talent(TALENT_FAERIES_FORTUNE))
		luck++;
	
	for(auto& e : temp_luck_effects) {
		luck += e.magnitude;
	}
	
	return luck;
}

uint16_t hero_t::get_maximum_dark_energy() const {
	uint16_t total = 0;
	
	if(has_talent(TALENT_NECROMASTERY))
		total += 30;
	
	int necro_value = get_skill_value(10, 5, SKILL_NECROMANCY);
	int kpp = get_effective_knowledge() + get_effective_power();
	
	float bonus = 1.f;
	if(is_artifact_in_effect(ARTIFACT_WAISTGUARD_OF_THE_DAMNED))
		bonus += .15f;

	total += (necro_value * kpp * bonus);
	
	return total;
}

int hero_t::get_number_of_troop_alignments() const {
	constexpr auto NUM_FACTIONS = 10; //magic_enum::enum_count<hero_class_e>();
	std::array<int, NUM_FACTIONS> has_alignment{};
	
	for(const auto& tr : troops) {
		if(tr.unit_type == UNIT_UNKNOWN)
			continue;
		
		const auto& cr = game_config::get_creature(tr.unit_type);
		assert(cr.faction < NUM_FACTIONS); //fixme
		has_alignment[cr.faction] = 1;
	}
	
	return std::accumulate(has_alignment.begin(), has_alignment.end(), 0);
}

bool hero_t::has_no_troops_in_army() const {
	for(const auto& tr : troops) {
		if(!tr.is_empty())
			return false;
	}

	return true;
}

bool hero_t::has_undead_in_army() const {
	for(const auto& tr : troops) {
		if(tr.unit_type == UNIT_UNKNOWN)
			continue;
		
		const auto& cr = game_config::get_creature(tr.unit_type);
		if(cr.has_inherent_buff(BUFF_UNDEAD))
			return true;
	}
	
	return false;
}

int hero_t::get_morale() const {
	int morale = 0;
	morale += get_secondary_skill_level(SKILL_LEADERSHIP);
	
	for(auto a : artifacts) {
		if(a == ARTIFACT_NONE)
			continue;
		
		const auto& artifact = game_config::get_artifact(a);
		morale += artifact.get_morale_bonus();
	}
	
	if(has_talent(TALENT_KNIGHTS_HONOR))
		morale++;
	
	int troop_alignments = get_number_of_troop_alignments();
	if(has_talent(TALENT_NATURAL_LEADER)) {
		if(troop_alignments == 1)
			morale++;
	}
	else {
		morale += (2 - troop_alignments);
	}
	
	if(has_undead_in_army())
		morale -= 1;

	for(auto& e : temp_morale_effects) {
		morale += e.magnitude;
	}
	
	return morale;
}


bool hero_t::add_temporary_morale_effect(temp_morale_effect_e type, int8_t magnitude, int8_t duration) {
	for(auto& e : temp_morale_effects) {
		if(e.effect_type == type) {
			//refresh duration
			e.duration = duration;
			return true;
		}
	}
	
	temp_morale_effects.push_back({type, magnitude, duration});
	return false;
}

bool hero_t::add_temporary_luck_effect(temp_luck_effect_e type, int8_t magnitude, int8_t duration) {
	for(auto& e : temp_luck_effects) {
		if(e.effect_type == type) {
			//refresh duration
			e.duration = duration;
			return true;
		}
	}
	
	temp_luck_effects.push_back({type, magnitude, duration});
	return false;
}

bool hero_t::is_artifact_equipped(artifact_e artifact) const {
	for(auto a : artifacts) {
		if(a == artifact)
			return true;
	}
	
	return false;
}

bool hero_t::is_artifact_in_effect(artifact_e artifact) const {
	if(is_artifact_equipped(artifact))
		return true;
	
	//check to see if artifact is one which has effects active in backpack
	//move this elsewhere
	/*if(artifact == ARTIFACT_MONOCULAR) {
		for(auto a : backpack) {
			if(a == artifact)
				return true;
		}
			
	}*/
	return false;
}

bool hero_t::does_artifact_fit_in_slot(artifact_slot_e artifact_slot, uint hero_slot) const {
	if(artifact_slot > game_config::HERO_ARTIFACT_SLOTS || hero_slot > game_config::HERO_ARTIFACT_SLOTS)
		return false;
	
	//todo fixme
	//if(artifact_t::slot_matches(hero_slot, artifact_slot) return true;
	if(hero_slot == artifact_slot)
		return true;
	
	if(artifact_slot == SLOT_TRINKET && hero_slot >= SLOT_TRINKET)
		return true;
	
	if(artifact_slot == SLOT_RING && hero_slot == SLOT_RING_2)
		return true;
	
	if(hero_class == HERO_BARBARIAN && artifact_slot == SLOT_WEAPON && hero_slot == SLOT_SHIELD)
		return true;
	
	return false;
}

bool hero_t::is_backpack_full() const {
	for(int i = 0; i < backpack.size(); i++) {
		if(backpack[i] == ARTIFACT_NONE)
			return false;
	}

	return true;
}

int hero_t::get_secondary_skill_base_level(skill_e skill) const {
	for(auto& s : skills) {
		if(s.skill == skill) {
			return s.level;
		}
	}

	return 0;
}

float hero_t::get_spell_effect_multiplier(spell_e spell_id) const {
	float multiplier = 1.0f;

	auto& spell = game_config::get_spell(spell_id);

	if(spell.school == SCHOOL_NATURE) {
		multiplier += get_skill_value(.1f, .05f, get_secondary_skill_level(SKILL_NATURE_MAGIC));
		if(has_talent(TALENT_GIFT_OF_THE_GODDESS))
			multiplier += .25f;
	}
	else if(spell.school == SCHOOL_ARCANE) {
		multiplier += get_skill_value(.1f, .05f, get_secondary_skill_level(SKILL_ARCANE_MAGIC));
		if(is_artifact_in_effect(ARTIFACT_ARCANE_CLOAK))
			multiplier += .5f;

		if(has_talent(TALENT_ARCANE_AFFINITY))
			multiplier += .25f;
	}
	else if(spell.school == SCHOOL_DEATH) {
		if(has_talent(TALENT_DEATHS_EMBRACE))
			multiplier += .25f;
	}
	else if(spell.school == SCHOOL_DESTRUCTION) {
		if(has_talent(TALENT_APOCALYPTIC))
			multiplier += .15f;
	}

	if(has_talent(TALENT_SPELL_MASTERY))
		multiplier += .4f;

	//todo: deal with skills like sorcery only affecting damage component of spells
	if(spell.damage_type != MAGIC_DAMAGE_NONE)
		multiplier += get_skill_value(.15f, .05f, get_secondary_skill_level(SKILL_SORCERY));

	if(spell_id == SPELL_REANIMATE_DEAD && has_talent(TALENT_REANIMATOR))
		multiplier += .5f;

	return multiplier;
}

float hero_t::get_spell_haste_percentage() const {
	float specialty_bonus = (specialty == SPECIALTY_SPELL_HASTE ? .05f : 0.f);
	auto skill_multi = get_skill_value(.1f + specialty_bonus, .05f, get_secondary_skill_level(SKILL_SPELL_HASTE));
	
	float talent_multi = 0;
	if(has_talent(TALENT_SORCERERS_SWIFTNESS))
		talent_multi += .1f;
	
	if(has_talent(TALENT_WANDSLINGER))
		talent_multi += .4f;
	
	uint artifact_total = 0;
	for(const auto art_id : artifacts) {
		if(art_id == ARTIFACT_NONE)
			continue;
		
		const auto& art_info = game_config::get_artifact(art_id);
		for(auto e : art_info.effects) {
			if(e.type == EFFECT_SPELL_HASTE_BOOST) {
				artifact_total += e.magnitude_1;
			}
		}
	}
	
	float artifact_multi = artifact_total / 100.f;
	float total_reduction = (skill_multi + talent_multi + artifact_multi);// *
	float value = (1.f - total_reduction);
	
	return value;
}

								   
uint hero_t::get_adjusted_power() {
	return get_effective_power();
}

int hero_t::get_artifact_effect_bonus_value(artifact_effect_e artifact_effect) const {
	for(const auto& art : artifacts) {
		const auto& art_info = game_config::get_artifact(art);
		for(const auto eff : art_info.effects) {
			if(eff.type == artifact_effect) {
				return eff.magnitude_1;
			}
		}
	}

	return 0;
}

int hero_t::get_secondary_skill_level(skill_e skill) const {
	int modifier = 0;
	if(skill == SKILL_SCOUTING && is_artifact_in_effect(ARTIFACT_MONOCULAR))
		modifier += 1;
	
	if(skill == SKILL_SORCERY && is_artifact_in_effect(ARTIFACT_CUIRASS_OF_THE_ARCHMAGE))
		modifier += 1;
	
	if(skill == SKILL_INTELLIGENCE && is_artifact_in_effect(ARTIFACT_DIADEM_OF_THE_ARCHMAGE))
		modifier += 1;

	//todo/fixme
	//if(skill == SKILL_INTELLIGENCE)
	//	modifier += get_artifact_effect_bonus_value(EFFECT_SKILL_BOOST, SKILL_INTELLIGENCE);

	if(is_artifact_in_effect(ARTIFACT_JORDANS_STONE))
		modifier += 1;
	
	int base = get_secondary_skill_base_level(skill);
	if(base > 0)
		base += modifier;

	//cap skill levels at 5
	return std::clamp(base, 0, 5);
}

int hero_t::get_slowest_unit_speed_in_army() const {
	int speed = 8;
	
	for(const auto& tr : troops) {
		if(tr.unit_type == UNIT_UNKNOWN)
			continue;
		
		if((int)tr.get_base_speed() < speed)
			speed = tr.get_base_speed();
	}
	
	return speed;
}

uint16_t hero_t::calculate_maximum_daily_movement_points(int day, terrain_type_e starting_terrain) const {
	int speed_factor = 50 * std::clamp((((int)get_slowest_unit_speed_in_army()) - 2), 0, 12);
	const int base = 1500 + speed_factor;
	
	float modifier = 1.0f + (.05 * get_secondary_skill_level(SKILL_LOGISTICS));
	
	if(day % 7 == 0 && has_talent(TALENT_FRESH_START))
		modifier += 0.25f;
	
	if(has_talent(TALENT_TIRELESS))
		modifier += 0.10f;
	
	if(has_talent(TALENT_MARCH_OF_THE_DEAD))
		modifier += 0.10f;
	
	if(has_talent(TALENT_NAVIGATOR))
		modifier += 0.15f;
	
	if(starting_terrain == TERRAIN_GRASS && has_talent(TALENT_GRASSLANDS_GRACE))
		modifier += 0.15f;
	
	if(is_artifact_in_effect(ARTIFACT_BOOTS_OF_THE_NOMAD))
		modifier += 0.2f;

	if(is_artifact_in_effect(ARTIFACT_MAGELLANS_COMPASS))
		modifier += 0.15f;

	return (uint16_t)std::round(base * modifier);
}

int hero_t::get_scouting_radius() const {
	static const int scouting_table[] = {0, 1, 2, 3, 4, 5};
	int scouting_level = get_secondary_skill_level(SKILL_SCOUTING);
	int modifier = 0;
	if(is_artifact_in_effect(ARTIFACT_MONOCULAR) && scouting_level == 0)
		modifier = 1;
	
	if(has_talent(TALENT_EAGLE_EYE))
		modifier++;
	
	return base_scouting_radius + scouting_table[scouting_level] + modifier;
}

int hero_t::get_observation_radius() const {
	static const int scouting_table[] = {0, 3, 4, 5, 6, 7};
	int scouting_level = std::clamp(get_secondary_skill_level(SKILL_SCOUTING), 0, 5);
	int modifier = 0;
	if(is_artifact_in_effect(ARTIFACT_MONOCULAR) && scouting_level == 0)
		modifier = 2;
	
	return base_observation_radius + scouting_table[scouting_level] + modifier;
}

const std::string hero_t::get_class_name() const {
	return hero_t::get_class_name(hero_class);
}

std::pair<int, int> hero_t::get_ballista_damage_range() const {
	const auto& cr_info = game_config::get_creature(UNIT_BALLISTA);

	int min_dmg = cr_info.min_damage;
	int max_dmg = cr_info.max_damage;

	return std::make_pair(min_dmg, max_dmg);
}

int hero_t::get_ballista_shot_count() const {
	int count = 1;

	if(get_secondary_skill_level(SKILL_WAR_MACHINES))
		count += 1;

	if(has_talent(TALENT_MACHINIST))
		count += 1;

	return count;
}

bool hero_t::get_ballista_manual_control() const {
	return get_secondary_skill_level(SKILL_WAR_MACHINES) > 0;
}

const std::string hero_t::get_title() const {
	if(title.empty())
		return QObject::tr("the").toStdString() + " " + get_class_name();
	else
		return title;
}

bool hero_t::has_talent(talent_e talent) const {
	for(auto& t : talents) {
		if(t == talent)
			return true;
	}
	
	return false;
}

bool hero_t::has_talent_available(talent_e talent) const {
	for(auto& t : available_talents) {
		if(t == talent)
			return true;
	}
	
	return false;
}

bool hero_t::give_talent(talent_e talent) {
	if(!has_talent_available(talent) || talent_points_spent_count() >= game_config::HERO_TALENT_POINTS)
		return false;
	
	//can't have duplicate talents
	if(utils::contains(talents, talent))
		return false;
	
	for(auto& t : talents) {
		if(t == TALENT_NONE) {
			t = talent;
			break;
		}
	}
	
	return true;
}

uint hero_t::talent_points_spent_count() const {
	uint count = 0;
	for(auto& t : talents) {
		if(t != TALENT_NONE)
			count++;
	}
	
	return count;
}

bool hero_t::has_talent_point_on_level_up() const {
	int nextlevel = level + 1;
	if(talent_points_spent_count() >= game_config::HERO_TALENT_POINTS) //full on talents
		return false;
	
	if(nextlevel % 2 == 0) //talent points on every even level-up
		return true;
	
	//if we get here, the next level is odd.  however, if for some reason
	//the hero is a higher level than they should be relative to their talent
	//point distribution, let them select a new talent anyway
	int expected_points = level / 2;
	if(expected_points > (int)talent_points_spent_count())
		return true;
	
	return false;
}

//talent(s)		tier
//	0			0
//	1,2			1
//	3,4,5,6		2
//	7,8,9,10	3
//	11,12,13,14	4
//	15,16		5

const static int talent_to_tier_mapping[] = { 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5 };

//needed for ui
void hero_t::move_necromancy_to_top_slot() {
	for(size_t i = 0; i < skills.size(); i++) {
		if(skills[i].skill == SKILL_NECROMANCY && i != 0) {
			std::swap(skills[i], skills[0]);
			return;
		}
	}
}

void hero_t::init_hero(int day, terrain_type_e starting_terrain) {
	move_necromancy_to_top_slot();
	init_talents();
	init_army();
	mana = (hero_class == HERO_BARBARIAN ? 0 : get_maximum_mana());
	dark_energy = get_maximum_dark_energy();
	new_day(day, starting_terrain);
	//fixme: temporary hack
	for(auto& sk : game_config::get_skills()) {
		enabled_skills.push_back(sk.skill_id);
	}
}

void hero_t::init_army() {
	for(const auto& t : troops) {
		if(t.unit_type != UNIT_UNKNOWN)
			return;
	}
	
	uint i = 0;
	for(auto st : starting_army) {
		troops[i].unit_type = std::get<0>(st);
		troops[i].stack_size = utils::rand_range(std::get<1>(st), std::get<2>(st));
		i++;
	}
	
}

void hero_t::init_talents() {
	//if we already have talents selected (from editor), just return
	bool talents_filled = true;
	for(auto t : available_talents) {
		if(t == TALENT_NONE)
			talents_filled = false;
	}
	if(talents_filled)
		return;
	
	const int TALENT_TIERS = 6;
	std::vector<talent_e> selected[TALENT_TIERS];

	for(const auto& gt : guaranteed_talents) {
		if(gt == TALENT_NONE)
			continue;

		const auto& info = game_config::get_talent(gt);
		selected[info.tier].push_back(gt);
	}

	auto seed = (unsigned)std::chrono::system_clock::now().time_since_epoch().count();

	const int tier_size[] = { 1, 2, 4, 4, 4, 2 };
	auto shuffled_talents = game_config::get_talents();
	std::shuffle(shuffled_talents.begin(), shuffled_talents.end(), std::default_random_engine(seed));

	for(auto& t : shuffled_talents) {
		if(t.hero_class != hero_class)
			continue;
		
		if(selected[t.tier].size() >= tier_size[t.tier])
			continue;
		
		//prevent duplicates
		if(utils::contains(guaranteed_talents, t.type))
			continue;

		//assert(t.tier < TALENT_TIERS);
		selected[t.tier].push_back(t.type);
	}

	for(int i = 0; i < TALENT_TIERS; i++)
		std::shuffle(selected[i].begin(), selected[i].end(), std::default_random_engine(seed + 1));
	
	for(auto i = 0u; i < game_config::HERO_TALENT_SLOTS; i++) {
		if(!selected[talent_to_tier_mapping[i]].size())
			continue;
		
		available_talents[i] = selected[talent_to_tier_mapping[i]].back();
		selected[talent_to_tier_mapping[i]].pop_back();
	}
}

bool talent_valid(talent_e talent) {
	return !(talent == TALENT_NONE);
}

bool hero_t::is_talent_unlocked(talent_e talent_id) const {
	int pos = -1;
	for(int i = 0; i < available_talents.size(); i++) {
		if(available_talents[i] == talent_id)
			pos = i;
	}

	if(pos == -1)
		return false;

	return is_talent_position_unlocked(pos);
}

bool hero_t::is_talent_position_unlocked(int position) const {
	if(position == 0)
		return true;
	
	switch(position) {
		case 1:		case 2:
			return has_talent(available_talents[0]);
		case 3:		case 4:
			return has_talent(available_talents[1]);
		case 5:		case 6:
			return has_talent(available_talents[2]);
		case 7:		case 8:
			return has_talent(available_talents[3]) || has_talent(available_talents[4]);
		case 9:		case 10:
			return has_talent(available_talents[5]) || has_talent(available_talents[6]);
		case 11:	case 12:
			return has_talent(available_talents[7]) || has_talent(available_talents[8]);
		case 13:	case 14:
			return has_talent(available_talents[9]) || has_talent(available_talents[10]);
		case 15:
			return has_talent(available_talents[11]) || has_talent(available_talents[12]);
		case 16:
			return has_talent(available_talents[13]) || has_talent(available_talents[14]);
	}
	
	return false;
}


int hero_t::get_megalomania_artifact_count() const {
	assert(has_talent(TALENT_MEGALOMANIA));
	int total = 0;
	for(auto a : artifacts) {
		if(a == ARTIFACT_NONE)
			continue;
		
		auto& artifact = game_config::get_artifact(a);
		if(artifact.rarity >= RARITY_RARE)
			total++;
	}
	
	return total;
}

uint hero_t::get_spell_count_by_school(spell_school_e school, spell_type_e type_filter) const {
	if(school == SCHOOL_ALL && type_filter == SPELL_TYPE_ALL)
		return (uint)spellbook.size();
	
	uint total = 0;
	for(auto sp : spellbook) {
		auto& spell = game_config::get_spell(sp);
		if(spell.school != school && school != SCHOOL_ALL)
			continue;
		
		if(spell.spell_type != type_filter && type_filter != SPELL_TYPE_ALL)
			continue;

		total++;
	}
	
	return total;
}

bool hero_t::knows_spell(spell_e spell_id) const {
	for(auto sp : spellbook) {
		if(sp == spell_id)
			return true;
	}

	return false;
}

bool hero_t::can_learn_spell(spell_e spell_id) const {
	if(spell_id == SPELL_UNKNOWN)
		return false;
	
	const auto& spell = game_config::get_spell(spell_id);
	return can_learn_spell_type(spell.level, spell.school);
}

bool hero_t::learn_spell(spell_e spell_id) {
	if(!can_learn_spell(spell_id))
		return false;
	
	if(utils::contains(spellbook, spell_id))
	   return false;
	
	spellbook.push_back(spell_id);
	return true;
}

bool hero_t::can_learn_spell_type(int spell_level, spell_school_e school) const {
	if(school == SCHOOL_WARCRY)
		return hero_class == HERO_BARBARIAN;
	
	if(hero_class == HERO_BARBARIAN)
		return false;
	
	if(spell_level < 3) //unrestricted_spells == true
		return true;
	
	if(has_talent(TALENT_WISDOM))
		return true;
	
	if(school == SCHOOL_NATURE) {
		if((get_secondary_skill_level(SKILL_NATURE_MAGIC) + 2) >= spell_level)
			return true;
		if(hero_class == HERO_SORCERESS || hero_class == HERO_WIZARD)
			return true;
	}
	else if(school == SCHOOL_HOLY) {
		if(((get_secondary_skill_level(SKILL_HOLY_MAGIC) + 2) >= spell_level) || has_talent(TALENT_PATH_OF_THE_PIOUS))
			return true;
		if(hero_class == HERO_KNIGHT || hero_class == HERO_WIZARD)
			return true;
	}
	else if(school == SCHOOL_ARCANE) {
		if(((get_secondary_skill_level(SKILL_ARCANE_MAGIC) + 2) >= spell_level) || has_talent(TALENT_PATH_OF_THE_MAGI))
			return true;
		if(hero_class == HERO_WARLOCK || hero_class == HERO_WIZARD)
			return true;
	}
	else if(school == SCHOOL_DESTRUCTION) {
		if(((get_secondary_skill_level(SKILL_DESTRUCTION_MAGIC) + 2) >= spell_level) || has_talent(TALENT_DESTROYER))
			return true;
		if(hero_class == HERO_WARLOCK)
			return true;
	}
	else if(school == SCHOOL_DEATH) {
		if(get_secondary_skill_level(SKILL_DEATH_MAGIC) || has_talent(TALENT_THE_DARK_ARTS))
			return true;
		if(hero_class == HERO_NECROMANCER)
			return true;
	}
	
	return false;
}

uint16_t hero_t::get_spell_cost(spell_e spell_id) const {
	const auto& spell = game_config::get_spell(spell_id);
	
	int base_cost = spell.mana_cost;
	float multi = 1.0;
	int flat = 0;
	
	if(is_artifact_in_effect(ARTIFACT_WOODLAND_SHIELD) && spell.school == SCHOOL_NATURE)
		flat -= 2;

	if(spell.school == SCHOOL_ARCANE && has_talent(TALENT_EFFICIENCY))
		multi -= 0.15f;
	
	if(spell.school != SCHOOL_ARCANE && has_talent(TALENT_VERSATILITY))
		multi -= 0.15f;

	int total = std::round(base_cost * multi) + flat;

	if(spell_id == SPELL_TIME_DILATION && has_talent(TALENT_RELATIVITY))
		total /= 2;

	return total;
}

void hero_t::new_day(int day, terrain_type_e starting_terrain) {
	maximum_daily_movement_points = calculate_maximum_daily_movement_points(day, starting_terrain);
	movement_points = maximum_daily_movement_points;

	start_of_day_x = x;
	start_of_day_y = y;
	wormhole_casts_today = 0;
	mana_battery_casts_today = 0;
	
	mana = std::min(mana + get_daily_mana_regen(), (int)get_maximum_mana());
}

bool hero_t::move_artifacts_in_backpack(uint from_slot, uint to_slot) {
	if(from_slot >= game_config::HERO_BACKPACK_SLOTS || to_slot >= game_config::HERO_BACKPACK_SLOTS)
		return false;
	
	std::swap(backpack[from_slot], backpack[to_slot]);
	
	return true;
}

bool hero_t::move_artifact_from_backpack_to_slot(uint from_slot, uint to_slot) {
	if(to_slot == 0 || from_slot >= game_config::HERO_BACKPACK_SLOTS || to_slot > game_config::HERO_ARTIFACT_SLOTS)
		return false;
	
	auto slot = game_config::get_artifact(backpack[from_slot]).slot;
	
	if(!does_artifact_fit_in_slot(slot, to_slot))
		return false;
	
	auto next_slot = to_slot;
	if(to_slot == SLOT_RING) {
		//if ring1 slot is occupied and ring2 slot is empty, move the artifact into ring2 slot
		if(artifacts[next_slot-1] == ARTIFACT_NONE && artifacts[next_slot] == ARTIFACT_NONE)
			next_slot = next_slot + 1;
	}
	//else if(to_slot == SLOT_TRINKET) {
	//	//try to find an empty trinket slot
	//	while(next_slot <= game_config::HERO_ARTIFACT_SLOTS) {
	//		if(artifacts[next_slot-1] == ARTIFACT_NONE)
	//			break;
	//		
	//		next_slot++;
	//	}
	//	
	//	//there are no empty trinket slots, so swap with the first trinket slot
	//	if(next_slot > game_config::HERO_ARTIFACT_SLOTS && artifacts[next_slot-1] != ARTIFACT_NONE) //fixme reading out of bounds here
	//		next_slot = to_slot;
	//}
	
	std::swap(backpack[from_slot], artifacts[next_slot-1]);
	
	return true;
}

bool hero_t::move_artifact_to_backpack_from_slot(uint from_slot) {
	if(from_slot == 0 || from_slot > game_config::HERO_ARTIFACT_SLOTS)
		return false;
	
	int open_slot = -1;//backpack.get_next_open_slot()
	for(uint i = 0; i < game_config::HERO_BACKPACK_SLOTS; i++) {
		if(backpack[i] == 0) {
			open_slot = i;
			break;
		}
	}
	
	if(open_slot == -1) //backpack is full
		return false;
	
	std::swap(artifacts[from_slot-1], backpack[open_slot]);
	return true;
}

bool hero_t::move_artifact_to_backpack_slot_from_slot(uint from_slot, uint to_slot) {
	if(from_slot == 0 || from_slot > game_config::HERO_ARTIFACT_SLOTS || to_slot >= game_config::HERO_BACKPACK_SLOTS)
		return false;
	

	auto slot = game_config::get_artifact(backpack[to_slot]).slot;
	if(does_artifact_fit_in_slot(slot, from_slot)) {
		std::swap(artifacts[from_slot-1], backpack[to_slot]);
		return true;
	}
	else {
		//if the item in the destination backpack slot does not
		//fit into the source artifact slot, then move it to the first empty backpack slot
		
		int open_slot = -1;//backpack.get_next_open_slot()
		for(uint i = 0; i < game_config::HERO_BACKPACK_SLOTS; i++) {
			if(backpack[i] == 0) {
				open_slot = i;
				break;
			}
		}
		
		if(open_slot == -1) //backpack is full
			return false;
		
		//if the requested destination backpack slot is empty, move it there
		if(backpack[to_slot] == ARTIFACT_NONE)
			open_slot = to_slot;
		
		std::swap(artifacts[from_slot-1], backpack[open_slot]);
		return true;
	}
	
	return true;
}

bool hero_t::move_artifacts_in_paperdoll_slot(uint from_slot, uint to_slot) {
	if(from_slot == 0 || from_slot > game_config::HERO_BACKPACK_SLOTS
	   || to_slot == 0 || to_slot > game_config::HERO_BACKPACK_SLOTS)
		return false;
	
	bool from_empty = artifacts[from_slot-1] == ARTIFACT_NONE;
	bool to_empty = artifacts[to_slot-1] == ARTIFACT_NONE;
	auto slot1 = game_config::get_artifact(artifacts[from_slot-1]).slot;
	auto slot2 = game_config::get_artifact(artifacts[to_slot-1]).slot;
	
	if(!to_empty && !does_artifact_fit_in_slot(slot2, from_slot))
		return false;
	
	if(!from_empty && !does_artifact_fit_in_slot(slot1, to_slot))
	   return false;
	
	
	std::swap(artifacts[from_slot-1], artifacts[to_slot-1]);
	
	return true;
}

bool hero_t::is_inventory_full(artifact_e artifact_id) const {
	auto& artifact = game_config::get_artifact(artifact_id);
	
	for(uint i = 0; i < game_config::HERO_ARTIFACT_SLOTS; i++) {
		if(artifacts[i] != ARTIFACT_NONE)
			continue;
		
		if(does_artifact_fit_in_slot(artifact.slot, i+1))
			return false;
	}
	
	int open_slot = -1;//backpack.get_next_open_slot()
	for(uint i = 0; i < game_config::HERO_BACKPACK_SLOTS; i++) {
		if(backpack[i] == ARTIFACT_NONE)
			return false;
	}

	return true;
}

bool hero_t::pickup_artifact(artifact_e artifact_id) {
	auto& artifact = game_config::get_artifact(artifact_id);
	
	for(uint i = 0; i < game_config::HERO_ARTIFACT_SLOTS; i++) {
		if(artifacts[i] != ARTIFACT_NONE)
			continue;
		
		if(does_artifact_fit_in_slot(artifact.slot, i+1)) {
			artifacts[i] = artifact_id;
			return true;
		}
	}
	
	int open_slot = -1;//backpack.get_next_open_slot()
	for(uint i = 0; i < game_config::HERO_BACKPACK_SLOTS; i++) {
		if(backpack[i] == ARTIFACT_NONE) {
			open_slot = i;
			break;
		}
	}
	
	if(open_slot == -1) //backpack is full
		return false;
	
	backpack[open_slot] = artifact_id;
	return true;
}

uint hero_t::get_total_attack_bonus() const {
	return get_effective_attack();
}

uint hero_t::get_total_defense_bonus() const {
	return get_effective_defense();
}

float hero_t::get_mana_percentage_remaining() const {
	return mana / (float)get_maximum_mana();
}

uint16_t hero_t::get_maximum_mana() const {
	if(hero_class == HERO_BARBARIAN)
		return get_effective_knowledge() * 5;

	float modifier = 1.0f;

	float specialty_bonus = (specialty == SPECIALTY_INTELLIGENCE ? .1f + (0.01 * level) : 0.f);
	modifier += get_skill_value(.2f + specialty_bonus, .05f, get_secondary_skill_level(SKILL_INTELLIGENCE));
	
	return std::round((get_effective_knowledge() * 10) * modifier);
}

uint16_t hero_t::get_daily_mana_regen() const {
	int value = 1;

	if(has_talent(TALENT_REPLENISHMENT))
		value += std::round(get_maximum_mana() * .1f);

	if(is_artifact_in_effect(ARTIFACT_WITCHFIRE_CAULDRON))
		value += std::round(get_maximum_mana() * .05f);

	return value;
}

uint hero_t::get_unit_attack(unit_type_e unit_type) const {
	auto base = game_config::get_creature(unit_type).attack;
	auto bonus = get_total_attack_bonus();

	int value = base + bonus;

	if(has_talent(TALENT_DRAGONBORNE))
		value += (unit_type == UNIT_RED_DRAGON) ? 2 : -1;

	return std::max(0, value);
}

uint hero_t::get_unit_defense(unit_type_e unit_type) const {
	auto base = game_config::get_creature(unit_type).defense;
	auto bonus = get_total_defense_bonus();

	int value = base + bonus;

	if(unit_type == UNIT_DRUID && specialty == SPECIALTY_DRUIDS)
		value += 3;

	if(has_talent(TALENT_DRAGONBORNE))
		value += (unit_type == UNIT_RED_DRAGON) ? 2 : -1;

	return std::max(0, value);
}

uint hero_t::get_unit_magic_resistance(unit_type_e unit_type, magic_damage_e damage_type) const {
	double resistance_skill_value = get_skill_value(.1f, .05f, get_secondary_skill_level(SKILL_RESISTANCE));
	if(specialty == SPECIALTY_RESISTANCE)
		resistance_skill_value += 0.15;

	double vulnerability = 1.0 - resistance_skill_value;
	
	if(has_talent(TALENT_NATURAL_RESISTANCE))
		vulnerability *= 0.8; //20% resistance

	if(has_talent(TALENT_HOLY_WARD))
		vulnerability *= 0.9; //10% resistance

	if(has_talent(TALENT_NATURAL_WARD))
		vulnerability *= 0.85; //15% resistance

	if(has_talent(TALENT_NEGATION))
		vulnerability *= 0.6; //40% resistance
	
	if(damage_type == MAGIC_DAMAGE_FROST && has_talent(TALENT_COLD_BLOODED)) {
		//only undead units are affected by Cold Blooded talent
		const auto& cr = game_config::get_creature(unit_type);
		if(cr.has_inherent_buff(BUFF_UNDEAD))
			vulnerability *= 0.6; //40% resistance
	}
	
	if(is_artifact_in_effect(ARTIFACT_SCALES_OF_THE_RED_DRAGON))
		vulnerability *= 0.7; //30% resistance
	
	if(damage_type == MAGIC_DAMAGE_FIRE && is_artifact_in_effect(ARTIFACT_RUBY_RING))
		vulnerability *= 0.5; //50% resistance
	
	uint total_resistance = (uint)std::round((1.0 - vulnerability) * 100);
	return total_resistance;
}

uint hero_t::get_unit_max_hp(unit_type_e unit_type) const {
	auto hp = game_config::get_creature(unit_type).health;

	if(unit_type == UNIT_WOLF && has_talent(TALENT_PACK_LEADER))
		hp += 10;

	if(unit_type == UNIT_SKELETON && has_talent(TALENT_SKELETAL_FORTITUDE))
		hp += 1;

	if(unit_type == UNIT_TITAN && has_talent(TALENT_TITANIC))
		hp += 50;

	if(is_artifact_in_effect(ARTIFACT_BLOODSTONE_BARRIER))
		hp += std::round(hp * 1.15);

	return hp;
}

uint hero_t::get_unit_min_damage(unit_type_e unit_type) const {
	uint min_damage = game_config::get_creature(unit_type).min_damage;

	if(unit_type == UNIT_GOBLIN && specialty == SPECIALTY_GOBLINS)
		min_damage += 2;
	
	return min_damage;
}

uint hero_t::get_unit_max_damage(unit_type_e unit_type) const {
	uint max_damage = game_config::get_creature(unit_type).max_damage;

	if(unit_type == UNIT_GOBLIN && specialty == SPECIALTY_GOBLINS)
		max_damage += 2;

	return max_damage;
}

uint hero_t::get_unit_speed(unit_type_e unit_type) const {
	auto speed = game_config::get_creature(unit_type).speed;

	if((unit_type == UNIT_ARCANE_CONSTRUCT || unit_type == UNIT_GOLEM) && has_talent(TALENT_OVERDRIVE))
		speed += 2;

	if((unit_type == UNIT_SKELETON || unit_type == UNIT_BONE_WYRM) && has_talent(TALENT_ONSLAUGHT))
		speed += 2;

	if(unit_type == UNIT_SKELETON && specialty == SPECIALTY_SKELETONS)
		speed += 1;

	if(unit_type == UNIT_GOBLIN && specialty == SPECIALTY_GOBLINS)
		speed += 1;

	if(is_artifact_in_effect(ARTIFACT_INFINITY_BAND))
		speed += 1;

	return speed;
}

uint hero_t::get_unit_initiative(unit_type_e unit_type) const {
	auto initiative = game_config::get_creature(unit_type).initiative;

	if((unit_type == UNIT_ARCANE_CONSTRUCT || unit_type == UNIT_GOLEM) && has_talent(TALENT_OVERDRIVE))
		initiative += 1;

	if((unit_type == UNIT_SKELETON || unit_type == UNIT_BONE_WYRM) && has_talent(TALENT_ONSLAUGHT))
		initiative += 1;

	if(unit_type == UNIT_SKELETON && specialty == SPECIALTY_SKELETONS)
		initiative += 1;

	if(unit_type == UNIT_DRUID && specialty == SPECIALTY_DRUIDS)
		initiative += 1;

	if(is_artifact_in_effect(ARTIFACT_INFINITY_BAND))
		initiative += 1;

	return initiative;
}


int hero_t::get_unit_morale(unit_type_e unit_type) const {
	const auto& creature = game_config::get_creature(unit_type);

	if(creature.has_inherent_buff(BUFF_UNDEAD) || has_talent(TALENT_MINDLESS_SUBSERVIENCE))
		return 0;

	int morale = get_morale();

	if(morale <= 0 && creature.has_inherent_buff(BUFF_POSITIVE_MORALE))
		return 1;

	return morale;
}

uint64_t hero_t::get_xp_to_next_level() const {
	return get_xp_to_level(level + 1) - experience;
}

uint64_t hero_t::get_xp_to_level(uint level) {
	if(level > 99) {
		level = 99;
	}
	
	static uint64_t xp_table[100] =
	{ 	0, 0, 1000, 2000, 3200, 4500, 6000, 8000, 10000, 12000, 14000
	};
	
	static bool table_initialized = false;
	if(!table_initialized) {
		for(uint i = 10; i < 100; i++) {
			xp_table[i] = xp_table[i-1] * 1.12f;
		}
		table_initialized = true;
	}
	
	return xp_table[level];
}

uint hero_t::get_number_of_open_skill_slots() const {
	uint count = 0;
	for(auto& sk : skills) {
		if(sk.skill == SKILL_NONE)
			count++;
	}
	
	return count;
}

bool hero_t::are_secondary_skills_full_and_max_level() const {
	for(auto& sk : skills) {
		if(sk.skill == SKILL_NONE || sk.level < 4)
			return false;
	}
	
	return true;
}

bool hero_t::can_learn_skill(skill_e skill) const {
	return utils::contains(enabled_skills, skill);
}

stat_e hero_t::level_up_stats() {
	stat_e stat;
	stat = (stat_e)(1 + (rand() % 4));
	//todo: tables for class/level
	switch(stat) {
		case STAT_ATTACK:
			increase_attack(1);
			break;
		case STAT_DEFENSE:
			increase_defense(1);
			break;
		case STAT_POWER:
			increase_power(1);
			break;
		case STAT_KNOWLEDGE:
			increase_knowledge(1);
			break;
		case STAT_NONE:
			break;
	}
	return stat;
}

std::array<skill_e, 3> hero_t::get_level_up_skills() const {
	std::array<skill_e, 3> skills_to_upgrade = { SKILL_NONE, SKILL_NONE, SKILL_NONE };
	
	//first, pick one already existing skill to upgrade:
	std::vector<skill_e> potential_upgrades;
	for(auto& sk : skills) {
		if(sk.skill != SKILL_NONE && sk.level < 4)
			potential_upgrades.push_back(sk.skill);
	}
	uint filled = 0;
	auto seed = (unsigned)std::chrono::system_clock::now().time_since_epoch().count();
	if(potential_upgrades.size()) {
		std::shuffle(potential_upgrades.begin(), potential_upgrades.end(), std::default_random_engine(seed));
		skills_to_upgrade[0] = potential_upgrades.back();
		potential_upgrades.pop_back();
		filled++;
	}
	
	//now, pick up to 2 new skills
	//there is an unusual case here where a hero has no existing skills to upgrade.  if that
	//is the case, we need to offer 3 new skills
	uint new_skill_count = 2;
	if(potential_upgrades.empty())
		new_skill_count = 3;
	
	int new_skills = std::min(new_skill_count, get_number_of_open_skill_slots());
	for(int i = 0; i < new_skills; i++) {
		int total = 0;
		for(auto& sk : game_config::get_skills()) {
			//disregard already known skills
			bool already_have_skill = false;
			for(auto& ksk : skills) {
				if(ksk.skill == sk.skill_id)
					already_have_skill = true;
			}
			for(auto& psk : skills_to_upgrade) {
				if(psk == sk.skill_id)
					already_have_skill = true;
			}
			if(already_have_skill)
				continue;
			
			total += sk.class_learnrate[hero_class];
		}
		
		int randval = rand() % total;
		int cumulative = 0;
		for(auto& sk : game_config::get_skills()) {
			//disregard already known skills
			bool already_have_skill = false;
			for(auto& ksk : skills) {
				if(ksk.skill == sk.skill_id)
					already_have_skill = true;
			}
			for(auto& psk : skills_to_upgrade) {
				if(psk == sk.skill_id)
					already_have_skill = true;
			}
			if(already_have_skill)
				continue;
			
			if(filled >= skills_to_upgrade.size())
				break;
			
			cumulative += sk.class_learnrate[hero_class];
			if(randval < cumulative) {
				skills_to_upgrade[filled++] = sk.skill_id;
				break;
			}
		}
	}
	
	//in the case that there are fewer than 2 new skill slots, fill in the remaining
	//options with skills to be upgraded
	for(int i = filled; i < 3; i++) {
		if(potential_upgrades.size()) {
			skills_to_upgrade[i] = potential_upgrades.back();
			potential_upgrades.pop_back();
		}
	}
	
	return skills_to_upgrade;
}

void hero_t::set_visited_object(interactable_object_t* object, bool visited) {
	uint32_t offset = (object->x << 16) | object->y;
	bool exists = utils::contains(visited_objects, offset);
	
	//if we've visited the object and it's in the visited objects list, or
	//if we haven't visited the object and it's not in the visited objects list, do nothing
	if((visited && exists) || (!visited && !exists))
		return;
	
	if(visited && !exists)
		visited_objects.push_back(offset);
//	else if(!visited && exists)
//		std::erase(visited_objects, offset); //std::remove(visited_objects.begin(), visited_objects.end(), offset);
}

bool hero_t::has_object_been_visited(interactable_object_t* object) const {
	if(!object)
		return false;
	
	uint32_t offset = (object->x << 16) | object->y;
	return utils::contains(visited_objects, offset);
}
