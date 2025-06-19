TEMPLATE = app
TARGET = cof_core

INCLUDEPATH += .
INCLUDEPATH += ./server/src
INCLUDEPATH += ./game/src

CONFIG += qt debug console
CONFIG -= app_bundle
QT += core network gui
CONFIG += c++17
#CONFIG += no_autoqmake

MOC_DIR = moc

#LIBS += -L/opt/homebrew/opt/lua/lib
#LIBS += -llua

#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
HEADERS += lua/lauxlib.h \
           lua/lua.h \
           lua/lua.hpp \
           lua/luaconf.h \
           lua/lualib.h \
           game/src/core/adventure_map.h \
           game/src/core/artifact.h \
           game/src/core/battlefield.h \
           game/src/core/battlefield_hex_grid.h \
           game/src/core/creature.h \
           game/src/core/game.h \
           game/src/core/game_config.h \
           game/src/core/hero.h \
           game/src/core/interactable_object.h \
           game/src/core/lua_api.h \
           game/src/core/map_file.h \
           game/src/core/script.h \
           game/src/core/network_actions.h \
           game/src/core/spell.h \
           game/src/core/town.h \
           game/src/core/troop.h \
           game/src/core/utils.h

SOURCES += 	main.cpp \
            game/src/core/ai_adventure_map.cpp \
            game/src/core/ai_combat.cpp \
            game/src/core/adventure_map.cpp \
            game/src/core/hero.cpp \
            game/src/core/artifact.cpp \
            game/src/core/battlefield.cpp \
            game/src/core/lua_api.cpp \
            game/src/core/game.cpp \
            game/src/core/game_config.cpp \
            game/src/core/interactable_object.cpp \
            game/src/core/map_file.cpp \
            game/src/core/town.cpp

