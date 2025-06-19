#include "core/artifact.h"
#include "core/utils.h"

bool artifact_t::is_spell_scroll(artifact_e artifact_id) {
	return ((int)artifact_id & ARTIFACT_SPELL_SCROLL);
}

spell_e artifact_t::get_spell_scroll_spell_id(artifact_e artifact_id) {
	if(!is_spell_scroll(artifact_id))
		return SPELL_UNKNOWN;

	return (spell_e)((int)artifact_id & ~ARTIFACT_SPELL_SCROLL);
}

artifact_e artifact_t::get_artifact_id_for_spell_scroll(spell_e spell_id) {
	if(spell_id == SPELL_UNKNOWN)
		return ARTIFACT_NONE;

	return (artifact_e)((int)ARTIFACT_SPELL_SCROLL | spell_id);
}

bool artifact_t::is_empty() const {
	return id == ARTIFACT_NONE;
}

bool artifact_t::does_slot_match(artifact_slot_e artifact_slot) const {
	if(artifact_slot == SLOT_RING_2 && slot == SLOT_RING)
		return true;
	if(artifact_slot == SLOT_RING && slot == SLOT_RING_2)
		return true;
	if(artifact_slot >= SLOT_TRINKET && slot >= SLOT_TRINKET)
		return true;
	if(artifact_slot == slot)
		return true;
	
	return false;
}

std::string artifact_t::get_slot_name(artifact_slot_e slot) {
	auto adjusted_slot = slot;
	if(adjusted_slot == SLOT_RING_2)
		adjusted_slot = SLOT_RING;
	if(adjusted_slot > SLOT_TRINKET)
		adjusted_slot = SLOT_TRINKET;
	
	auto slotname = QString::fromUtf8(magic_enum::enum_name((artifact_slot_e)adjusted_slot).data());
	slotname.remove("SLOT_");
	return utils::to_camel_case(slotname.replace('_', ' ').toLower()).toStdString();
}


int8_t artifact_t::get_attack_bonus() const {
	for(auto& e : effects) {
		if(e.type == EFFECT_ATTACK_BONUS)
			return e.magnitude_1;
	}
	
	return 0;
}

int8_t artifact_t::get_defense_bonus() const {
	for(auto& e : effects) {
		if(e.type == EFFECT_DEFENSE_BONUS)
			return e.magnitude_1;
	}
	
	return 0;
}

int8_t artifact_t::get_power_bonus() const {
	for(auto& e : effects) {
		if(e.type == EFFECT_POWER_BONUS)
			return e.magnitude_1;
	}
	
	return 0;
}

int8_t artifact_t::get_knowledge_bonus() const {
	for(auto& e : effects) {
		if(e.type == EFFECT_KNOWLEDGE_BONUS)
			return e.magnitude_1;
	}
	
	return 0;
}

int8_t artifact_t::get_luck_bonus() const {
	for(auto& e : effects) {
		if(e.type == EFFECT_LUCK_BONUS)
			return e.magnitude_1;
	}
	
	return 0;
}

int8_t artifact_t::get_morale_bonus() const {
	for(auto& e : effects) {
		if(e.type == EFFECT_MORALE_BONUS)
			return e.magnitude_1;
	}
	
	return 0;
}
