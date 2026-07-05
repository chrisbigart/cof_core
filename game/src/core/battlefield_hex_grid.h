#pragma once

#include "core/game_config.h"

#include <set>
#include <vector>
#include <cmath>

typedef std::pair<int, int> point_t;
typedef std::pair<int8_t, int8_t> hex_location_t;
typedef std::pair<float, float> pointf_t;
typedef std::pair<double, double> pointd_t;

struct battlefield_unit_t;

struct battlefield_hex_t {
	/*const*/ int8_t x;
	/*const*/ int8_t y;
	bool passable = true;
	//int elevation;
	//effects (e.g. quicksand, firewall, etc.)
	battlefield_unit_t* unit = nullptr;
};

enum battlefield_direction_e {
	TOPLEFT = 0,
	TOPRIGHT,
	LEFT,
	RIGHT,
	BOTTOMLEFT,
	BOTTOMRIGHT
};

struct battlefield_hex_grid_t {
	battlefield_hex_t hexes[game_config::BATTLEFIELD_WIDTH][game_config::BATTLEFIELD_HEIGHT];

	battlefield_hex_grid_t() {
		for(uint y = 0; y < game_config::BATTLEFIELD_HEIGHT; y++) {
			for(uint x = 0; x < game_config::BATTLEFIELD_WIDTH; x++) {
				hexes[x][y].x = x;
				hexes[x][y].y = y;
			}
		}
	}
	
	void clear_units() {
		for(uint y = 0; y < game_config::BATTLEFIELD_HEIGHT; y++) {
			for(uint x = 0; x < game_config::BATTLEFIELD_WIDTH; x++) {
				hexes[x][y].unit = nullptr;
			}
		}
	}
	
	static point_t offset_to_axial(int x, int y) {
		auto q = x - (y + (y & 1)) / 2;
		auto r = y;
		return point_t(q, r);
	}
	
	static point_t axial_to_offset(int q, int r) {
		auto x = q + (r + (r & 1)) / 2;
		auto y = r;
		return point_t(x, y);
	}
	//int axial_distance()

	static pointf_t hex_to_pixel(int x, int y, float radius) {
		auto xpos = radius * std::sqrt(3) * (x - 0.5 * (y & 1));
		auto ypos = radius * 3 / 2 * y;
		return pointf_t((float)xpos, (float)ypos);
	}
	
	static pointd_t hex_to_pixel_d(int x, int y, double radius) {
		auto xpos = radius * std::sqrt(3) * (x - 0.5 * (y & 1));
		auto ypos = radius * 3 / 2 * y;
		return pointd_t(xpos, ypos);
	}
	
	static int distance(int x1, int y1, int x2, int y2) {
		auto a = offset_to_axial(x1, y1);
		auto b = offset_to_axial(x2, y2);
		point_t p(a.first - b.first, a.second - b.second);
		return (abs(p.first) + abs(p.first + p.second) + abs(p.second)) / 2;
	}

	static bool is_in_radius_of(int x1, int y1, int x2, int y2, int radius) {
		return distance(x1, y1, x2, y2) > radius ? false : true;
	}

	battlefield_hex_t* get_hex(int x, int y) {
		if(x < 0 || y < 0 || x >= (int)game_config::BATTLEFIELD_WIDTH || y >= (int)game_config::BATTLEFIELD_HEIGHT)
			return nullptr;

		return &hexes[x][y];
	}

	battlefield_hex_t* pixel_to_hex(int xpos, int ypos, float radius) {
		auto a = pixel_to_hex_coords(xpos, ypos, radius);
		return get_hex(a.first, a.second);
	}
	
	static point_t pixel_to_hex_coords(int xpos, int ypos, float radius) {
		float q = (std::sqrt(3) / 3 * xpos - 1. / 3 * ypos) / radius;
		float r = (2. / 3 * ypos) / radius;
		
		float x = q;
		float z = r;
		float y = -x - z;
		
		int rx = std::round(x);
		int ry = std::round(y);
		int rz = std::round(z);
		
		float x_diff = std::abs(rx - x);
		float y_diff = std::abs(ry - y);
		float z_diff = std::abs(rz - z);
		
		if (x_diff > y_diff && x_diff > z_diff) {
			rx = -ry - rz;
		} else if (y_diff > z_diff) {
			ry = -rx - rz;
		} else {
			rz = -rx - ry;
		}
		
		return axial_to_offset(rx, rz);
	}

//	static point_t pixel_to_hex_coords(int xpos, int ypos, float radius) {
//		auto q = (std::sqrt(3) / 3 * xpos - 1. / 3 * ypos) / radius;
//		auto r = (2. / 3 * ypos) / radius;
//
//		return axial_to_offset(q, r);
//	}
	const battlefield_hex_t* get_adjacent_hex(int x, int y, battlefield_direction_e direction, bool) const {
		return (const battlefield_hex_t*)((battlefield_hex_grid_t*)this)->get_adjacent_hex(x, y, direction);
	}
	
	battlefield_hex_t* get_adjacent_hex(int x, int y, battlefield_direction_e direction) {
		int parity = y & 1;
		switch(direction) {
		case TOPLEFT:
			return get_hex(parity ? x - 1 : x, y - 1);
		case TOPRIGHT:
			return get_hex(parity ? x : x + 1, y - 1);
		case LEFT:
			return get_hex(x - 1, y);
		case RIGHT:
			return get_hex(x + 1, y);
		case BOTTOMLEFT:
			return get_hex(parity ? x - 1 : x, y + 1);
		case BOTTOMRIGHT:
			return get_hex(parity ? x : x + 1, y + 1);
		default:
			return nullptr;
		}
	}

	battlefield_direction_e get_adjacent_hex_direction(int from_x, int from_y, int to_x, int to_y) {
		auto dx = to_x - from_x;
		auto dy = to_y - from_y;

		int parity = from_y & 1;

		if (dy == -1) {
			if(parity == 0) {
				if(dx == 0)
					return TOPLEFT;
				else if(dx == 1)
					return TOPRIGHT;
			}
			else { //parity == 1
				if(dx == -1)
					return TOPLEFT;
				else if(dx == 0)
					return TOPRIGHT;
			}
		}
		else if (dy == 0) {
			if (dx == -1)
				return LEFT;
			else if (dx == 1)
				return RIGHT;
		}
		else if (dy == 1) {
			if(parity == 0) {
				if(dx == 0)
					return BOTTOMLEFT;
				else if(dx == 1)
					return BOTTOMRIGHT;
			}
			else { //parity == 1
				if(dx == 0)
					return BOTTOMRIGHT;
				else if(dx == -1)
					return BOTTOMLEFT;
			}
		}

		return TOPLEFT;
	}

	std::vector<battlefield_hex_t*> get_neighbors(int x, int y, bool only_traversable = false) {
		std::vector<battlefield_hex_t*> neighbor_hexes;
		if(x < 0 || y < 0 || x > (int)game_config::BATTLEFIELD_WIDTH || y > (int)game_config::BATTLEFIELD_HEIGHT)
			return neighbor_hexes;

		battlefield_hex_t* h = nullptr;
		for(auto i = 0; i < 6; i++) {
			h = get_adjacent_hex(x, y, battlefield_direction_e((int)battlefield_direction_e::TOPLEFT + i));
			if(h && (!only_traversable || h->passable))
				neighbor_hexes.push_back(h);
		}
		
		return neighbor_hexes;
	}

	std::vector<battlefield_hex_t*> get_neighbors(int x, int y, int radius, bool include_origin = true) {
		std::vector<battlefield_hex_t*> neighbor_hexes;
		if(x < 0 || y < 0 || x >(int)game_config::BATTLEFIELD_WIDTH || y >(int)game_config::BATTLEFIELD_HEIGHT)
			return neighbor_hexes;

		for(uint ny = 0; ny < game_config::BATTLEFIELD_HEIGHT; ny++) {
			for(uint nx = 0; nx < game_config::BATTLEFIELD_WIDTH; nx++) {
			if(!is_in_radius_of(x, y, nx, ny, radius))
				continue;

			if(!include_origin && (nx == x && ny == y))
				continue;
			
			neighbor_hexes.push_back(&hexes[nx][ny]);
			}
		}

		return neighbor_hexes;
	}
	
	std::set<battlefield_hex_t*> get_movement_range_flyer(const battlefield_unit_t& unit, int radius);
	std::set<battlefield_hex_t*> get_movement_range(const battlefield_unit_t& unit, int radius, bool flyer);
	
};
