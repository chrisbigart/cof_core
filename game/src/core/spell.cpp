#include "core/spell.h"
#include "core/utils.h"

#include "core/qt_headers.h"

std::string spell_t::get_school_name(spell_school_e school) {
	   return QObject::tr(utils::formatted_name_from_enum(school).toUtf8()).toStdString();
   }
