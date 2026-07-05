#include "core/spell.h"
#include "core/utils.h"

#include "core/qt_headers.h"

std::string spell_t::get_school_name(spell_school_e school, bool include_magic_suffix) {
	switch(school) {
		case SCHOOL_NONE:
			return include_magic_suffix ? QObject::tr("None").toStdString() : QObject::tr("None").toStdString();
		case SCHOOL_ARCANE:
			return include_magic_suffix ? QObject::tr("Arcane Magic").toStdString() : QObject::tr("Arcane").toStdString();
		case SCHOOL_NATURE:
			return include_magic_suffix ? QObject::tr("Nature Magic").toStdString() : QObject::tr("Nature").toStdString();
		case SCHOOL_HOLY:
			return include_magic_suffix ? QObject::tr("Holy Magic").toStdString() : QObject::tr("Holy").toStdString();
		case SCHOOL_DESTRUCTION:
			return include_magic_suffix ? QObject::tr("Destruction Magic").toStdString() : QObject::tr("Destruction").toStdString();
		case SCHOOL_DEATH:
			return include_magic_suffix ? QObject::tr("Death Magic").toStdString() : QObject::tr("Death").toStdString();
		case SCHOOL_WARCRY:
			return include_magic_suffix ? QObject::tr("Warcry Magic").toStdString() : QObject::tr("Warcry").toStdString();
		case SCHOOL_ALL:
			return include_magic_suffix ? QObject::tr("All Schools").toStdString() : QObject::tr("All Schools").toStdString();
		default:
			return include_magic_suffix ? QObject::tr("Unknown School").toStdString() : QObject::tr("Unknown").toStdString();
	}
}
