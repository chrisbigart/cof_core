#include "core/map_file.h"

#include "core/qt_headers.h"

using utils::get_enum_value;
using utils::name_from_enum;

std::string get_error_string_from_error_code(map_error_e error_code) {
	auto err = magic_enum::enum_name(error_code);
	return err.data();
}

map_error_e validate_map_file_header(const map_file_header_t& header) {
	if(header.version_major < 1 || header.version_major > 2)
		return MAP_VERSION_INVALID;
	if(header.version_minor > 10)
		return MAP_VERSION_INVALID;
	if(header.width < MINIMUM_MAP_WIDTH)
		return MAP_WIDTH_TOO_SMALL;
	if(header.width > MAXIMUM_MAP_WIDTH)
		return MAP_WIDTH_TOO_LARGE;
	if(header.height < MINIMUM_MAP_HEIGHT)
		return MAP_HEIGHT_TOO_SMALL;
	if(header.height > MAXIMUM_MAP_HEIGHT)
		return MAP_HEIGHT_TOO_LARGE;
	if(header.players < 1 || header.players > 8)
		return MAP_PLAYER_COUNT_INVALID;
	if(header.difficulty > 4)
		return MAP_DIFFICULTY_INVALID;
	if(header.win_condition > 10)
		return MAP_WIN_CONDITION_INVALID;
	if(header.loss_condition > 10)
		return MAP_LOSS_CONDITION_INVALID;

	return SUCCESS;
}

map_error_e read_map_file_header_c(const std::string& filename, map_file_header_t& header) {
	FILE* fp = fopen(filename.c_str(), "rb");
	if(!fp)
		return MAP_COULD_NOT_OPEN_FILE;

	if(fread(&header, MAP_FILE_HEADER_SIZE, 1, fp) != MAP_FILE_HEADER_SIZE) {
		fclose(fp);
		return MAP_COULD_NOT_READ_HEADER;
	}

	fclose(fp);

	return validate_map_file_header(header);
}

map_error_e read_map_file_header_json(const std::string& filepath, map_file_header_t& header) {
	QFile file(QString::fromStdString(filepath));
	if(!file.open(QIODevice::ReadOnly))
		return MAP_COULD_NOT_OPEN_FILE;
	
	auto bytes = file.readAll();
	auto json_document = QJsonDocument::fromJson(bytes);
	
	if(!json_document.isObject()) {
		return MAP_JSON_FORMAT_ERROR;
	}
	
	QJsonObject json = json_document.object();
	
	if (!json.contains("header") || !json["header"].isObject()) {
		return MAP_JSON_FORMAT_ERROR;
	}
	
	if (json.contains("header") && json["header"].isObject()) {
		QJsonObject headerObj = json["header"].toObject();
		header.width = headerObj["width"].toInt();
		header.height = headerObj["height"].toInt();
		
		std::string name = headerObj["name"].toString().toStdString();
		strncpy(header.name, name.c_str(), sizeof(header.name) - 1);
		header.name[sizeof(header.name) - 1] = '\0';
		
		std::string description = headerObj["description"].toString().toStdString();
		strncpy(header.description, description.c_str(), sizeof(header.description) - 1);
		header.description[sizeof(header.description) - 1] = '\0';
		
		header.players = headerObj["players"].toInt();
		header.difficulty = headerObj["difficulty"].toInt();
		header.version_major = headerObj["version_major"].toInt();
		header.version_minor = headerObj["version_minor"].toInt();
		header.win_condition = get_enum_value<win_condition_e>(headerObj["win_condition"].toString());
		header.loss_condition = get_enum_value<loss_condition_e>(headerObj["loss_condition"].toString());
	}
	
	auto err = validate_map_file_header(header);
	if(err != SUCCESS)
		return err;
	
	return SUCCESS;
}

map_error_e read_map_file_header(QDataStream& stream, map_file_header_t& header) {
	stream >> header.map_uuid;
	stream >> header.version_major >> header.version_minor
		   >> header.width >> header.height;

	memset(header.name, 0, sizeof(header.name));
	memset(header.description, 0, sizeof(header.description));
	
	if(stream.readRawData(header.name, 32) != 32)
		return MAP_COULD_NOT_READ_HEADER;
	if(stream.readRawData(header.description, 256) != 256)
		return MAP_COULD_NOT_READ_HEADER;

	stream >> header.players >> header.difficulty >> header.win_condition >> header.loss_condition;
	
	return SUCCESS;
}

map_error_e write_map_file_header(QDataStream& stream, const map_file_header_t& header) {
	stream << header.map_uuid;
	stream << header.version_major << header.version_minor
		   << header.width << header.height;

	stream.writeRawData(header.name, 32);
	stream.writeRawData(header.description, 256);

	stream << header.players << header.difficulty << header.win_condition << header.loss_condition;
	
	return SUCCESS;
}

map_error_e write_map_file_header_c(const std::string& filename, const map_file_header_t& header) {
	if(validate_map_file_header(header) != 0)
		return MAP_COULD_NOT_OPEN_FILE;

	FILE* fp = fopen(filename.c_str(), "wb");
	if(!fp)
		return MAP_COULD_NOT_OPEN_FILE;

	if(fwrite(&header, MAP_FILE_HEADER_SIZE, 1, fp) != 1) {
		fclose(fp);
		return MAP_COULD_NOT_WRITE_HEADER;
	}

	fclose(fp);

	return SUCCESS;
}

map_error_e read_map_file(const std::string& filename, adventure_map_t& map) {
	map_file_header_t header;
	return read_map_file(filename, map, header);
}

map_error_e read_map_file(const std::string& filename, adventure_map_t& map, map_file_header_t& header) {
	QFile file(QString::fromStdString(filename));
	if(!file.open(QIODevice::ReadOnly))
		return MAP_COULD_NOT_OPEN_FILE;
	
	//clear any existing data in the map
	map.clear();
	
	QDataStream stream(&file);
	
	return read_map_file_stream(stream, map, header);
}

map_error_e read_map_file_stream(QDataStream& stream, adventure_map_t& map, map_file_header_t& header) {
	//clear any existing data in the map
	map.clear();
	
	auto err = read_map_file_header(stream, header);
	if(err != SUCCESS)
		return MAP_COULD_NOT_READ_HEADER;
	
	
	err = validate_map_file_header(header);
	if(err != SUCCESS)
		return err;
	
	
	map.width = header.width;
	map.height = header.height;
	map.players = header.players;
	map.difficulty = header.difficulty;
	map.win_condition = header.win_condition;
	map.loss_condition = header.loss_condition;
	map.map_uuid = header.map_uuid;
	map.name = header.name;
	map.description = header.description;
	//map.minimap = header.minimap;
	
	//read player specifications
	for(auto& pc : map.player_configurations) {
		stream >> pc.player_number;
		stream >> pc.color;
		stream >> pc.team;
		stream_read_vector(stream, pc.allowed_classes);
		stream >> pc.allowed_player_type;
	}
	
	
	map.tiles.clear();
	map.tiles.resize(map.width * map.height);
	
	//read terrain
	for(int i = 0; i < map.width * map.height; i++) {
		stream >> map.tiles[i];
		
		
		auto e = stream.status();
		if(e != QDataStream::Ok)
			return MAP_TILE_DATA_INVALID;
		
		if(map.tiles[i].passability > 2)
			return MAP_TILE_DATA_INVALID;
		//if(map.tiles[i].terrain_type)
	}
	
	//read doodads
	uint32_t doodad_count = 0;
	stream >> doodad_count;
	if(doodad_count > MAXIMUM_DOODAD_COUNT)
		return MAP_TOO_MANY_DOODADS;

	map.doodads.resize(doodad_count);
	for(uint32_t i = 0; i < doodad_count; i++) {
		stream >> map.doodads[i];
		
		if(map.doodads[i].x < -1024 || map.doodads[i].y < -1024)
			return MAP_DOODAD_DATA_INVALID;
		
		if(map.doodads[i].width > 10 || map.doodads[i].height > 10)
			return MAP_DOODAD_DATA_INVALID;
	}
	
	//read interactable objects
	uint32_t object_count;
	stream >> object_count;
	if(object_count > MAXIMUM_OBJECTS_COUNT)
		return MAP_TOO_MANY_OBJECTS;

	map.objects.resize(object_count);
	for(uint32_t i = 0; i < object_count; i++) {
		interactable_object_e type;
		stream >> type;
		if(type == OBJECT_UNKNOWN) {
			map.objects[i] = nullptr;
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
			map.objects[i] = nullptr;
			continue;
		}

		//read the base class members from the buffer
		object_stream >> obj->asset_id >> obj->x >> obj->y >> obj->z >> obj->width >> obj->height;

		//read the derived class data
		obj->read_data(object_stream);

		map.objects[i] = obj;
	}
	
	//read heroes
	uint16_t hero_count = 0;
	stream >> hero_count;
	if(hero_count > MAXIMUM_HERO_COUNT)
		return MAP_TOO_MANY_HEROES;
	
	//read magic
	uint16_t magic1;
	stream >> magic1;
	
	for(int i = 0; i < hero_count; i++) {
		uint16_t hero_size;
		stream >> hero_size;

		//read hero data into a temporary buffer
		QByteArray buffer_data;
		buffer_data.resize(hero_size);
		stream.readRawData(buffer_data.data(), hero_size);

		QBuffer buffer(&buffer_data);
		buffer.open(QIODevice::ReadOnly);
		QDataStream hero_stream(&buffer);

		hero_t hero;
		hero_stream >> hero;
		if(map.heroes.count(hero.id) != 0)
			return MAP_DUPLICATE_HERO_ID;
		
		map.heroes[hero.id] = hero;
	}	
	
	//read magic
	uint16_t magic;
	stream >> magic;
	
	return SUCCESS;
}

map_error_e write_map_file(const std::string& filename, const adventure_map_t& map) {
	QFile file(QString::fromStdString(filename));
	if(!file.open(QIODevice::WriteOnly))
		return MAP_COULD_NOT_OPEN_FILE;
	

	QDataStream stream(&file);
	return write_map_file_stream(stream, map);
}

map_error_e write_map_file_stream(QDataStream& stream, const adventure_map_t& map) {
	map_file_header_t header;
	header.map_uuid = map.map_uuid;
	header.version_major = 1;
	header.version_minor = 0;
	header.width = map.width;
	header.height = map.height;
	std::string name = map.name;
	if(name.length() >= sizeof(header.name) / sizeof(char) - 1)
		name = name.substr(0, 31);

	strcpy(header.name, name.c_str());

	std::string description = map.description;
	if(description.length() >= sizeof(header.description) / sizeof(char) - 1)
		description = description.substr(0, 254);

	strcpy(header.description, description.c_str());

	header.players = map.players;
	header.difficulty = map.difficulty;
	header.win_condition = map.win_condition;
	header.loss_condition = map.loss_condition;
	
	auto err = write_map_file_header(stream, header);
	if(err != SUCCESS)
		return MAP_COULD_NOT_WRITE_HEADER;
	
	
	err = validate_map_file_header(header);
	if(err != SUCCESS)
		return err;
	
	//write player specifications
	for(auto& pc : map.player_configurations) {
		stream << pc.player_number;
		stream << pc.color;
		stream << pc.team;
		stream_write_vector(stream, pc.allowed_classes);
		stream << pc.allowed_player_type;
	}

	//todo: move to 'validate_map()' function
	if(map.tiles.size() != map.width * map.height)
		return MAP_TILE_DATA_INVALID;
	
	//write terrain
	for(auto& tile : map.tiles) {
		stream << tile;
		
		auto e = stream.status();
		if(e != QDataStream::Ok)
			return MAP_TILE_DATA_INVALID;
	}
	
	//write doodads
	stream << (uint32_t)map.doodads.size();
	for(auto& doodad : map.doodads)
		stream << doodad;
	
	//write interactable objects
	
	//when we write an existing game, some map objects can be null (removed, e.g. resources)
	stream << (uint32_t)map.objects.size();
	for(auto obj : map.objects) {
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
	
	//write heroes
	stream << (uint16_t)map.heroes.size();
	
	//write magic
	stream << (uint16_t)0x4321;
	
	for(const auto& hero_pair : map.heroes) {
		const auto& hero = hero_pair.second;

		//create temp buffer for hero data
		QBuffer buffer;
		buffer.open(QIODevice::WriteOnly);
		QDataStream hero_stream(&buffer);

		hero_stream << hero;

		//write size and buffer contents to main stream
		auto hero_size = (uint16_t)buffer.size();
		stream << hero_size;
		stream.writeRawData(buffer.data().constData(), buffer.size());
	}
	
	//write magic
	stream << (uint16_t)0x1234;
	return SUCCESS;
}

QJsonObject hero_to_json(const hero_t& hero) {
	QJsonObject heroJson;
	heroJson["id"] = hero.id;
	heroJson["player"] = name_from_enum(hero.player);
	heroJson["x"] = hero.x;
	heroJson["y"] = hero.y;
	heroJson["name"] = QString::fromStdString(hero.name).toStdString().c_str();
	heroJson["title"] = QString::fromStdString(hero.title).toStdString().c_str();
	heroJson["biography"] = QString::fromStdString(hero.biography).toStdString().c_str();
	heroJson["hero_class"] = name_from_enum(hero.hero_class);
	heroJson["portrait"] = hero.portrait;
	heroJson["specialty"] = name_from_enum(hero.specialty);
	heroJson["uncontrolled_by_human"] = hero.uncontrolled_by_human;
	heroJson["garrisoned"] = hero.garrisoned;
	heroJson["level"] = hero.level;
	heroJson["attack"] = hero.attack;
	heroJson["defense"] = hero.defense;
	heroJson["power"] = hero.power;
	heroJson["knowledge"] = hero.knowledge;
	heroJson["experience"] = (long long)hero.experience;
	heroJson["mana"] = hero.mana;
	heroJson["movement_points"] = hero.movement_points;
	heroJson["outlaw_bonus"] = hero.outlaw_bonus;
	
	QJsonArray skillsArray;
	for(const auto& s : hero.skills) {
		QJsonObject skillObj;
		skillObj["skill"] = name_from_enum(s.skill);
		skillObj["level"] = s.level;
		skillsArray.append(skillObj);
	}
	heroJson["skills"] = skillsArray;
	
	QJsonArray artifactsArray;
	for(const auto& a : hero.artifacts)
		artifactsArray.append(name_from_enum(a));
	
	heroJson["artifacts"] = artifactsArray;
	
	QJsonArray backpackArray;
	for(const auto& a : hero.backpack)
		backpackArray.append(name_from_enum(a));
	
	heroJson["backpack"] = backpackArray;
	
	QJsonArray troopsArray;
	for (const auto& t : hero.troops) {
		QJsonObject troopJson;
		troopJson["unit_type"] = name_from_enum(t.unit_type);
		troopJson["stack_size"] = t.stack_size;
		troopsArray.append(troopJson);
	}
	heroJson["troops"] = troopsArray;
	
	QJsonArray talentsArray;
	for(const auto& t : hero.talents)
		talentsArray.append(name_from_enum(t));
	
	heroJson["talents"] = talentsArray;
	
	QJsonArray availableTalentsArray;
	for(const auto& t : hero.available_talents)
		availableTalentsArray.append(name_from_enum(t));
	
	heroJson["available_talents"] = availableTalentsArray;
	
	QJsonArray spellbookArray;
	for(const auto& spell : hero.spellbook)
		spellbookArray.append(name_from_enum(spell));
	
	heroJson["spellbook"] = spellbookArray;
	
	QJsonArray scriptsArray;
	for (const auto& script : hero.scripts) {
		QJsonObject scriptJson;
		scriptJson["script_id"] = script.script_id;
		scriptJson["enabled"] = script.enabled;
		scriptJson["finished"] = script.finished;
		scriptJson["name"] = QString::fromStdString(script.name).toStdString().c_str();
		scriptJson["text"] = QString::fromStdString(script.text).toStdString().c_str();
		scriptJson["parent_type"] = name_from_enum(script.parent_type);
		scriptJson["run_condition"] = name_from_enum(script.run_condition);
		scriptsArray.append(scriptJson);
	}
	heroJson["scripts"] = scriptsArray;
	
	QJsonObject appearanceJson;
	appearanceJson["class_appearance"] = (int)hero.appereance.class_appearance;
	appearanceJson["body"] = (int)hero.appereance.body;
	appearanceJson["face"] = (int)hero.appereance.face;
	appearanceJson["eye_color"] = (int)hero.appereance.eye_color;
	appearanceJson["beard"] = (int)hero.appereance.beard;
	appearanceJson["eyebrows"] = (int)hero.appereance.eyebrows;
	appearanceJson["ears"] = (int)hero.appereance.ears;
	appearanceJson["skin"] = (int)hero.appereance.skin;
	appearanceJson["hair"] = (int)hero.appereance.hair;
	appearanceJson["hair_color"] = (int)hero.appereance.hair_color;
	appearanceJson["mount_type"] = (int)hero.appereance.mount_type;
	appearanceJson["pet_type"] = (int)hero.appereance.pet_type;

	heroJson["appearance"] = appearanceJson;

	//	QJsonArray visitedObjectsArray;
	//	for (const auto& object : hero.visited_objects) {
	//		visitedObjectsArray.append(object);
	//	}
	//	heroJson["visited_objects"] = visitedObjectsArray;
	
	return heroJson;
}

hero_t json_to_hero(const QJsonObject& heroJson) {
	hero_t hero;
	hero.id = heroJson["id"].toInt();
	hero.player = get_enum_value<player_e>(heroJson["player"].toString());
	hero.x = heroJson["x"].toInt();
	hero.y = heroJson["y"].toInt();
	hero.name = heroJson["name"].toString().toStdString();
	hero.title = heroJson["title"].toString().toStdString();
	hero.biography = heroJson["biography"].toString().toStdString();
	hero.hero_class = get_enum_value<hero_class_e>(heroJson["hero_class"].toString());
	hero.portrait = heroJson["portrait"].toInt();
	hero.specialty = get_enum_value<hero_specialty_e>(heroJson["specialty1"].toString());
	hero.uncontrolled_by_human = heroJson["uncontrolled_by_human"].toBool();
	hero.garrisoned = heroJson["garrisoned"].toBool();
	hero.level = heroJson["level"].toInt();
	hero.attack = heroJson["attack"].toInt();
	hero.defense = heroJson["defense"].toInt();
	hero.power = heroJson["power"].toInt();
	hero.knowledge = heroJson["knowledge"].toInt();
	hero.experience = heroJson["experience"].toVariant().toLongLong();  // Ensure large integers are handled correctly
	hero.mana = heroJson["mana"].toInt();
	hero.movement_points = heroJson["movement_points"].toInt();
	hero.outlaw_bonus = heroJson["outlaw_bonus"].toInt();
	
	int i = 0;
	QJsonArray skillsArray = heroJson["skills"].toArray();
	for (const auto& skillValue : skillsArray) {
		QJsonObject skillObj = skillValue.toObject();
		secondary_skill_t skill;
		skill.skill = get_enum_value<skill_e>(skillObj["skill"].toString());
		skill.level = skillObj["level"].toInt();
		hero.skills[i] = skill;
		i++;
	}
	
	i = 0;
	QJsonArray artifactsArray = heroJson["artifacts"].toArray();
	for(const auto& artifactValue : artifactsArray) {
		hero.artifacts[i] = get_enum_value<artifact_e>(artifactValue.toString());
		i++;
	}
	
	i = 0;
	QJsonArray backpackArray = heroJson["backpack"].toArray();
	for (const auto& artifactValue : backpackArray) {
		hero.backpack[i] = get_enum_value<artifact_e>(artifactValue.toString());
		i++;
	}
	
	i = 0;
	QJsonArray troopsArray = heroJson["troops"].toArray();
	for (const auto& troopValue : troopsArray) {
		QJsonObject troopObj = troopValue.toObject();
		troop_t troop;
		troop.unit_type = get_enum_value<unit_type_e>(troopObj["unit_type"].toString());
		troop.stack_size = troopObj["stack_size"].toInt();
		hero.troops[i] = troop;
		i++;
	}
	
	i = 0;
	QJsonArray talentsArray = heroJson["talents"].toArray();
	for (const auto& talentValue : talentsArray) {
		hero.talents[i] = get_enum_value<talent_e>(talentValue.toString());
		i++;
	}
	
	i = 0;
	QJsonArray availableTalentsArray = heroJson["available_talents"].toArray();
	for(const auto& talentValue : availableTalentsArray) {
		hero.available_talents[i] = get_enum_value<talent_e>(talentValue.toString());
		i++;
	}
	
	QJsonArray spellbookArray = heroJson["spellbook"].toArray();
	for(const auto& spellValue : spellbookArray) {
		hero.spellbook.push_back(get_enum_value<spell_e>(spellValue.toString()));
	}
	
	QJsonArray scriptsArray = heroJson["scripts"].toArray();
	for(const auto& scriptValue : scriptsArray) {
		if (scriptValue.isObject()) {
			QJsonObject scriptJson = scriptValue.toObject();
			script_t script;
			script.script_id = scriptJson["script_id"].toInt();
			script.enabled = scriptJson["enabled"].toBool();
			script.finished = scriptJson["finished"].toBool();
			script.name = scriptJson["name"].toString().toStdString();
			script.text = scriptJson["text"].toString().toStdString();
			script.parent_type = get_enum_value<script_t::parent_type_e>(scriptJson["parent_type"].toString());
			script.run_condition = get_enum_value<script_t::run_condition_e>(scriptJson["run_condition"].toString());
			hero.scripts.push_back(script);
		}
	}
	
	if(heroJson.contains("appearance")) {
		QJsonObject appearanceJson = heroJson["appearance"].toObject();
		hero.appereance.class_appearance = (hero_class_appearance_e)appearanceJson["class_appearance"].toInt();
		hero.appereance.body = (hero_body_appearance_e)appearanceJson["body"].toInt();
		hero.appereance.face = (hero_face_appearance_e)appearanceJson["face"].toInt();
		hero.appereance.eye_color = (hero_eyes_appearance_e)appearanceJson["eye_color"].toInt();
		hero.appereance.beard = (hero_beard_appearance_e)appearanceJson["beard"].toInt();
		hero.appereance.eyebrows = (hero_eyebrow_appearance_e)appearanceJson["eyebrows"].toInt();
		hero.appereance.ears = (hero_ears_appearance_e)appearanceJson["ears"].toInt();
		hero.appereance.skin = (hero_skin_appearance_e)appearanceJson["skin"].toInt();
		hero.appereance.hair = (hero_hair_appearance_e)appearanceJson["hair"].toInt();
		hero.appereance.hair_color = (hero_hair_color_appearance_e)appearanceJson["hair_color"].toInt();
		hero.appereance.mount_type = (hero_mount_type_e)appearanceJson["mount_type"].toInt();
		hero.appereance.pet_type = (hero_pet_type_e)appearanceJson["pet_type"].toInt();
	}

	// QJsonArray visitedObjectsArray = heroJson["visited_objects"].toArray();
	// for (const auto& objectValue : visitedObjectsArray) {
	//     hero.visited_objects.push_back(objectValue.toInt());
	// }
	
	return hero;
}

map_error_e read_map_file_json(const std::string& filepath, adventure_map_t& map) {
	//clear any existing data in the map
	map.clear();
	
	QFile file(QString::fromStdString(filepath));
	if(!file.open(QIODevice::ReadOnly))
		return MAP_COULD_NOT_OPEN_FILE;
	
	auto bytes = file.readAll();
	auto json_document = QJsonDocument::fromJson(bytes);
	
	if(!json_document.isObject()) {
		return MAP_JSON_FORMAT_ERROR;
	}
	
	QJsonObject json = json_document.object();
	
	if (json.contains("header") && json["header"].isObject()) {
		QJsonObject headerObj = json["header"].toObject();
		map.map_uuid = QUuid::fromString(headerObj["map_uuid"].toString());
		map.width = headerObj["width"].toInt();
		map.height = headerObj["height"].toInt();
		map.name = headerObj["name"].toString().toStdString();
		map.description = headerObj["description"].toString().toStdString();
		map.players = headerObj["players"].toInt();
		map.difficulty = headerObj["difficulty"].toInt();
		map.win_condition = get_enum_value<win_condition_e>(headerObj["win_condition"].toString());
		map.loss_condition = get_enum_value<loss_condition_e>(headerObj["loss_condition"].toString());
	}
	
	if (json.contains("player_configurations") && json["player_configurations"].isArray()) {
		QJsonArray playerConfigsArray = json["player_configurations"].toArray();
		size_t i = 0;
		for (const auto& pcValue : playerConfigsArray) {
			if (i < map.player_configurations.size() && pcValue.isObject()) {
				QJsonObject pcObj = pcValue.toObject();
				player_configuration_t pc;
				pc.player_number = get_enum_value<player_e>(pcObj["player_number"].toString());
				pc.color = get_enum_value<player_color_e>(pcObj["color"].toString());
				pc.team = pcObj["team"].toInt();
				pc.is_human = pcObj["is_human"].toBool();
				pc.player_name = pcObj["player_name"].toString().toStdString();
				pc.selected_class = pcObj["selected_class"].toInt();
				
				pc.allowed_classes.clear();
				QJsonArray allowedClassesArray = pcObj["allowed_classes"].toArray();
				for (const auto& clsValue : allowedClassesArray) {
					pc.allowed_classes.push_back(get_enum_value<hero_class_e>(clsValue.toString()));
				}
				
				pc.allowed_player_type = get_enum_value<player_type_e>(pcObj["allowed_player_type"].toString());
				pc.starting_hero_id = pcObj["starting_hero_id"].toInt();
				pc.was_class_random = pcObj["was_class_random"].toBool();
				pc.was_hero_random = pcObj["was_hero_random"].toBool();
				
				map.player_configurations[i] = pc;
				i++;
			}
		}
	}
	
	if(json.contains("tiles") && json["tiles"].isArray()) {
		QJsonArray tilesArray = json["tiles"].toArray();
		for(const auto& tileValue : tilesArray) {
			if(tileValue.isObject()) {
				QJsonObject tileObj = tileValue.toObject();
				map_tile_t tile;
				tile.asset_id = tileObj["asset_id"].toInt();
				tile.terrain_type = get_enum_value<terrain_type_e>(tileObj["terrain_type"].toString());
				tile.road_type = get_enum_value<road_type_e>(tileObj["road_type"].toString());
				tile.passability = tileObj["passability"].toInt();
				tile.zone_id = tileObj["zone_id"].toInt();
				tile.interactable_object = tileObj["interactable_object"].toInt();
				map.tiles.push_back(tile);
			}
		}
	}
	
	if(json.contains("doodads") && json["doodads"].isArray()) {
		QJsonArray doodadsArray = json["doodads"].toArray();
		for(const auto& doodadValue : doodadsArray) {
			if(doodadValue.isObject()) {
				QJsonObject doodadObj = doodadValue.toObject();
				doodad_t doodad;
				doodad.asset_id = doodadObj["asset_id"].toInt();
				doodad.z = doodadObj["z"].toInt();
				doodad.x = doodadObj["x"].toInt();
				doodad.y = doodadObj["y"].toInt();
				doodad.width = doodadObj["width"].toInt();
				doodad.height = doodadObj["height"].toInt();
				map.doodads.push_back(doodad);
			}
		}
	}
	
	if(json.contains("objects") && json["objects"].isArray()) {
		QJsonArray objectsArray = json["objects"].toArray();
		for(const auto& objValue : objectsArray) {
			if(!objValue.isObject())
				continue;
			
			QJsonObject objJson = objValue.toObject();
			auto type = get_enum_value<interactable_object_e>(objJson["object_type"].toString());
			if(type == OBJECT_UNKNOWN) {
				map.objects.push_back(nullptr);
				continue;
			}
			
			auto obj = interactable_object_t::make_new_object(type);
			obj->object_type = type;
			obj->asset_id = objJson["asset_id"].toInt();
			obj->x = objJson["x"].toInt();
			obj->y = objJson["y"].toInt();
			obj->z = objJson["z"].toInt();
			obj->width = objJson["width"].toInt();
			obj->height = objJson["height"].toInt();
			
			obj->read_data_json(objJson["data"].toObject());
			
			map.objects.push_back(obj);
		}
	}
	
	if (json.contains("heroes") && json["heroes"].isArray()) {
		QJsonArray heroesArray = json["heroes"].toArray();
		for (const auto& heroValue : heroesArray) {
			if (heroValue.isObject()) {
				QJsonObject heroJson = heroValue.toObject();
				int heroId = heroJson["id"].toInt();
				hero_t hero = json_to_hero(heroJson);
				map.heroes[heroId] = hero;
			}
		}
	}
	
	return SUCCESS;
}

map_error_e write_map_file_json(const std::string& path, const adventure_map_t& map) {
	QJsonObject json;
	
	QJsonObject headerObj;
	headerObj["map_uuid"] = map.map_uuid.toString();
	headerObj["version_major"] = 1;
	headerObj["version_minor"] = 0;
	headerObj["width"] = map.width;
	headerObj["height"] = map.height;
	headerObj["name"] = QString::fromStdString(map.name.substr(0, 31));
	headerObj["description"] = QString::fromStdString(map.description.substr(0, 254));
	headerObj["players"] = map.players;
	headerObj["difficulty"] = map.difficulty;
	headerObj["win_condition"] = name_from_enum(map.win_condition);
	headerObj["loss_condition"] = name_from_enum(map.loss_condition);
	json["header"] = headerObj;
	
	QJsonArray playerConfigs;
	for (const auto& pc : map.player_configurations) {
		QJsonObject pcObj;
		pcObj["player_number"] = name_from_enum(pc.player_number);
		pcObj["color"] = name_from_enum(pc.color);
		pcObj["team"] = static_cast<int>(pc.team);
		pcObj["is_human"] = pc.is_human;
		pcObj["player_name"] = QString::fromStdString(pc.player_name).toStdString().c_str();
		pcObj["selected_class"] = pc.selected_class;
		
		QJsonArray allowedClasses;
		for (auto& cls : pc.allowed_classes) {
			allowedClasses.append(name_from_enum(cls));
		}
		pcObj["allowed_classes"] = allowedClasses;
		
		pcObj["allowed_player_type"] = name_from_enum(pc.allowed_player_type);
		pcObj["starting_hero_id"] = pc.starting_hero_id;
		pcObj["was_class_random"] = pc.was_class_random;
		pcObj["was_hero_random"] = pc.was_hero_random;
		
		playerConfigs.append(pcObj);
	}
	json["player_configurations"] = playerConfigs;
	
	QJsonArray tilesArray;
	for(const auto& tile : map.tiles) {
		QJsonObject tileJson;
		tileJson["asset_id"] = tile.asset_id;
		tileJson["terrain_type"] = name_from_enum(tile.terrain_type);
		tileJson["road_type"] = name_from_enum(tile.road_type);
		tileJson["passability"] = tile.passability;
		tileJson["zone_id"] = tile.zone_id;
		tileJson["interactable_object"] = tile.interactable_object;
		tilesArray.append(tileJson);
	}
	json["tiles"] = tilesArray;
	
	QJsonArray doodadsArray;
	for(const auto& doodad : map.doodads) {
		QJsonObject doodadJson;
		doodadJson["asset_id"] = doodad.asset_id;
		doodadJson["z"] = doodad.z;
		doodadJson["x"] = doodad.x;
		doodadJson["y"] = doodad.y;
		doodadJson["width"] = doodad.width;
		doodadJson["height"] = doodad.height;
		doodadsArray.append(doodadJson);
	}
	json["doodads"] = doodadsArray;
	
	QJsonArray objectsArray;
	for(const auto& obj : map.objects) {
		if(!obj) {
			QJsonObject objJson;
			objJson["object_type"] = "OBJECT_UNKNOWN";
			objectsArray.append(objJson);
			continue;
		}
		
		QJsonObject objJson;
		objJson["object_type"] = name_from_enum(obj->object_type);
		objJson["asset_id"] = obj->asset_id;
		objJson["x"] = obj->x;
		objJson["y"] = obj->y;
		objJson["z"] = obj->z;
		objJson["width"] = obj->width;
		objJson["height"] = obj->height;
		objJson["data"] = obj->write_data_json();
		
		objectsArray.append(objJson);
	}
	json["objects"] = objectsArray;
	
	QJsonArray heroesArray;
	for (const auto& heroPair : map.heroes) {
		QJsonObject heroJson = hero_to_json(heroPair.second);
		heroJson["id"] = heroPair.first;  // Add if hero id is stored in the key
		heroesArray.append(heroJson);
	}
	
	json["heroes"] = heroesArray;
	
	
	
	QJsonDocument doc(json);
	
	
	QFile file(QString::fromStdString(path));
	if(!file.open(QIODevice::WriteOnly))
		return MAP_COULD_NOT_OPEN_FILE;
	
	
	QTextStream stream(&file);
	stream << doc.toJson(QJsonDocument::Indented);
	
	return SUCCESS;
	
}
