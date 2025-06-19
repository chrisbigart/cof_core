#include "core/lua_api.h"
#include "core/resource.h"

//probably need to change this

//QList<lua_function_info_t> lua_functions;
static game_t* _game = nullptr;
QList<lua_function_info_t> lua_functions = QList<lua_function_info_t>();

const QList<lua_function_info_t>& get_lua_functions() {
	return lua_functions;
}

int register_lua_functions(lua_State* L, game_t* game_ptr) {

	lua_functions.push_back({lua::GetCurrentDay, "GetCurrentDay", 0, "Gets the current day as an integer. The first day is 1, and it increments by 1 per day (does not wrap around on a new week.)."});
	lua_functions.push_back({lua::GetCurrentWeek, "GetCurrentWeek", 0, "Gets the current week as an integer. The first week is 1, and it increments by 1 per week."});
	lua_functions.push_back({lua::GetCurrentMonth, "GetCurrentMonth", 0, "Gets the current month as an integer. The first month is 1, and it increments by 1 per month."});
	lua_functions.push_back({lua::GetPlayerCount, "GetPlayerCount", 0, "Gets the total number of players in the game as an integer."});
	lua_functions.push_back({lua::GetCurrentPlayer, "GetCurrentPlayer", 0, "Gets the current player's index as an integer."});
	lua_functions.push_back({lua::GivePlayerResources, "GivePlayerResources", 3, "Gives a player resources. Takes three arguments: player index, resource type, and the amount of the resource."});
	lua_functions.push_back({lua::SendPlayerConsoleMessage, "SendPlayerConsoleMessage", 2, "Sends a console message to a player. Takes two arguments: player index and the message string."});
	lua_functions.push_back({lua::ShowPlayerMessage, "ShowPlayerMessage", 4, "Shows a message to a player as a dialog. Takes two arguments: player index, message string."});
	lua_functions.push_back({lua::ScriptCompleted, "ScriptCompleted", 0, "Signals when a script's condition has been triggered and it no longer needs to be run. Returns a true or false."});
	lua_functions.push_back({lua::SetHeroName, "SetHeroName", 2, "Set the name of the hero. Takes two arguments: the hero object and the new name string."});
	lua_functions.push_back({lua::SetHeroClass, "SetHeroClass", 2, "Set the class of the hero. Takes two arguments: the hero object and the new class."});
	lua_functions.push_back({lua::SetHeroLevel, "SetHeroLevel", 2, "Set the level of the hero. Takes two arguments: the hero object and the new level as an integer."});
	lua_functions.push_back({lua::AddHeroExperience, "AddHeroExperience", 2, "Add experience points to the hero. Takes two arguments: the hero object and the amount of experience points to add as an integer."});
	lua_functions.push_back({lua::LearnSkill, "LearnSkill", 3, "Make the hero learn a specific skill at a certain level. Takes three arguments: the hero object, the skill, and the skill level as an integer."});
	lua_functions.push_back({lua::UnlearnSkill, "UnlearnSkill", 2, "Make the hero unlearn a specific skill. Takes two arguments: the hero object and the skill."});
	lua_functions.push_back({lua::UpgradeSkill, "UpgradeSkill", 3, "Upgrade a specific skill of the hero to a certain level. Takes three arguments: the hero object, the skill, and the new skill level as an integer."});
	lua_functions.push_back({lua::GiveHeroArtifact, "GiveHeroArtifact", 2, "Give a specific artifact to the hero. Takes two arguments: the hero object and the artifact."});

	
	
	for(auto& finfo : lua_functions) {
		lua_register(L, finfo.name.toUtf8(), finfo.fptr);
	}
	
	
	// Create a new table
	lua_newtable(L);

	magic_enum::enum_for_each<resource_e>([L](auto val) {
		constexpr resource_e sp = val;
		auto sview = magic_enum::enum_name(sp);
		sview.remove_prefix(9); //9 == count("RESOURCE_")
		lua_pushstring(L, sview.data());
		lua_pushinteger(L, val);
		lua_settable(L, -3);
		});
	
	lua_setglobal(L, "Resource");
	
	_game = game_ptr;
	
	return 0;
}

void update_lua_hero_table(lua_State* L, hero_t* hero_ptr) {
	if(!hero_ptr) {
		//todo: clear current hero table?
		return;
	}
	
	// Create a new table
	lua_newtable(L);

	lua_pushstring(L, "Player");
	lua_pushinteger(L, hero_ptr->player);
	lua_settable(L, -3);
	lua_pushstring(L, "Name");
	lua_pushstring(L, hero_ptr->name.c_str());
	lua_settable(L, -3);
	lua_pushstring(L, "Level");
	lua_pushinteger(L, hero_ptr->level);
	lua_settable(L, -3);
	
	
	lua_setglobal(L, "Hero");
}

namespace lua {

	int GetCurrentDay(lua_State* L) {
		if(!_game) return 0;
		
		lua_pushinteger(L, _game->get_day());
		return 1;
	}
	int GetCurrentWeek(lua_State* L) {
		if(!_game) return 0;
		
		lua_pushinteger(L, _game->get_week());
		return 1;
	}
	int GetCurrentMonth(lua_State* L) {
		if(!_game) return 0;
		
		lua_pushinteger(L, _game->get_month());
		return 1;
	}

	int GetPlayerCount(lua_State* L) {
		if(!_game) return 0;
		
		lua_pushinteger(L, _game->map.players);
		return 1;
	}
	int GetCurrentPlayer(lua_State* L) {
		if(!_game) return 0;
		
		int current_player = 1;
		lua_pushinteger(L, current_player);
		return 1;
	}

	int GivePlayerResources(lua_State* L) {
		if(!_game) return 0;
		
		auto player = (player_e)lua_tonumber(L,1);
		auto resource_type = (resource_e)lua_tonumber(L,2);
		auto amount = (int)lua_tonumber(L,3);
		
		if(!_game->player_valid(player))
			return 0;
		
		//validity checks
		
		resource_group_t res;
		res.set_value_for_type(resource_type, amount);
		_game->get_player(player).resources += res;
		
		return 1;
	}

	//hero-related functions
	int SetHeroName(lua_State* L) {
		// Implement the functionality to set the hero's name
		return 0;
	}

	int SetHeroClass(lua_State* L) {
		// Implement the functionality to set the hero's class
		return 0;
	}

	int SetHeroLevel(lua_State* L) {
		// Implement the functionality to set the hero's level
		return 0;
	}

	int AddHeroExperience(lua_State* L) {
		// Implement the functionality to add experience points to the hero
		return 0;
	}

	int LearnSkill(lua_State* L) {
		// Implement the functionality to make the hero learn a specific skill at a certain level
		return 0;
	}

	int UnlearnSkill(lua_State* L) {
		// Implement the functionality to make the hero unlearn a specific skill
		return 0;
	}

	int UpgradeSkill(lua_State* L) {
		// Implement the functionality to upgrade a specific skill of the hero to a certain level
		return 0;
	}

	int GiveHeroArtifact(lua_State* L) {
		// Implement the functionality to give a specific artifact to the hero
		return 0;
	}


	int SendPlayerConsoleMessage(lua_State* L) {
		if(!_game) return 0;
		
		auto player = lua_tonumber(L,1);
		auto message = lua_tostring(L, 2);
		
		std::cout << "console message from lua: " << message << std::endl;
		return 1;
	}

	int ShowPlayerMessage(lua_State* L) {
		if(!_game) return 0;
		
		auto player = lua_tonumber(L,1);
		auto message = lua_tostring(L, 2);
		auto resource_type = lua_tonumber(L,2);
		auto amount = lua_tonumber(L,3);
		return 1;
	}

	//used to signal when a script's condition has triggered and it no lnoger needs to be run
	int ScriptCompleted(lua_State* L) {
		if(!_game) return 0;
		
		lua_pushboolean(L, true);
		return 1;
	}
}
