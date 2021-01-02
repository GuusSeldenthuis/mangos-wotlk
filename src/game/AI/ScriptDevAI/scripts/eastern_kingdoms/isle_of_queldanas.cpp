/* This file is part of the ScriptDev2 Project. See AUTHORS file for Copyright information
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

/* ScriptData
SDName: Isle_of_Queldanas
SD%Complete: 100
SDComment: Quest support: 11524, 11525
SDCategory: Isle Of Quel'Danas
EndScriptData */

/* ContentData
npc_converted_sentry
EndContentData */

#include "AI/ScriptDevAI/include/sc_common.h"

/*######
## npc_converted_sentry
######*/

enum
{
    SAY_CONVERTED_1             = -1000188,
    SAY_CONVERTED_2             = -1000189,

    SPELL_CONVERT_CREDIT        = 45009,
    TIME_PET_DURATION           = 7500
};

struct npc_converted_sentryAI : public ScriptedAI
{
    npc_converted_sentryAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiCreditTimer;

    void Reset() override
    {
        m_uiCreditTimer = 2500;
    }

    void MoveInLineOfSight(Unit* /*pWho*/) override {}

    void UpdateAI(const uint32 uiDiff) override
    {
        if (m_uiCreditTimer)
        {
            if (m_uiCreditTimer <= uiDiff)
            {
                DoScriptText(urand(0, 1) ? SAY_CONVERTED_1 : SAY_CONVERTED_2, m_creature);

                DoCastSpellIfCan(m_creature, SPELL_CONVERT_CREDIT);
                ((Pet*)m_creature)->SetDuration(TIME_PET_DURATION);
                m_uiCreditTimer = 0;
            }
            else
                m_uiCreditTimer -= uiDiff;
        }
    }
};

UnitAI* GetAI_npc_converted_sentry(Creature* pCreature)
{
    return new npc_converted_sentryAI(pCreature);
}

/*######
# npc_shattered_sun_fighter
######*/

enum
{
    ZONEID_ISLE_OF_QUELDANAS                = 4080,
    NPC_SHATTERED_SUN_WARRIOR               = 25115,
    NPC_SHATTERED_SUN_MARKSMAN              = 24938,
    // relay dbscripts - each of which cast 1 of 4 transformation spells based on current Sun's Reach game event information
    SCRIPT_TRANSFORM_ARCHER_BE_MALE         = 2493801,
    SCRIPT_TRANSFORM_ARCHER_BE_FEMALE       = 2493802,
    SCRIPT_TRANSFORM_ARCHER_DRAENEI_MALE    = 2493803,
    SCRIPT_TRANSFORM_ARCHER_DRAENEI_FEMALE  = 2493804,
    SCRIPT_TRANSFORM_WARRIOR_BE_MALE        = 2511502,
    SCRIPT_TRANSFORM_WARRIOR_BE_FEMALE      = 2511501,
    SCRIPT_TRANSFORM_WARRIOR_DRAENEI_MALE   = 2511504,
    SCRIPT_TRANSFORM_WARRIOR_DRAENEI_FEMALE = 2511503,
};

struct npc_shattered_sun_fighterAI : public ScriptedAI
{
    npc_shattered_sun_fighterAI(Creature* creature) : ScriptedAI(creature)
    {
        if (creature->GetZoneId() == ZONEID_ISLE_OF_QUELDANAS) // let the spawns in Shattrath be handled via movement dbscript
        {
            uint32 transformScriptId = 0;
            if (creature->GetEntry() == NPC_SHATTERED_SUN_MARKSMAN)
            {
                switch (urand(0, 3))
                {
                    case 0:
                        transformScriptId = SCRIPT_TRANSFORM_ARCHER_BE_MALE;
                        m_uiMarksmanRace = RACE_BLOODELF;
                        break;
                    case 1:
                        transformScriptId = SCRIPT_TRANSFORM_ARCHER_BE_FEMALE;
                        m_uiMarksmanRace = RACE_BLOODELF;
                        break;
                    case 2:
                        transformScriptId = SCRIPT_TRANSFORM_ARCHER_DRAENEI_MALE;
                        m_uiMarksmanRace = RACE_DRAENEI;
                        break;
                    case 3:
                        transformScriptId = SCRIPT_TRANSFORM_ARCHER_DRAENEI_FEMALE;
                        m_uiMarksmanRace = RACE_DRAENEI;
                        break;
                }
            }
            else if (creature->GetEntry() == NPC_SHATTERED_SUN_WARRIOR)
            {
                switch (urand(0, 3))
                {
                    case 0: transformScriptId = SCRIPT_TRANSFORM_WARRIOR_BE_MALE; break;
                    case 1: transformScriptId = SCRIPT_TRANSFORM_WARRIOR_BE_FEMALE; break;
                    case 2: transformScriptId = SCRIPT_TRANSFORM_WARRIOR_DRAENEI_MALE; break;
                    case 3: transformScriptId = SCRIPT_TRANSFORM_WARRIOR_DRAENEI_FEMALE; break;
                }
            }
            if (transformScriptId)
                creature->GetMap()->ScriptsStart(sRelayScripts, transformScriptId, m_creature, creature);
        }
        Reset();
    }
    
    uint32 m_uiMarksmanRace;

    void Reset() override {}
    void UpdateAI(const uint32 diff) override {}
};

struct npc_shattered_sun_warriorAI : public npc_shattered_sun_fighterAI
{
    npc_shattered_sun_warriorAI(Creature* creature) : npc_shattered_sun_fighterAI(creature) { Reset(); }
    void Reset() override {}
    void UpdateAI(const uint32 diff) override {}
};

enum
{
    SPELL_SHOOT_BLOODELF_NW = 45219,
    SPELL_SHOOT_DRAENEI_NW  = 45223,
    SPELL_SHOOT_BLOODELF_SE = 45229,
    SPELL_SHOOT_DRAENEI_SE  = 45233,
};

struct npc_shattered_sun_marksmanAI : public npc_shattered_sun_fighterAI
{
    npc_shattered_sun_marksmanAI(Creature* creature) : npc_shattered_sun_fighterAI(creature) { Reset(); }

    uint32 m_uiShootTimer;
    uint32 m_uiShootSpell;

    void Reset() override
    {
        SetCombatMovement(false);
    }

    void ReceiveAIEvent(AIEventType eventType, Unit* sender, Unit* /*invoker*/, uint32 /*miscValue*/) override
    {
        if (sender->GetObjectGuid() != m_creature->GetObjectGuid())
            return;
        // Facing North-West
        if (eventType == AI_EVENT_CUSTOM_A)
            m_uiShootSpell = m_uiMarksmanRace == RACE_BLOODELF ? SPELL_SHOOT_BLOODELF_NW : SPELL_SHOOT_DRAENEI_NW;
        // Facing South-East
        else if (eventType == AI_EVENT_CUSTOM_B)
            m_uiShootSpell = m_uiMarksmanRace == RACE_BLOODELF ? SPELL_SHOOT_BLOODELF_SE : SPELL_SHOOT_DRAENEI_SE;
        m_uiShootTimer = urand(1000, 10000);
    }

    void UpdateAI(const uint32 diff) override
    {
        if (m_uiShootTimer)
        {
            if (m_uiShootTimer <= diff)
            {
                DoCastSpellIfCan(m_creature, m_uiShootSpell);
                m_uiShootTimer = urand(8000, 14000);
            }
            else
                m_uiShootTimer -= diff;
        }
    }
};

void AddSC_isle_of_queldanas()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "npc_converted_sentry";
    pNewScript->GetAI = &GetAI_npc_converted_sentry;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_shattered_sun_marksman";
    pNewScript->GetAI = &GetNewAIInstance<npc_shattered_sun_marksmanAI>;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_shattered_sun_warrior";
    pNewScript->GetAI = &GetNewAIInstance<npc_shattered_sun_warriorAI>;
    pNewScript->RegisterSelf();
}
