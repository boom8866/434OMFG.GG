/*
 * Copyright (C) 2005-2011 MaNGOS <http://www.getmangos.com/>
 *
 * Copyright (C) 2008-2011 Trinity <http://www.trinitycore.org/>
 *
 * Copyright (C) 2006-2011 ScriptDev2 <http://www.scriptdev2.com/>
 *
 * Copyright (C) 2010-2011 Project Trinity <http://www.projecttrinity.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* ToDo
 * Make Overdrive Randomize (Like Marrowgar)
 * Spell Harvest needs Voids + Charge (need Retail Infos)
 * Correct Timers
 * Add the OFF_LINE Spell, but first the Creatures to Activate Foe Reaper 5000
 */

#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "deadmines.h"

enum eSpell
{
	SPELL_ENERGIZE = 89132,
	SPELL_ENERGIZED = 91733, // -> 89200,
	SPELL_ON_FIRE = 91737,
	SPELL_COSMETIC_STAND = 88906,

	// BOSS spells
	SPELL_OVERDRIVE = 88481, // 88484
	SPELL_OFFLINE = 88348,
	SPELL_HARVEST = 88495,
	SPELL_HARVEST_AURA = 88497,

	SPELL_HARVEST_SWEEP = 88521,
	SPELL_HARVEST_SWEEP_H = 91718,

	SPELL_REAPER_STRIKE = 88490,
	SPELL_REAPER_STRIKE_H = 91717,

	SPELL_SAFETY_REST_OFFLINE = 88522,
	SPELL_SAFETY_REST_OFFLINE_H = 91720,

	SPELL_SUMMON_MOLTEN_SLAG = 91839,

	// heroic
	SPELL_SUMMON_SCORIES = 91839,
	SPELL_SCORIES_SHIELD = 91815,

	// scoried
	SPELL_FIXATE = 91830,
};

enum eAchievementMisc
{
	ACHIEVEMENT_PROTOTYPE_PRODIGY = 5368,
	NPC_PROTOTYPE_REAPER = 49208,
	DATA_ACHIV_PROTOTYPE_PRODIGY = 1,
};

const Position OverdrivePoint =
{
	-184.3978f, -565.997f, 19.30717f, 0.0f
};

enum Events
{
	EVENT_NULL,
	EVENT_START,
	EVENT_START_2,
	EVENT_SRO,
	EVENT_OVERDRIVE,
	EVENT_HARVEST,
	EVENT_HARVEST_SWEAP,
	EVENT_REAPER_STRIKE,
	EVENT_SAFETY_OFFLINE,
	EVENT_SWITCH_TARGET,
	EVENT_MOLTEN_SLAG,
	EVENT_START_ATTACK,

	// scories
	EVENT_FIXATE,
	EVENT_FIXATE_CHECK,
};

enum eSays
{
	SAY_CAST_OVERDRIVE = 0,
	SAY_JUSTDIED = 1,
	SAY_KILLED_UNIT = 2,
	SAY_EVENT_START = 3,

	SAY_HARVEST_SWEAP = 4,
	SAY_CAST_OVERDRIVE_E = 5,
	SAY_EVENT_SRO = 6,
};

#define MONSTER_START "A stray jolt from the Foe Reaper has distrupted the foundry controls!"
#define MONSTER_SLAG "The monster slag begins to bubble furiously!"


Position const HarvestSpawn[] =
{
	{ -229.72f, -590.37f, 19.38f, 0.71f },
	{ -229.67f, -565.75f, 19.38f, 5.98f },
	{ -205.53f, -552.74f, 19.38f, 4.53f },
	{ -182.74f, -565.96f, 19.38f, 3.35f },
};

Position const PrototypeSpawn = { -200.499f, -553.946f, 51.2295f, 4.32651f };


class boss_foe_reaper_5000 : public CreatureScript
{
public:
	boss_foe_reaper_5000() : CreatureScript("boss_foe_reaper_5000") { }

	CreatureAI* GetAI(Creature* creature) const
	{
		return new boss_foe_reaper_5000AI(creature);
	}

	struct boss_foe_reaper_5000AI : public BossAI
	{
		boss_foe_reaper_5000AI(Creature* creature) : BossAI(creature, BOSS_FOE_REAPER_5000_DATA)
		{
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_STUNNED);
			prototypeGUID = 0;
		}

		uint32 eventId;
		uint32 Step;
		uint64 prototypeGUID;

		bool Below;

		void Reset()
		{
			if (!me)
				return;

			_Reset();
			me->SetReactState(REACT_PASSIVE);
			me->SetPower(POWER_ENERGY, 100);
			me->SetMaxPower(POWER_ENERGY, 100);
			me->setPowerType(POWER_ENERGY);
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);
			Step = 0;
			Below = false;

			instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

			me->SetFullHealth();
			me->SetOrientation(4.273f);

			DespawnOldWatchers();
			RespawnWatchers();

			if (IsHeroic())
			{
				if (Creature* Reaper = me->GetCreature(*me, prototypeGUID))
					Reaper->DespawnOrUnsummon();

				if (Creature *prototype = me->SummonCreature(NPC_PROTOTYPE_REAPER, PrototypeSpawn, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000))
				{
					prototype->SetFullHealth();
					prototypeGUID = prototype->GetGUID();
				}
			}
		}

		void EnterCombat(Unit* /*who*/)
		{
			_EnterCombat();
			events.ScheduleEvent(EVENT_REAPER_STRIKE, 10000);
			events.ScheduleEvent(EVENT_OVERDRIVE, 11000);
			events.ScheduleEvent(EVENT_HARVEST, 25000);

			if (IsHeroic())
			{
				events.ScheduleEvent(EVENT_MOLTEN_SLAG, 15000);
			}

			if (!me)
				return;

			instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
		}

		void JustDied(Unit* /*Killer*/)
		{
			if (!me)
				return;

			_JustDied();
			DespawnOldWatchers();
			Talk(SAY_JUSTDIED);
			instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

			if (IsHeroic())
				if (Creature* Reaper = me->GetCreature(*me, prototypeGUID))
					Reaper->DespawnOrUnsummon();
		}

		uint32 GetData(uint32 type) const
		{
			if (type == DATA_ACHIV_PROTOTYPE_PRODIGY)
			{
				if (!IsHeroic())
					return false;

				if (Creature *prototype_reaper = me->GetCreature(*me, prototypeGUID))
					if (prototype_reaper->GetHealth() >= 0.9 * prototype_reaper->GetMaxHealth())
						return true;
			}

			return false;
		}

		void JustReachedHome()
		{
			if (!me)
				return;

			_JustReachedHome();
			Talk(SAY_KILLED_UNIT);
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_STUNNED);
			instance->SetBossState(BOSS_FOE_REAPER_5000_DATA, FAIL);
		}

		void DespawnOldWatchers()
		{
			std::list<Creature*> reapers;
			me->GetCreatureListWithEntryInGrid(reapers, 47403, 250.0f);

			reapers.sort(Trinity::ObjectDistanceOrderPred(me));
			for (std::list<Creature*>::iterator itr = reapers.begin(); itr != reapers.end(); ++itr)
			{
				if ((*itr) && (*itr)->GetTypeId() == TYPEID_UNIT)
				{
					(*itr)->DespawnOrUnsummon();
				}
			}
		}

		void RespawnWatchers()
		{
			for (uint8 i = 0; i<4; ++i)
			{
				me->SummonCreature(47403, HarvestSpawn[i], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
			}
		}

		void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
		{
			if (!spell || !me)
				return;

			if (spell->Id == SPELL_ENERGIZE)
			{
				if (Step == 3)
				{
					events.ScheduleEvent(EVENT_START, 100);
				}
				Step++;
			}
		}

		void MovementInform(uint32 /*type*/, uint32 id)
		{
			if (id == 0)
			{
				if (Creature* HarvestTarget = me->FindNearestCreature(NPC_HARVEST_TARGET, 200.0f, true))
				{
					//DoCast(HarvestTarget, IsHeroic() ? SPELL_HARVEST_SWEEP_H : SPELL_HARVEST_SWEEP);
					me->RemoveAurasDueToSpell(SPELL_HARVEST_AURA);
					events.ScheduleEvent(EVENT_START_ATTACK, 1000);
				}
			}
		}

		void HarvestChase()
		{
			if (Creature* HarvestTarget = me->FindNearestCreature(NPC_HARVEST_TARGET, 200.0f, true))
			{
				me->SetSpeed(MOVE_RUN, 3.0f, true);
				me->GetMotionMaster()->MoveCharge(HarvestTarget->GetPositionX(), HarvestTarget->GetPositionY(), HarvestTarget->GetPositionZ(), 5.0f, 0);
				HarvestTarget->DespawnOrUnsummon(8500);
			}
		}

		void UpdateAI(uint32 const uiDiff)
		{
			if (!me)
				return;

			DoMeleeAttackIfReady();

			events.Update(uiDiff);

			while (uint32 eventId = events.ExecuteEvent())
			{
				switch (eventId)
				{
				case EVENT_START:
					Talk(SAY_EVENT_START);
					me->AddAura(SPELL_ENERGIZED, me);
					me->MonsterTextEmote(MONSTER_START, 0, true);
					events.ScheduleEvent(EVENT_START_2, 5000);
					break;

				case EVENT_START_2:
					me->MonsterTextEmote(MONSTER_SLAG, 0, true);
					me->SetHealth(me->GetMaxHealth());
					DoZoneInCombat();
					me->SetReactState(REACT_AGGRESSIVE);
					me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
					me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
					me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
					me->RemoveAurasDueToSpell(SPELL_ENERGIZED);
					events.ScheduleEvent(EVENT_SRO, 1000);
					break;

				case EVENT_SRO:
					me->RemoveAurasDueToSpell(SPELL_OFFLINE);

					if (Player* victim = me->FindNearestPlayer(40.0f))
						me->Attack(victim, false);
					break;

				case EVENT_START_ATTACK:
					me->RemoveAurasDueToSpell(SPELL_HARVEST_AURA);
					me->SetSpeed(MOVE_RUN, 2.0f, true);
					if (Player* victim = me->FindNearestPlayer(40.0f))
						me->Attack(victim, true);
					break;

				case EVENT_OVERDRIVE:
					if (!UpdateVictim())
						return;

					me->MonsterTextEmote("|TInterface\\Icons\\ability_whirlwind.blp:20|tFoe Reaper 5000 begins to activate |cFFFF0000|Hspell:91716|h[Overdrive]|h|r!", 0, true);
					me->AddAura(SPELL_OVERDRIVE, me);
					me->SetSpeed(MOVE_RUN, 4.0f, true);
					events.ScheduleEvent(EVENT_SWITCH_TARGET, 1500);
					events.ScheduleEvent(EVENT_OVERDRIVE, 45000);
					break;

				case EVENT_SWITCH_TARGET:
					if (Unit* victim = SelectTarget(SELECT_TARGET_RANDOM, 0, 150, true))
						me->Attack(victim, true);

					if (me->HasAura(SPELL_OVERDRIVE))
					{
						events.ScheduleEvent(EVENT_SWITCH_TARGET, 1500);
					}
					break;

				case EVENT_HARVEST:
					if (!UpdateVictim())
						return;

					if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150, true))
						me->CastSpell(target, SPELL_HARVEST);

					events.RescheduleEvent(EVENT_HARVEST_SWEAP, 5500);
					break;

				case EVENT_HARVEST_SWEAP:
					if (!UpdateVictim())
						return;

					HarvestChase();
					Talk(SAY_HARVEST_SWEAP);
					events.ScheduleEvent(EVENT_START_ATTACK, 8000);
					events.RescheduleEvent(EVENT_HARVEST, 45000);
					break;

				case EVENT_REAPER_STRIKE:
					if (!UpdateVictim())
						return;

					if (Unit* victim = me->getVictim())
					{
						if (me->IsWithinDist(victim, 25.0f))
						{
							DoCast(victim, IsHeroic() ? SPELL_REAPER_STRIKE_H : SPELL_REAPER_STRIKE);
						}
					}
					events.ScheduleEvent(EVENT_REAPER_STRIKE, urand(9000, 12000));
					break;

				case EVENT_MOLTEN_SLAG:
					me->MonsterTextEmote(MONSTER_SLAG, 0, true);
					me->CastSpell(-213.21f, -576.85f, 20.97f, SPELL_SUMMON_MOLTEN_SLAG, false);
					events.ScheduleEvent(EVENT_MOLTEN_SLAG, 20000);
					break;

				case EVENT_SAFETY_OFFLINE:
					Talk(SAY_EVENT_SRO);
					DoCast(me, IsHeroic() ? SPELL_SAFETY_REST_OFFLINE_H : SPELL_SAFETY_REST_OFFLINE);
					break;
				}

				if (HealthBelowPct(30) && !Below)
				{
					events.ScheduleEvent(EVENT_SAFETY_OFFLINE, 0);
					Below = true;
				}
			}
		}
	};
};

class npc_scormies : public CreatureScript
{
public:
    npc_scormies() : CreatureScript("npc_scormies") { }

    struct npc_scormiesAI : public ScriptedAI
    {
        npc_scormiesAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            _fixateTarget = 0;
            me->CastSpell(me, SPELL_SCORIES_SHIELD, true);
            _events.Reset();
            _events.ScheduleEvent(EVENT_FIXATE, 30000);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_FIXATE:
                        if(Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me)))
                        {
                            me->CastSpell(target, SPELL_FIXATE, true);
                            _fixateTarget = target->GetGUID();
                        }
                        _events.ScheduleEvent(EVENT_FIXATE_CHECK, 1000);
                        _events.ScheduleEvent(EVENT_FIXATE, 30000);
                        break;
                    case EVENT_FIXATE_CHECK:
                        if (_fixateTarget != 0)
                        {
                            if (Unit *target = Unit::GetUnit(*me, _fixateTarget))
                            {
                                if (target->HasAura(SPELL_FIXATE) && me->getVictim() != target)
                                    me->AddThreat(target, 50000000.0f);
                                else if (!target->HasAura(SPELL_FIXATE))
                                {
                                    me->getThreatManager().addThreat(target, -90000000);
                                    _fixateTarget = 0;
                                }
                            }
                            else
                                _fixateTarget = 0;
                            if (_fixateTarget)
                                _events.ScheduleEvent(EVENT_FIXATE_CHECK, 1000);
                        }
                        break;
                    default:
                        break;
                }
            }
        }

    private :
        InstanceScript *instance;
        EventMap _events;
        uint64 _fixateTarget;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_scormiesAI (creature);
    }
};

// 16210
class achievement_prototype_prodigy : public AchievementCriteriaScript
{
public:
    achievement_prototype_prodigy() : AchievementCriteriaScript("achievement_prototype_prodigy") { }

    bool OnCheck(Player* /*source*/, Unit* target)
    {
        if (!target)
            return false;
        if (InstanceScript *instance = target->GetInstanceScript())
            return instance->instance->IsHeroic() && instance->GetData(DATA_PRODIGY);
        return false;
    }
};

class npc_defias_watcher : public CreatureScript
{
public:
	npc_defias_watcher() : CreatureScript("npc_defias_watcher") { }

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npc_defias_watcherAI(creature);
	}

	struct npc_defias_watcherAI : public ScriptedAI
	{
		npc_defias_watcherAI(Creature* creature) : ScriptedAI(creature)
		{
			instance = creature->GetInstanceScript();
			Status = false;
		}

		InstanceScript* instance;
		bool Status;

		void Reset()
		{
			if (!me)
				return;

			me->SetPower(POWER_ENERGY, 100);
			me->SetMaxPower(POWER_ENERGY, 100);
			me->setPowerType(POWER_ENERGY);
			if (Status == true)
			{
				if (!me->HasAura(SPELL_ON_FIRE))
					me->AddAura(SPELL_ON_FIRE, me);
				me->setFaction(35);
			}
		}

		void EnterCombat(Unit* /*who*/) {}

		void JustDied(Unit* /*Killer*/)
		{
			if (!me || Status == true)
				return;

			Energizing();
		}

		void Energizing()
		{
			Status = true;
			me->SetHealth(15);
			me->setRegeneratingHealth(false);
			me->setFaction(35);
			me->AddAura(SPELL_ON_FIRE, me);
			me->CastSpell(me, SPELL_ON_FIRE);
			me->SetInCombatWithZone();

			if (Creature* reaper = me->FindNearestCreature(NPC_FOE_REAPER_5000, 200.0f))
			{
				me->CastSpell(reaper, SPELL_ENERGIZE);
			}
		}

		void DamageTaken(Unit* /*attacker*/, uint32& damage)
		{
			if (!me || damage <= 0 || Status == true)
				return;

			if (me->GetHealth() - damage <= me->GetMaxHealth()*0.10)
			{
				damage = 0;
				Energizing();
			}
		}

		void UpdateAI(uint32 const diff)
		{
			if (!UpdateVictim())
				return;

			DoMeleeAttackIfReady();
		}
	};
};


void AddSC_boss_foe_reaper_5000()
{
    new boss_foe_reaper_5000();
    new npc_scormies();
    new achievement_prototype_prodigy();
	new npc_defias_watcher();
}