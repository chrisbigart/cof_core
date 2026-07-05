TEMPLATE = app
TARGET = combat_core_tests

INCLUDEPATH += ..
INCLUDEPATH += ../game/src

CONFIG += qt debug console c++20 link_pkgconfig
CONFIG -= app_bundle
QT += core network gui
PKGCONFIG += lua5.4

LIBS += -llua5.4

SOURCES += combat_core_tests.cpp \
           ../game/src/core/ai_adventure_map.cpp \
           ../game/src/core/ai_combat.cpp \
           ../game/src/core/adventure_map.cpp \
           ../game/src/core/hero.cpp \
           ../game/src/core/artifact.cpp \
           ../game/src/core/battlefield.cpp \
           ../game/src/core/lua_api.cpp \
           ../game/src/core/game.cpp \
           ../game/src/core/game_config.cpp \
           ../game/src/core/interactable_object.cpp \
           ../game/src/core/map_file.cpp \
           ../game/src/core/script.cpp \
           ../game/src/core/town.cpp
