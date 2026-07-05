#include "core/interactable_object.h"
#include "core/adventure_map.h"
#include "core/utils.h"
#include "core/utils_enum.h"

#include "core/qt_headers.h"

QDataStream& operator<<(QDataStream& stream, const interactable_object_t& object) {
	stream << object.asset_id << object.object_type << object.x << object.y;
	return stream;
}

QDataStream& operator>>(QDataStream& stream, interactable_object_t& object) {
	stream >> object.asset_id >> object.object_type >> object.x >> object.y;
	return stream;
}

bool interactable_object_t::is_pickupable(interactable_object_e object_type) {
	return (object_type == OBJECT_RESOURCE
			|| object_type == OBJECT_TREASURE_CHEST
			|| object_type == OBJECT_ARTIFACT
			|| object_type == OBJECT_CAMPFIRE
			|| object_type == OBJECT_SCHOLAR
			|| object_type == OBJECT_SCHOLAR
			|| object_type == OBJECT_PANDORAS_BOX
			|| object_type == OBJECT_FLOATSAM
			|| object_type == OBJECT_SEA_CHEST
			|| object_type == OBJECT_SHIPWRECK_SURVIVOR
			|| object_type == OBJECT_SEA_BARRELS);
}

bool interactable_object_t::is_pickupable(interactable_object_t* object) {
	if(!object)
		return false;
	
	return is_pickupable(object->object_type);
}

void interactable_object_t::copy_interactable_object(interactable_object_t* left, const interactable_object_t* right) {
	assert(left->object_type == right->object_type);
	switch(left->object_type) {
		case OBJECT_MAP_TOWN: *((town_t*)left) = *((town_t*)right); break;
		case OBJECT_SIGN: *((sign_t*)left) = *((sign_t*)right); break;
		case OBJECT_PYRAMID: *((pyramid_t*)left) = *((pyramid_t*)right); break;
		case OBJECT_RESOURCE: *((map_resource_t*)left) = *((map_resource_t*)right); break;
		case OBJECT_ARTIFACT: *((map_artifact_t*)left) = *((map_artifact_t*)right); break;
		case OBJECT_MAP_MONSTER: *((map_monster_t*)left) = *((map_monster_t*)right); break;
		case OBJECT_MINE: *((mine_t*)left) = *((mine_t*)right); break;
		case OBJECT_TREASURE_CHEST: *((treasure_chest_t*)left) = *((treasure_chest_t*)right); break;
		case OBJECT_MAGIC_SHRINE: *((shrine_t*)left) = *((shrine_t*)right); break;
		case OBJECT_WARRIORS_TOMB: *((warriors_tomb_t*)left) = *((warriors_tomb_t*)right); break;
		case OBJECT_REFUGEE_CAMP: *((refugee_camp_t*)left) = *((refugee_camp_t*)right); break;
		case OBJECT_WITCH_HUT: *((witch_hut_t*)left) = *((witch_hut_t*)right); break;
		case OBJECT_BLACKSMITH: *((blacksmith_t*)left) = *((blacksmith_t*)right); break;
		case OBJECT_TELEPORTER: *((teleporter_t*)left) = *((teleporter_t*)right); break;
		case OBJECT_PANDORAS_BOX: *((pandoras_box_t*)left) = *((pandoras_box_t*)right); break;
		case OBJECT_SCHOLAR: *((scholar_t*)left) = *((scholar_t*)right); break;
		case OBJECT_GRAVEYARD: *((graveyard_t*)left) = *((graveyard_t*)right); break;
		case OBJECT_MONUMENT: *((monument_t*)left) = *((monument_t*)right); break;
		case OBJECT_CAMPFIRE: *((campfire_t*)left) = *((campfire_t*)right); break;
		case OBJECT_WAGON: *((wagon_t*)left) = *((wagon_t*)right); break;
		case OBJECT_EYE_OF_THE_MAGI: *((eye_of_the_magi_t*)left) = *((eye_of_the_magi_t*)right); break;
		case OBJECT_WATERWHEEL:
		case OBJECT_WINDMILL: *((flaggable_object_t*)left) = *((flaggable_object_t*)right); break;
		default:
			*left = *right;
	}
}

interactable_object_t* _make_object(interactable_object_e object_type) {
	switch(object_type) {
		case OBJECT_MAP_TOWN:
			return new town_t;
		case OBJECT_SIGN:
			return new sign_t;
		case OBJECT_PYRAMID:
			return new pyramid_t;
		case OBJECT_RESOURCE:
			return new map_resource_t;
		case OBJECT_ARTIFACT:
			return new map_artifact_t;
		case OBJECT_MAP_MONSTER:
			return new map_monster_t;
		case OBJECT_MINE:
			return new mine_t;
		case OBJECT_TREASURE_CHEST:
			return new treasure_chest_t;
		case OBJECT_MAGIC_SHRINE:
			return new shrine_t;
		case OBJECT_WITCH_HUT:
			return new witch_hut_t;
		case OBJECT_BLACKSMITH:
			return new blacksmith_t;
		case OBJECT_TELEPORTER:
			return new teleporter_t;
		case OBJECT_PANDORAS_BOX:
			return new pandoras_box_t;
		case OBJECT_SCHOLAR:
			return new scholar_t;
		case OBJECT_WARRIORS_TOMB:
			return new warriors_tomb_t;
		case OBJECT_REFUGEE_CAMP:
			return new refugee_camp_t;
		case OBJECT_GRAVEYARD:
			return new graveyard_t;
		case OBJECT_MONUMENT:
			return new monument_t;
		case OBJECT_CAMPFIRE:
			return new campfire_t;
		case OBJECT_WAGON:
			return new wagon_t;
		case OBJECT_EYE_OF_THE_MAGI:
			return new eye_of_the_magi_t;
		case OBJECT_KEYMASTER_TENT:
			return new keymaster_tent_t;
		case OBJECT_WATERWHEEL:
		case OBJECT_WINDMILL:
		case OBJECT_WATCHTOWER:
			return new flaggable_object_t;
		default:
			return new interactable_object_t;

	}

	return nullptr;
}


#include <bitset>

//utility function to convert bitset to JSON array
template<size_t N>
QJsonArray bitset_to_json_array(const std::bitset<N>& bitset) {
	QJsonArray array;
	for(size_t i = 0; i < N; ++i) {
		array.append(bitset.test(i));
	}
	return array;
}

inline troop_group_t troops_from_json_array(const QJsonArray& troopsArray) {
	troop_group_t troops;
	size_t i = 0;

	for (const auto& troopValue : troopsArray) {
		if(i < troops.size() && troopValue.isObject()) {
			QJsonObject troopJson = troopValue.toObject();
			troop_t troop;
			troop.unit_type = utils::get_enum_value<unit_type_e>(troopJson["unit_type"].toString());
			troop.stack_size = troopJson["stack_size"].toInt();
			troops[i] = troop;
		}
	}
	return troops;
}

inline QJsonArray troops_to_json_array(const troop_group_t& troops) {
	QJsonArray troopsArray;

	for(const auto& t : troops) {
		QJsonObject troopJson;
		troopJson["unit_type"] = utils::name_from_enum(t.unit_type);
		troopJson["stack_size"] = t.stack_size;
		troopsArray.append(troopJson);
	}
	return troopsArray;
}

//helper function for getting the last (highest) value of an enum
template<typename T> T max_enum_val() {
	return (T)(magic_enum::enum_values<T>().back());
}

//template function for reading (up to a specified number of bytes) fields of a derived class of interactable_object_t to a datastream (e.g. map file or network)
template<typename T> void read_clamped(QDataStream& stream, T& field, const T& min_value, const T& max_value) {
	stream >> field;
	field = std::clamp(field, min_value, max_value);
}

interactable_object_t* interactable_object_t::make_new_object(interactable_object_e object_type) {
	if(object_type > max_enum_val<interactable_object_e>())
		return nullptr;

	auto obj = _make_object(object_type);
	if(obj)
		obj->object_type = object_type;
	
	return obj;
}

const std::string interactable_object_t::get_name(interactable_object_e type) {
	return magic_enum::enum_name(type).data();
}


//flaggable_object_t
void flaggable_object_t::read_data(QDataStream& stream) {
	read_clamped(stream, owner, PLAYER_NONE, max_enum_val<player_e>());
	read_clamped(stream, visited, (uint8_t)0u, (uint8_t)1u);
	read_clamped(stream, date_visited, (int16_t)-1, (int16_t)30000);
}
void flaggable_object_t::write_data(QDataStream& stream) const {
	stream << owner << visited << date_visited;
}
int flaggable_object_t::read_data_json(QJsonObject json) {
	owner = utils::get_enum_value<player_e>(json["owner"].toString());
	visited = json["visited"].toInt();
	date_visited = json["date_visited"].toInt();
	return 0;
}
QJsonObject flaggable_object_t::write_data_json() const {
	QJsonObject json;
	json["owner"] = utils::name_from_enum(owner);
	json["visited"] = visited;
	json["date_visited"] = date_visited;
	return json;
}

//graveyard_t
void graveyard_t::read_data(QDataStream& stream) {
	read_clamped(stream, visited, (uint8_t)0u, (uint8_t)1u);
	read_clamped(stream, size, (uint8_t)0u, (uint8_t)4u);
	read_clamped(stream, artifact_reward, ARTIFACT_NONE, max_enum_val<artifact_e>());
}
void graveyard_t::write_data(QDataStream& stream) const {
	stream << visited << size << artifact_reward;
}
int graveyard_t::read_data_json(QJsonObject json) {
	visited = json["visited"].toInt();
	size = json["size"].toInt();
	artifact_reward = utils::get_enum_value<artifact_e>(json["artifact_reward"].toString());
	return 0;
}
QJsonObject graveyard_t::write_data_json() const {
	QJsonObject json;
	json["visited"] = visited;
	json["size"] = size;
	json["artifact_reward"] = artifact_reward;
	return json;
}

//teleporter_t
void teleporter_t::read_data(QDataStream& stream) {
	read_clamped(stream, teleporter_type, TELEPORTER_TYPE_TWO_WAY, max_enum_val<teleporter_type_e>());
	read_clamped(stream, color, (uint8_t)0u, (uint8_t)16u);
	read_clamped(stream, is_blockable, (uint8_t)0u, (uint8_t)1u);
}
void teleporter_t::write_data(QDataStream& stream) const {
	stream << teleporter_type << color << is_blockable;
}
int teleporter_t::read_data_json(QJsonObject json) {
	teleporter_type = utils::get_enum_value<teleporter_type_e>(json["type"].toString());
	color = json["color"].toInt();
	is_blockable = json["is_blockable"].toBool();
	return 0;
}
QJsonObject teleporter_t::write_data_json() const {
	QJsonObject json;
	json["teleporter_type"] = utils::name_from_enum(teleporter_type);
	json["color"] = color;
	json["is_blockable"] = is_blockable;
	return json;
}

//blacksmith_t
void blacksmith_t::read_data(QDataStream& stream) {
	read_clamped(stream, visited, (uint8_t)0u, (uint8_t)1u);
	read_clamped(stream, offers_upgrade, (uint8_t)0u, (uint8_t)1u);
	read_clamped(stream, choice1, ARTIFACT_NONE, max_enum_val<artifact_e>());
	read_clamped(stream, choice2, ARTIFACT_NONE, max_enum_val<artifact_e>());
}
void blacksmith_t::write_data(QDataStream& stream) const {
	stream << visited << offers_upgrade << choice1 << choice2;
}
int blacksmith_t::read_data_json(QJsonObject json) {
	visited = json["visited"].toInt();
	offers_upgrade = json["offers_upgrade"].toBool();
	choice1 = utils::get_enum_value<artifact_e>(json["choice1"].toString());
	choice2 = utils::get_enum_value<artifact_e>(json["choice2"].toString());
	return 0;
}
QJsonObject blacksmith_t::write_data_json() const {
	QJsonObject json;
	json["visited"] = visited;
	json["offers_upgrade"] = offers_upgrade;
	json["choice1"] = utils::name_from_enum(choice1);
	json["choice2"] = utils::name_from_enum(choice2);
	return json;
}

//sign_t
void sign_t::read_data(QDataStream& stream) {
	text = stream_read_string(stream);
	quint8 val;
	stream >> val;
	visited = val;
}
void sign_t::write_data(QDataStream& stream) const {
	stream_write_string(stream, text);
	stream << (quint8)visited.to_ulong();
}
int sign_t::read_data_json(QJsonObject json) {
	text = json["text"].toString().toStdString();
	visited = std::bitset<8>(json["visited"].toInt());
	return 0;
}
QJsonObject sign_t::write_data_json() const {
	QJsonObject json;
	json["text"] = QString::fromStdString(text).toStdString().c_str();
	json["visited"] = static_cast<int>(visited.to_ulong());
	return json;
}

//monument_t
void monument_t::read_data(QDataStream& stream) {
	stream >> monument_type;
}
void monument_t::write_data(QDataStream& stream) const {
	stream << monument_type;
}
int monument_t::read_data_json(QJsonObject json) {
	monument_type = (monument_type_e)json["monument_type"].toInt();
	return 0;
}
QJsonObject monument_t::write_data_json() const {
	QJsonObject json;
	json["monument_type"] = monument_type;
	return json;
}

//pyramid_t
void pyramid_t::read_data(QDataStream& stream) {
	read_clamped(stream, spell_id, SPELL_UNKNOWN, max_enum_val<spell_e>());
	read_clamped(stream, visited, (uint8_t)0u, (uint8_t)1u);
	stream_read_vector(stream, available_spells, SPELL_UNKNOWN, max_enum_val<spell_e>());
}
void pyramid_t::write_data(QDataStream& stream) const {
	stream << spell_id << visited;
	stream_write_vector(stream, available_spells);
}
int pyramid_t::read_data_json(QJsonObject json) {
	spell_id = utils::get_enum_value<spell_e>(json["spell_id"].toString());
	visited = json["visited"].toBool();

	available_spells.clear();
	QJsonArray spellsArray = json["available_spells"].toArray();
	for(const auto& spellValue : spellsArray)
	{
		available_spells.push_back(utils::get_enum_value<spell_e>(spellValue.toString()));
	}
	return 0;
}
QJsonObject pyramid_t::write_data_json() const {
	QJsonObject json;
	json["spell_id"] = utils::name_from_enum(spell_id);
	json["visited"] = visited;

	QJsonArray spellsArray;
	for(const auto& spell : available_spells)
	{
		spellsArray.append(utils::name_from_enum(spell));
	}
	json["available_spells"] = spellsArray;

	return json;
}

//map_resource_t
void map_resource_t::read_data(QDataStream& stream) {
	read_clamped(stream, min_quantity, (uint16_t)0u, (uint16_t)10000u);
	read_clamped(stream, max_quantity, (uint16_t)0u, (uint16_t)10000u);
	read_clamped(stream, resource_type, RESOURCE_RANDOM, max_enum_val<resource_e>());
}
void map_resource_t::write_data(QDataStream& stream) const {
	stream << min_quantity << max_quantity << resource_type;
}
int map_resource_t::read_data_json(QJsonObject json) {
	min_quantity = json["min_quantity"].toInt();
	max_quantity = json["max_quantity"].toInt();
	resource_type = utils::get_enum_value<resource_e>(json["resource_type"].toString());
	return 0;
}
QJsonObject map_resource_t::write_data_json() const {
	QJsonObject json;
	json["min_quantity"] = min_quantity;
	json["max_quantity"] = max_quantity;
	json["resource_type"] = utils::name_from_enum(resource_type);
	return json;
}

//map_artifact_t
void map_artifact_t::read_data(QDataStream& stream) {
	read_clamped(stream, artifact_id, ARTIFACT_NONE, (artifact_e)0xffff /*max_enum_val<artifact_e>()*/);
	read_clamped(stream, rarity, RARITY_UNKNOWN, max_enum_val<artifact_rarity_e>());
	read_clamped(stream, can_be_highlighted, (uint8_t)0u, (uint8_t)1u);
	read_clamped(stream, show_full_name, (uint8_t)0u, (uint8_t)1u);
	custom_pickup_message = stream_read_string(stream);
}
void map_artifact_t::write_data(QDataStream& stream) const {
	stream << artifact_id << rarity << can_be_highlighted << show_full_name;
	stream_write_string(stream, custom_pickup_message);
}
int map_artifact_t::read_data_json(QJsonObject json) {
	artifact_id = utils::get_enum_value<artifact_e>(json["artifact_id"].toString());
	rarity = utils::get_enum_value<artifact_rarity_e>(json["rarity"].toString());
	can_be_highlighted = json["can_be_highlighted"].toInt();
	show_full_name = json["show_full_name"].toInt();
	custom_pickup_message = json["custom_pickup_message"].toString().toStdString();
	return 0;
}
QJsonObject map_artifact_t::write_data_json() const {
	QJsonObject json;
	json["artifact_id"] = utils::name_from_enum(artifact_id);
	json["rarity"] = utils::name_from_enum(rarity);
	json["can_be_highlighted"] = can_be_highlighted;
	json["show_full_name"] = show_full_name;
	json["custom_pickup_message"] = QString::fromStdString(custom_pickup_message).toStdString().c_str();
	return json;
}

//map_monster_t
void map_monster_t::read_data(QDataStream& stream) {
	read_clamped(stream, unit_type, UNIT_UNKNOWN, max_enum_val<unit_type_e>());
	read_clamped(stream, quantity, (uint16_t)0u, (uint16_t)50000u);
	read_clamped(stream, grows, (uint8_t)0u, (uint8_t)1u);
	read_clamped(stream, ranged_melee, (uint8_t)0u, (uint8_t)2u);
	read_clamped(stream, tier, (uint8_t)0u, (uint8_t)game_config::CREATURE_TIERS);
	message = stream_read_string(stream, 1024);
}
void map_monster_t::write_data(QDataStream& stream) const {
	stream << unit_type << quantity << grows << ranged_melee << tier;
	stream_write_string(stream, message);
}
int map_monster_t::read_data_json(QJsonObject json) {
	unit_type = utils::get_enum_value<unit_type_e>(json["unit_type"].toString());
	quantity = json["quantity"].toInt();
	grows = json["grows"].toBool();
	ranged_melee = json["ranged_melee"].toInt();
	tier = json["tier"].toInt();
	message = json["message"].toString().toStdString();
	return 0;
}
QJsonObject map_monster_t::write_data_json() const {
	QJsonObject json;
	json["unit_type"] = utils::name_from_enum(unit_type);
	json["quantity"] = quantity;
	json["grows"] = grows;
	json["ranged_melee"] = ranged_melee;
	json["tier"] = tier;
	json["message"] = QString::fromStdString(message).toStdString().c_str();
	return json;
}

//mine_t
void mine_t::read_data(QDataStream& stream) {
	read_clamped(stream, mine_type, RESOURCE_RANDOM, max_enum_val<resource_e>());
	read_clamped(stream, owner, PLAYER_NONE, max_enum_val<player_e>());
}
void mine_t::write_data(QDataStream& stream) const {
	stream << mine_type << owner;
}
int mine_t::read_data_json(QJsonObject json) {
	mine_type = utils::get_enum_value<resource_e>(json["mine_type"].toString());
	owner = utils::get_enum_value<player_e>(json["owner"].toString());
	return 0;
}
QJsonObject mine_t::write_data_json() const {
	QJsonObject json;
	json["mine_type"] = utils::name_from_enum(mine_type);
	json["owner"] = utils::name_from_enum(owner);
	return json;
}

//shrine_t
void shrine_t::read_data(QDataStream& stream) {
	read_clamped(stream, spell_level, (uint8_t)0u, (uint8_t)5u);
	read_clamped(stream, spell, SPELL_UNKNOWN, max_enum_val<spell_e>());
	stream_read_vector(stream, available_spells);
}
void shrine_t::write_data(QDataStream& stream) const {
	stream << spell_level << spell;
	stream_write_vector(stream, available_spells);
}
int shrine_t::read_data_json(QJsonObject json) {
	spell_level = json["spell_level"].toInt();
	spell = utils::get_enum_value<spell_e>(json["spell"].toString());

	available_spells.clear();
	QJsonArray spellsArray = json["available_spells"].toArray();
	for(const auto& spellValue : spellsArray)
	{
		available_spells.push_back(utils::get_enum_value<spell_e>(spellValue.toString()));
	}
	return 0;
}
QJsonObject shrine_t::write_data_json() const {
	QJsonObject json;
	json["spell_level"] = spell_level;
	json["spell"] = utils::name_from_enum(spell);

	QJsonArray spellsArray;
	for(const auto& sp : available_spells)
	{
		spellsArray.append(utils::name_from_enum(sp));
	}
	json["available_spells"] = spellsArray;

	return json;
}

//warriors_tomb_t
void warriors_tomb_t::read_data(QDataStream& stream) {
	read_clamped(stream, visited, (uint8_t)0u, (uint8_t)1u);
	read_clamped(stream, artifact, ARTIFACT_NONE, max_enum_val<artifact_e>());
	read_clamped(stream, visited_by_player, (uint8_t)0u, (uint8_t)1u);
}
void warriors_tomb_t::write_data(QDataStream& stream) const {
	stream << visited << artifact << visited_by_player;
}
int warriors_tomb_t::read_data_json(QJsonObject json) {
	visited = json["visited"].toInt();
	visited_by_player = json["visited_by_player"].toInt();
	artifact = utils::get_enum_value<artifact_e>(json["artifact"].toString());
	return 0;
}
QJsonObject warriors_tomb_t::write_data_json() const {
	QJsonObject json;
	json["visited_by_player"] = visited_by_player;
	json["visited"] = visited;
	json["artifact"] = utils::name_from_enum(artifact);
	return json;
}

//refugee_camp_t
void refugee_camp_t::read_data(QDataStream& stream) {
	stream >> available_troops;
}
void refugee_camp_t::write_data(QDataStream& stream) const {
	stream << available_troops;
}
int refugee_camp_t::read_data_json(QJsonObject json) {
	QJsonObject troopJson = json["available_troops"].toObject();
	available_troops.unit_type = utils::get_enum_value<unit_type_e>(troopJson["unit_type"].toString());
	available_troops.stack_size = troopJson["stack_size"].toInt();
	return 0;
}
QJsonObject refugee_camp_t::write_data_json() const {
	QJsonObject troopJson;
	troopJson["unit_type"] = utils::name_from_enum(available_troops.unit_type);
	troopJson["stack_size"] = available_troops.stack_size;
	QJsonObject json;
	json["available_troops"] = troopJson;
	return json;
}

//witch_hut_t
void witch_hut_t::read_data(QDataStream& stream) {
	read_clamped(stream, skill, SKILL_NONE, max_enum_val<skill_e>());
	read_clamped(stream, allows_upgrade, (uint8_t)0u, (uint8_t)1u);
	stream_read_vector(stream, available_skills, SKILL_NONE, max_enum_val<skill_e>());
}
void witch_hut_t::write_data(QDataStream& stream) const {
	stream << skill << allows_upgrade;
	stream_write_vector(stream, available_skills);
}
int witch_hut_t::read_data_json(QJsonObject json) {
	skill = utils::get_enum_value<skill_e>(json["skill"].toString());
	allows_upgrade = json["allows_upgrade"].toBool();

	available_skills.clear();
	QJsonArray skillsArray = json["available_skills"].toArray();
	for(const auto& skillValue : skillsArray)
	{
		available_skills.push_back(utils::get_enum_value<skill_e>(skillValue.toString()));
	}
	return 0;
}
QJsonObject witch_hut_t::write_data_json() const {
	QJsonObject json;
	json["skill"] = utils::name_from_enum(skill);
	json["allows_upgrade"] = allows_upgrade;

	QJsonArray skillsArray;
	for(const auto& sk : available_skills)
	{
		skillsArray.append(utils::name_from_enum(sk));
	}
	json["available_skills"] = skillsArray;

	return json;
}

//pandoras_box_t
void pandoras_box_t::read_data(QDataStream& stream) {
	custom_message = stream_read_string(stream);
	stream_read_array(stream, guardians);
	stream_read_vector(stream, rewards);
}
void pandoras_box_t::write_data(QDataStream& stream) const {
	stream_write_string(stream, custom_message);
	stream_write_vector(stream, guardians);
	stream_write_vector(stream, rewards);
}
int pandoras_box_t::read_data_json(QJsonObject json) {
	custom_message = json["custom_message"].toString().toStdString();
	guardians = troops_from_json_array(json["guardians"].toArray());

	rewards.clear();
	QJsonArray rewardsArray = json["rewards"].toArray();
	for(const auto& rewardValue : rewardsArray)
	{
		QJsonObject rewardJson = rewardValue.toObject();
		reward_t reward;
		reward.type = utils::get_enum_value<reward_e>(rewardJson["type"].toString());
		reward.subtype = rewardJson["subtype"].toInt();
		reward.magnitude = rewardJson["magnitude"].toInt();
		rewards.push_back(reward);
	}
	return 0;
}
QJsonObject pandoras_box_t::write_data_json() const {
	QJsonObject json;
	json["custom_message"] = QString::fromStdString(custom_message).toStdString().c_str();
	json["guardians"] = troops_to_json_array(guardians);

	QJsonArray rewardsArray;
	for(const auto& reward : rewards)
	{
		QJsonObject rewardJson;
		rewardJson["type"] = utils::name_from_enum(reward.type);
		rewardJson["subtype"] = reward.subtype;
		rewardJson["magnitude"] = (int)reward.magnitude;
		rewardsArray.append(rewardJson);
	}
	json["rewards"] = rewardsArray;

	return json;
}

//scholar_t
void scholar_t::read_data(QDataStream& stream) {
	stream >> reward_type >> reward_subtype.stat >> reward_property.magnitude;
}
void scholar_t::write_data(QDataStream& stream) const {
	stream << reward_type << reward_subtype.stat << reward_property.magnitude;
}
int scholar_t::read_data_json(QJsonObject json) {
	reward_type = utils::get_enum_value<reward_e>(json["reward_type"].toString());
	reward_subtype.stat = utils::get_enum_value<stat_e>(json["reward_subtype"].toString());
	reward_property.magnitude = json["reward_property"].toInt();
	return 0;
}
QJsonObject scholar_t::write_data_json() const {
	QJsonObject json;
	json["reward_type"] = utils::name_from_enum(reward_type);
	json["reward_subtype"] = utils::name_from_enum(reward_subtype.stat); // Assuming stat is the default active member
	json["reward_property"] = reward_property.magnitude; // Adjust based on active member
	return json;
}

//eye_of_the_magi_t
void eye_of_the_magi_t::read_data(QDataStream& stream) {
	stream >> reveal_radius;
}
void eye_of_the_magi_t::write_data(QDataStream& stream) const {
	stream << reveal_radius;
}
int eye_of_the_magi_t::read_data_json(QJsonObject json) {
	reveal_radius = json["reveal_radius"].toInt();
	return 0;
}
QJsonObject eye_of_the_magi_t::write_data_json() const {
	QJsonObject json;
	json["reveal_radius"] = reveal_radius;
	return json;
}

//keymaster_tent_t
void keymaster_tent_t::read_data(QDataStream& stream) {
	read_clamped(stream, color, (uint8_t)0u, (uint8_t)16u);
}
void keymaster_tent_t::write_data(QDataStream& stream) const {
	stream << color;
}
int keymaster_tent_t::read_data_json(QJsonObject json) {
	color = json["color"].toInt();
	return 0;
}
QJsonObject keymaster_tent_t::write_data_json() const {
	QJsonObject json;
	json["color"] = color;
	return json;
}

//gate_t
void gate_t::read_data(QDataStream& stream) {
	read_clamped(stream, color, (uint8_t)0u, (uint8_t)16u);
	stream >> is_open;
}
void gate_t::write_data(QDataStream& stream) const {
	stream << color << is_open;
}
int gate_t::read_data_json(QJsonObject json) {
	color = json["color"].toInt();
	is_open = json["is_open"].toInt();
	return 0;
}
QJsonObject gate_t::write_data_json() const {
	QJsonObject json;
	json["color"] = color;
	json["is_open"] = is_open;
	return json;
}

//treasure_chest_t
void treasure_chest_t::read_data(QDataStream& stream) {
	stream >> value;
}
void treasure_chest_t::write_data(QDataStream& stream) const {
	stream << value;
}
int treasure_chest_t::read_data_json(QJsonObject json) {
	value = json["value"].toInt();
	return 0;
}
QJsonObject treasure_chest_t::write_data_json() const {
	QJsonObject json;
	json["value"] = value;
	return json;
}

//campfire_t
void campfire_t::read_data(QDataStream& stream) {
	stream >> gold_value >> resource_value >> resource_type;
}
void campfire_t::write_data(QDataStream& stream) const {
	stream << gold_value << resource_value << resource_type;
}
int campfire_t::read_data_json(QJsonObject json) {
	gold_value = json["gold_value"].toInt();
	resource_value = json["resource_value"].toInt();
	resource_type = utils::get_enum_value<resource_e>(json["resource_type"].toString());
	return 0;
}
QJsonObject campfire_t::write_data_json() const {
	QJsonObject json;
	json["gold_value"] = gold_value;
	json["resource_value"] = resource_value;
	json["resource_type"] = utils::name_from_enum(resource_type);
	return json;
}

//wagon_t
void wagon_t::read_data(QDataStream& stream) {
	stream >> reward_type >> gold_resource_value >> resource_type >> artifact_id >> visited_by_player;
}
void wagon_t::write_data(QDataStream& stream) const {
	stream << reward_type << gold_resource_value << resource_type << artifact_id << visited_by_player;
}
int wagon_t::read_data_json(QJsonObject json) {
	reward_type = utils::get_enum_value<reward_e>(json["reward_type"].toString());
	gold_resource_value = json["gold_resource_value"].toInt();
	visited_by_player = json["visited_by_player"].toInt();
	resource_type = utils::get_enum_value<resource_e>(json["resource_type"].toString());
	artifact_id = utils::get_enum_value<artifact_e>(json["artifact_id"].toString());
	return 0;
}
QJsonObject wagon_t::write_data_json() const {
	QJsonObject json;
	json["visited_by_player"] = visited_by_player;
	json["reward_type"] = reward_type;
	json["gold_resource_value"] = gold_resource_value;
	json["resource_type"] = utils::name_from_enum(resource_type);
	json["artifact_id"] = utils::name_from_enum(artifact_id);
	return json;
}
