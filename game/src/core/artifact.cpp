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

std::string artifact_t::get_slot_name(artifact_slot_e slot, bool include_slot_text /*= true*/) {
	auto adjusted_slot = slot;
	if(adjusted_slot > SLOT_TRINKET)
		adjusted_slot = SLOT_TRINKET;

	switch (adjusted_slot) {
		case SLOT_HELM:
			return (include_slot_text ? QObject::tr("Helm Slot") : QObject::tr("Helm")).toStdString();
		case SLOT_NECKLACE:
			return (include_slot_text ? QObject::tr("Necklace Slot") : QObject::tr("Necklace")).toStdString();
		case SLOT_SHOULDERS:
			return (include_slot_text ? QObject::tr("Shoulders Slot") : QObject::tr("Shoulders")).toStdString();
		case SLOT_WEAPON:
			return (include_slot_text ? QObject::tr("Weapon Slot") : QObject::tr("Weapon")).toStdString();
		case SLOT_SHIELD:
			return (include_slot_text ? QObject::tr("Shield Slot") : QObject::tr("Shield")).toStdString();
		case SLOT_GLOVES:
			return (include_slot_text ? QObject::tr("Gloves Slot") : QObject::tr("Gloves")).toStdString();
		case SLOT_RING:
		case SLOT_RING_2:
			return (include_slot_text ? QObject::tr("Ring Slot") : QObject::tr("Ring")).toStdString();
		case SLOT_ARMOUR:
			return (include_slot_text ? QObject::tr("Armour Slot") : QObject::tr("Armour")).toStdString();
		case SLOT_BELT:
			return (include_slot_text ? QObject::tr("Belt Slot") : QObject::tr("Belt")).toStdString();
		case SLOT_BOOTS:
			return (include_slot_text ? QObject::tr("Boots Slot") : QObject::tr("Boots")).toStdString();
		case SLOT_TRINKET:
			return (include_slot_text ? QObject::tr("Trinket Slot") : QObject::tr("Trinket")).toStdString();
		default:
			return QObject::tr("Unknown Slot").toStdString();
	}
}

std::string artifact_t::get_rarity_name(artifact_rarity_e rarity) {
	switch(rarity) {
		case RARITY_COMMON:
			return QObject::tr("Common").toStdString();
		case RARITY_UNCOMMON:
			return QObject::tr("Uncommon").toStdString();
		case RARITY_RARE:
			return QObject::tr("Rare").toStdString();
		case RARITY_EXCEPTIONAL:
			return QObject::tr("Exceptional").toStdString();
		case RARITY_LEGENDARY:
			return QObject::tr("Legendary").toStdString();
		default:
			return QObject::tr("Unknown").toStdString();
	}
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
