/*
===========================================================================

Copyright (c) 2010-2015 Darkstar Dev Teams

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see http://www.gnu.org/licenses/

This file is part of DarkStar-server source code.

===========================================================================
*/

#include "ai_container.h"

#include "states/ability_state.h"
#include "states/attack_state.h"
#include "states/death_state.h"
#include "states/despawn_state.h"
#include "states/magic_state.h"
#include "states/mobskill_state.h"
#include "states/raise_state.h"
#include "states/trigger_state.h"
#include "states/weaponskill_state.h"
#include "states/range_state.h"
#include "controllers/player_controller.h"
#include "controllers/ai_controller.h"
#include "../entities/baseentity.h"
#include "../entities/battleentity.h"
#include "../entities/charentity.h"
#include "../entities/mobentity.h"
#include "../packets/entity_animation.h"

CAIContainer::CAIContainer(CBaseEntity* _PEntity) :
    CAIContainer(_PEntity, nullptr, nullptr, nullptr)
{
}

CAIContainer::CAIContainer(CBaseEntity* _PEntity, std::unique_ptr<CPathFind>&& _pathfind,
    std::unique_ptr<CController>&& _controller, std::unique_ptr<CTargetFind>&& _targetfind) :
    m_Tick(server_clock::now()),
    m_PrevTick(server_clock::now()),
    PEntity(_PEntity),
    ActionQueue(_PEntity),
    PathFind(std::move(_pathfind)),
    Controller(std::move(_controller)),
    TargetFind(std::move(_targetfind))
{
}

void CAIContainer::Cast(uint16 targid, uint16 spellid)
{
    if (Controller)
    {
        Controller->Cast(targid, spellid);
    }
    else
    {
        Internal_Cast(targid, spellid);
    }
}

void CAIContainer::Engage(uint16 targid)
{
    if (Controller)
    {
        Controller->Engage(targid);
    }
    else
    {
        Internal_Engage(targid);
    }
}

void CAIContainer::ChangeTarget(uint16 targid)
{
    if (Controller)
    {
        Controller->ChangeTarget(targid);
    }
    else
    {
        Internal_ChangeTarget(targid);
    }
}

void CAIContainer::Disengage()
{
    if (Controller)
    {
        Controller->Disengage();
    }
    else
    {
        Internal_Disengage();
    }
}

void CAIContainer::WeaponSkill(uint16 targid, uint16 wsid)
{
    if (Controller)
    {
        Controller->WeaponSkill(targid, wsid);
    }
    else
    {
        Internal_WeaponSkill(targid, wsid);
    }
}

void CAIContainer::MobSkill(uint16 targid, uint16 wsid)
{
    auto AIController = dynamic_cast<CAIController*>(Controller.get());
    if (AIController)
    {
        AIController->MobSkill(targid, wsid);
    }
    else
    {
        Internal_MobSkill(targid, wsid);
    }
}

void CAIContainer::Ability(uint16 targid, uint16 abilityid)
{
    auto PlayerController = dynamic_cast<CPlayerController*>(Controller.get());
    if (PlayerController)
    {
        PlayerController->Ability(targid, abilityid);
    }
    else
    {
        Internal_Ability(targid, abilityid);
    }
}

void CAIContainer::RangedAttack(uint16 targid)
{
    auto PlayerController = dynamic_cast<CPlayerController*>(Controller.get());
    if (PlayerController)
    {
        PlayerController->RangedAttack(targid);
    }
    else
    {
        Internal_RangedAttack(targid);
    }
}

void CAIContainer::Trigger(uint16 targID)
{
    if (CanChangeState())
    {
        ChangeState<CTriggerState>(PEntity, targID);
        if (PathFind)
        {
            PathFind->Clear(); //#TODO: pause/resume after?
        }
    }
}

bool CAIContainer::Internal_Engage(uint16 targetid)
{
    //#TODO: pet engage/disengage
    auto PTarget {dynamic_cast<CBattleEntity*>(PEntity->GetEntity(targetid))};
    auto entity {dynamic_cast<CBattleEntity*>(PEntity)};

    //#TODO: use valid target stuff from spell
    if (entity && !entity->GetBattleTargetID() && PTarget && !PTarget->isDead())
    {
        //#TODO: remove m_battleTarget if possible (need to check disengage)
        entity->SetBattleTargetID(targetid);
        if (CanChangeState() || (GetCurrentState() && GetCurrentState()->IsCompleted()))
        {
            ForceChangeState<CAttackState>(entity, targetid);
        }
        return true;
    }
    return false;
}

bool CAIContainer::Internal_Cast(uint16 targetid, uint16 spellid)
{
    auto entity {dynamic_cast<CBattleEntity*>(PEntity)};
    if (entity)
        return ChangeState<CMagicState>(entity, targetid, spellid);
    return false;
}

void CAIContainer::Internal_ChangeTarget(uint16 targetid)
{
    auto entity {dynamic_cast<CBattleEntity*>(PEntity)};
    entity->SetBattleTargetID(targetid);
}

void CAIContainer::Internal_Disengage()
{
    auto entity {dynamic_cast<CBattleEntity*>(PEntity)};
    entity->SetBattleTargetID(0);
}

bool CAIContainer::Internal_WeaponSkill(uint16 targid, uint16 wsid)
{
    auto entity {dynamic_cast<CBattleEntity*>(PEntity)};
    if (entity)
        return ChangeState<CWeaponSkillState>(entity, targid, wsid);
    return false;
}

bool CAIContainer::Internal_MobSkill(uint16 targid, uint16 wsid)
{
    auto entity {dynamic_cast<CMobEntity*>(PEntity)};
    if (entity)
        return ChangeState<CMobSkillState>(entity, targid, wsid);
    return false;
}

bool CAIContainer::Internal_Ability(uint16 targetid, uint16 abilityid)
{
    auto entity {dynamic_cast<CBattleEntity*>(PEntity)};
    if (entity)
        return ChangeState<CAbilityState>(static_cast<CCharEntity*>(PEntity), targetid, abilityid);
    return false;
}

bool CAIContainer::Internal_RangedAttack(uint16 targetid)
{
    auto entity {dynamic_cast<CBattleEntity*>(PEntity)};
    if (entity)
        return ChangeState<CRangeState>(static_cast<CCharEntity*>(PEntity), targetid);
    return false;
}

void CAIContainer::Internal_Die(duration deathTime)
{
    auto entity {dynamic_cast<CBattleEntity*>(PEntity)};
    if (entity)
        ChangeState<CDeathState>(entity, deathTime);
}

CState* CAIContainer::GetCurrentState()
{
    if (!m_stateStack.empty())
    {
        return m_stateStack.top().get();
    }
    return nullptr;
}

bool CAIContainer::CanChangeState()
{
    return !GetCurrentState() || GetCurrentState()->CanChangeState();
}

CController* CAIContainer::GetController()
{
    return Controller.get();
}

void CAIContainer::Internal_Raise()
{
    auto entity {dynamic_cast<CBattleEntity*>(PEntity)};
    if (entity)
        ForceChangeState<CRaiseState>(entity);
}

void CAIContainer::Reset()
{
    if (PathFind)
    {
        PathFind->Clear();
    }
}

void CAIContainer::Tick(time_point _tick)
{
    m_PrevTick = m_Tick;
    m_Tick = _tick;
    CBaseEntity* PreEntity = PEntity;
    
    //#TODO: check this in the controller instead maybe? (might not want to check every tick) - same for pathfind
    ActionQueue.checkAction(_tick);

    // check pathfinding
    if (PathFind && (!GetCurrentState() || (GetCurrentState()->CanFollowPath())))
    {
        PathFind->FollowPath();
    }

    if (Controller && Controller->canUpdate)
    {
        Controller->Tick(_tick);
    }

    while (!m_stateStack.empty() && m_stateStack.top()->DoUpdate(_tick))
    {
        m_stateStack.top()->Cleanup(_tick);
        m_stateStack.pop();
    }

    //make sure this AI hasn't been replaced by another
    if (PreEntity->updatemask && PreEntity->PAI.get() == this)
    {
        PreEntity->UpdateEntity();
    }
}

bool CAIContainer::IsStateStackEmpty()
{
    return m_stateStack.empty();
}

void CAIContainer::ClearStateStack()
{
    while (!m_stateStack.empty())
    {
        m_stateStack.pop();
    }
}

bool CAIContainer::IsSpawned()
{
    return PEntity->status != STATUS_DISAPPEAR;
}

bool CAIContainer::IsRoaming()
{
    return PEntity->animation == ANIMATION_NONE;
}

bool CAIContainer::IsEngaged()
{
    return PEntity->animation == ANIMATION_ATTACK;
}

time_point CAIContainer::getTick()
{
    return m_Tick;
}

time_point CAIContainer::getPrevTick()
{
    return m_PrevTick;
}

void CAIContainer::Despawn()
{
    if (Controller)
    {
        Controller->Despawn();
    }
    else
    {
        Internal_Despawn(0s);
    }
}

void CAIContainer::QueueAction(queueAction_t&& action)
{
    ActionQueue.pushAction(std::move(action));
}

void CAIContainer::Internal_Despawn(duration spawnTime)
{
    ForceChangeState<CDespawnState>(PEntity, spawnTime);
}

void CAIContainer::CheckCompletedStates()
{
    while (!m_stateStack.empty() && m_stateStack.top()->IsCompleted())
    {
        m_stateStack.pop();
    }
}