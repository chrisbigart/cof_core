#pragma once

#include "core/qt_headers.h"
#include "core/magic_enum.hpp"

namespace utils {
	inline QString to_camel_case(const QString& s) {
		auto parts = s.split(' ', Qt::SkipEmptyParts);
		for (int i = 0; i < parts.size(); ++i)
			parts[i].replace(0, 1, parts[i][0].toUpper());

		return parts.join(" ");
	}

	template<typename T> QString formatted_name_from_enum(T e) {
		auto str = QString(magic_enum::enum_name(e).data());
		auto pos = str.indexOf('_');
		str = utils::to_camel_case(str.remove(0, pos+1).replace('_', ' ').toLower());
		return str;
	}

	template<typename T> QString name_from_enum(T e) {
		auto name = magic_enum::enum_name(e);
		if(name.empty())
			return "UNKNOWN";
		
		return QString(name.data());
	}

	template<typename T> T get_enum_value(const QString& str) {
		auto val = magic_enum::enum_cast<T>(str.trimmed().toStdString());
		if(!val.has_value())
			return (T)0;
		
		return val.value();
	}
	
	template<> hero_class_e utils::get_enum_value<hero_class_e>(const QString& str);
}