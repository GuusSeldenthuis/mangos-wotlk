/*
* This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "Spells/Scripts/SpellScript.h"
#include "Spells/SpellAuras.h"

struct SealOfTheCrusader : public AuraScript
{
    void OnApply(Aura* aura, bool apply) const
    {
        if (aura->GetEffIndex() != EFFECT_INDEX_1)
            return;

        // Seal of the Crusader damage reduction
        // SotC increases attack speed but reduces damage to maintain the same DPS
        float reduction = (-100.0f * aura->GetModifier()->m_amount) / (aura->GetModifier()->m_amount + 100.0f);
        aura->GetTarget()->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, reduction, apply);
    }
};

struct spell_judgement : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        Unit* unitTarget = spell->GetUnitTarget();
        if (!unitTarget || !unitTarget->IsAlive())
            return;

        Unit* caster = spell->GetCaster();

        uint32 spellId1;
        uint32 spellId2 = 0;

        // Judgement self add switch
        switch (spell->m_spellInfo->Id)
        {
            case 53407: spellId1 = 20184; break;    // Judgement of Justice
            case 20271:                             // Judgement of Light
            case 57774: spellId1 = 20185; break;    // Judgement of Light
            case 53408: spellId1 = 20186; break;    // Judgement of Wisdom
            default:
                sLog.outError("Unsupported Judgement (seal trigger) spell (Id: %u) in Spell::EffectScriptEffect", spell->m_spellInfo->Id);
                return;
        }

        // offensive seals have aura dummy in 2 effect
        Unit::AuraList const& m_dummyAuras = caster->GetAurasByType(SPELL_AURA_DUMMY);
        for (auto m_dummyAura : m_dummyAuras)
        {
            // search seal (offensive seals have judgement's aura dummy spell id in 2 effect
            if (m_dummyAura->GetEffIndex() != EFFECT_INDEX_2 || !IsSealSpell(m_dummyAura->GetSpellProto()))
                continue;
            spellId2 = m_dummyAura->GetModifier()->m_amount;
            SpellEntry const* judge = sSpellTemplate.LookupEntry<SpellEntry>(spellId2);
            if (!judge)
                continue;
            break;
        }

        // if there were no offensive seals than there is seal with proc trigger aura
        if (!spellId2)
        {
            Unit::AuraList const& procTriggerAuras = caster->GetAurasByType(SPELL_AURA_PROC_TRIGGER_SPELL);
            for (auto procTriggerAura : procTriggerAuras)
            {
                if (procTriggerAura->GetEffIndex() != EFFECT_INDEX_0 || !IsSealSpell(procTriggerAura->GetSpellProto()))
                    continue;
                spellId2 = 54158;
                break;
            }
        }

        if (spellId1)
            caster->CastSpell(unitTarget, spellId1, TRIGGERED_OLD_TRIGGERED);

        if (spellId2)
            caster->CastSpell(unitTarget, spellId2, TRIGGERED_OLD_TRIGGERED);

        if (caster->HasAura(37188)) // improved judgement
            caster->CastSpell(nullptr, 43838, TRIGGERED_OLD_TRIGGERED);
    }
};

struct IncreasedHolyLightHealing : public AuraScript
{
    void OnApply(Aura* aura, bool apply) const
    {
        aura->GetTarget()->RegisterScriptedLocationAura(aura, SCRIPT_LOCATION_SPELL_HEALING_DONE, apply);
    }

    void OnDamageCalculate(Aura* aura, int32& advertisedBenefit, float& /*totalMod*/) const override
    {
        advertisedBenefit += aura->GetModifier()->m_amount;
    }
};

struct RighteousDefense : public SpellScript
{
    bool OnCheckTarget(const Spell* /*spell*/, Unit* target, SpellEffectIndex /*eff*/) const override
    {
        if (!target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
            return false;

        return true;
    }

    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return;

        Unit* unitTarget = spell->GetUnitTarget();
        if (!unitTarget)
            return;
        Unit* caster = spell->GetCaster();

        // non-standard cast requirement check
        if (unitTarget->getAttackers().empty())
        {
            caster->RemoveSpellCooldown(*spell->m_spellInfo, true);
            spell->SendCastResult(SPELL_FAILED_TARGET_AFFECTING_COMBAT);
            return;
        }

        // not empty (checked), copy
        Unit::AttackerSet attackers = unitTarget->getAttackers();

        // selected from list 3
        size_t size = std::min(size_t(3), attackers.size());
        for (uint32 i = 0; i < size; ++i)
        {
            Unit::AttackerSet::iterator aItr = attackers.begin();
            std::advance(aItr, urand() % attackers.size());
            caster->CastSpell((*aItr), 31790, TRIGGERED_NONE); // step 2
            attackers.erase(aItr);
        }
    }
};

void LoadPaladinScripts()
{
    RegisterAuraScript<IncreasedHolyLightHealing>("spell_increased_holy_light_healing");
    RegisterSpellScript<spell_judgement>("spell_judgement");
    RegisterSpellScript<RighteousDefense>("spell_righteous_defense");
    RegisterAuraScript<SealOfTheCrusader>("spell_seal_of_the_crusader");
}