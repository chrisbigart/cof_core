TEMPLATE = app
TARGET = basic_core_tests

INCLUDEPATH += ..
INCLUDEPATH += ../game/src

CONFIG += qt debug console c++20
CONFIG -= app_bundle
QT += core gui

SOURCES += basic_core_tests.cpp
