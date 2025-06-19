#pragma once

enum resource_e : uint8_t;

struct resource_value_t {
	resource_e type = RESOURCE_RANDOM;
	uint32_t value = 0;
};

static constexpr int MAX_RESOURCE_VALUE = 2000000000;
static constexpr int NUMBER_OF_RESOURCES = 7;

QDataStream& operator<<(QDataStream& stream, const resource_value_t& resource);
QDataStream& operator>>(QDataStream& stream, resource_value_t& resource);

struct resource_group_t {
	std::array<resource_value_t, NUMBER_OF_RESOURCES> values; //todo probably change all this
	
	resource_group_t() {
		for(int i = 0; i < NUMBER_OF_RESOURCES; i++)
			values[RESOURCE_RANDOM + i].type = (resource_e)(RESOURCE_RANDOM + i + 1);
	}
	resource_group_t(uint wood, uint ore, uint gems, uint mercury, uint crystal, uint sulfur, uint gold) {
		for(int i = 0; i < NUMBER_OF_RESOURCES; i++)
			values[RESOURCE_RANDOM + i].type = (resource_e)(RESOURCE_RANDOM + i + 1);
		
		set_value_for_type(RESOURCE_WOOD, wood);
		set_value_for_type(RESOURCE_ORE, ore);
		set_value_for_type(RESOURCE_GEMS, gems);
		set_value_for_type(RESOURCE_MERCURY, mercury);
		set_value_for_type(RESOURCE_CRYSTALS, crystal);
		set_value_for_type(RESOURCE_SULFUR, sulfur);
		set_value_for_type(RESOURCE_GOLD, gold);
	}
	
	uint32_t get_value_for_type(resource_e resource) const {
		for(auto& r : values) {
			if(r.type == resource)
				return r.value;
		}
		return 0;
	}
	
	void set_value_for_type(resource_e resource, uint32_t amount) {
		for(auto& r : values) {
			if(r.type == resource)
				r.value = amount;
		}
	}
	
	void add_value(resource_e type, uint32_t amount) {
		auto val = get_value_for_type(type);
		if((val + amount < val) || (val + amount >= MAX_RESOURCE_VALUE))
			val = MAX_RESOURCE_VALUE;
		else
			val += amount;
		
		set_value_for_type(type, val);
		//return val;
	}
	
	void sub_value(resource_e type, uint32_t amount) {
		auto val = get_value_for_type(type);
		if(amount >= val)
			val = 0;
		else
			val -= amount;
		
		set_value_for_type(type, val);
		//return val;
	}
	
	bool covers_cost(const resource_group_t& cost) const {
		for(int i = 0; i < NUMBER_OF_RESOURCES; i++) {
			if(cost.values[i].value > values[i].value)
				return false;
		}
		
		return true;
	}
	
	resource_group_t& operator+=(const resource_group_t& rhs) {
		for(int i = 0; i < NUMBER_OF_RESOURCES; i++) {
			values[i].value += rhs.values[i].value;
			values[i].value = std::max(values[i].value, 0u); //
		}
		return *this;
	}
	
	resource_group_t& operator-=(const resource_group_t& rhs) {
		for(int i = 0; i < NUMBER_OF_RESOURCES; i++) {
			values[i].value -= rhs.values[i].value;
			values[i].value = std::max(values[i].value, 0u);
		}
		return *this;
	}
	
	bool operator<(const resource_group_t& rhs) const {
		for(int i = 0; i < NUMBER_OF_RESOURCES; i++) {
			if(values[i].value > rhs.values[i].value)
				return false;
		}
		
		return true;
	}
	
	bool operator<=(const resource_group_t& rhs) const {
		for(int i = 0; i < NUMBER_OF_RESOURCES; i++) {
			if(values[i].value > rhs.values[i].value)
				return false;
		}
		
		return true;
	}
	
	template<typename T> resource_group_t operator*(const T& multi) const {
		resource_group_t res;
		for(int i = 0; i < NUMBER_OF_RESOURCES; i++) {
			res.values[i].value = values[i].value * multi;
		}
		return res;
	}
};

QDataStream& operator<<(QDataStream& stream, const resource_group_t& resource);
QDataStream& operator>>(QDataStream& stream, resource_group_t& resource);
