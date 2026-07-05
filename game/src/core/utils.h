#pragma once

#include <string>
#include <vector>
#include <random>

#include "core/qt_headers.h"
#include "core/magic_enum.hpp"


inline std::string stream_read_string(QDataStream& stream, uint16_t max_length = 255) {
	uint16_t length;
	stream >> length;
	
	std::string string;
	if(length > max_length) { //we got a problem
		//we could only read up to max_length characters, but then the next read will
		//be of garbage data.
		//do we just read max_length, skip length - max_length bytes in the stream?
		return "Error - String length too long";
	}
	
	string.resize(length);
	stream.readRawData(&string[0], length);
	
	return string;
}

inline int stream_write_string(QDataStream& stream, const std::string& string, uint16_t max_length = 255) {
	uint16_t length = (string.length() > (size_t)max_length) ? max_length : (uint16_t)string.length();
	
	stream << length;
	auto written = stream.writeRawData(&string[0], length);
	
	if(written != length)
		return 0;
	
	return sizeof(uint16_t) + length;
}

template<typename T> int stream_write_vector(QDataStream& stream, const T& data, uint32_t max_length = 65535) {
	uint32_t length = (data.size() > (size_t)max_length) ? max_length : (uint32_t)data.size();
	
	stream << length;
	
	for(auto& i : data)
		stream << i;
	
	return sizeof(uint32_t) + (length * sizeof(typename T::value_type));
}

template<typename T> int stream_read_vector(QDataStream& stream, T& data, uint32_t max_length = 65535) {
	uint32_t length;
	stream >> length;
	
	if(length > max_length)
		return 0;
	
	data.resize(length);
	
	for(auto& i : data)
		stream >> i;
	
	return sizeof(uint32_t) + (length * sizeof(typename T::value_type));
}

template<typename T> int stream_read_vector(QDataStream& stream, T& data, const typename T::value_type& min_value, const typename T::value_type& max_value, uint32_t max_length = 65535) {
	uint32_t length;
	stream >> length;
	
	if(length > max_length)
		return 0;
	
	data.resize(length);
	
	for(auto& i : data) {
		typename T::value_type val;
		stream >> val;
		i = std::clamp(val, min_value, max_value);
	}
	
	return sizeof(uint32_t) + (length * sizeof(typename T::value_type));
}

template<typename T> int stream_read_array(QDataStream& stream, T& data) {
	uint32_t length;
	stream >> length;
	
	assert(length == data.size());
	
	for(auto& i : data)
		stream >> i;
	
	return 0;
}


namespace utils {
	template <typename T> T max_abs_preserving(const T& a, const T& b) {
		return (std::abs(a) > std::abs(b)) ? a : b;
	}

	template <typename T> T max_abs(const T& a, const T& b) {
		return std::max(std::abs(a), std::abs(b));
	}

	inline int mh_dist(int x1, int y1, int x2, int y2) {
		return abs(x1 - x2) + abs(y1 - y2);
	}

	inline int eu_dist(int x1, int y1, int x2, int y2) {
		auto xdiff = x1 - x2;
		auto ydiff = y1 - y2;
		return xdiff*xdiff + ydiff*ydiff;
	}

	inline float dist(int x1, int y1, int x2, int y2) {
		return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
	}

	inline float distf(float x1, float y1, float x2, float y2) {
		return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
	}

	template<typename T> T rand_range(const T& min, const T& max) {
		//assert(max >= min)
		//fixme to use std::random
		T remainder = std::rand() % (max - min + 1);
		T val = min + remainder;
		return val;
	}

	template<typename T> T rand_rangef(const T& min, const T& max) {
		return ((T)rand() / RAND_MAX) * (max - min) + min;
	}

	template<typename TContainer> const auto& rand_item(const TContainer& container) {
		auto index = rand_range(0, (int)container.size() - 1);
		return container[index];
	}

	template<typename T> T rand_range(const T& min, const T& max, std::mt19937_64& rng) {
		std::uniform_int_distribution<T> dist(min, max);
		return dist(rng);
	}

	template<typename T> T rand_rangef(const T& min, const T& max, std::mt19937_64& rng) {
		std::uniform_real_distribution<T> dist(min, max);
		return dist(rng);
	}

	template<typename TContainer> const auto& rand_item(const TContainer& container, std::mt19937_64& rng) {
		auto index = rand_range(0, (int)container.size() - 1, rng);
		return container[index];
	}

	inline bool rand_chance(const int percent_chance, std::mt19937_64& rng) {
		std::uniform_int_distribution<int> dist(0, 99);
		return dist(rng) < percent_chance;
	}

	///returns true percent_chance percent of the time
	inline bool rand_chance(const int percent_chance) {
		return (std::rand() % 100) < percent_chance;
	}

	template<typename T> std::string to_str(const T& val) { //fixme to use std::format
		//std::format("{0}", val);
		std::stringstream strm;
		strm << val;
		return (strm.str());
	}
	
	template<typename T, typename Ty> bool contains(const Ty& container, const T& item) {
		return std::find(container.begin(), container.end(), item) != container.end();
	}

	template<typename T> bool intersects(T x, T y, T width, T height, T xpos, T ypos) {
		if(xpos < x)
			return false;
		if(xpos > x + width)
			return false;
		if(ypos < y)
			return false;
		if(ypos > y + height)
			return false;

		return true;
	}

	template<typename T> bool float_equals(const T lhs, const T rhs, const T diff = /* std::numeric_limits<double>::epsilon */ 0.001) {
		if(lhs == rhs)
			return true;
		if(abs(lhs - rhs) < diff)
			return true;
		
		return false;
	}
}
