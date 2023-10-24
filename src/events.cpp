/**
 * =============================================================================
 * CS2Fixes
 * Copyright (C) 2023 Source2ZE
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"
#include "KeyValues.h"
#include "commands.h"
#include "ctimer.h"
#include "eventlistener.h"
#include "entity/cbaseplayercontroller.h"
#include "adminsystem.h"


#include "tier0/memdbgon.h"
#include "playermanager.h"

extern IGameEventManager2 *g_gameEventManager;
extern IServerGameClients *g_pSource2GameClients;
extern CEntitySystem *g_pEntitySystem;

CUtlVector<CGameEventListener *> g_vecEventListeners;

void RegisterEventListeners()
{
	if (!g_gameEventManager)
		return;

	FOR_EACH_VEC(g_vecEventListeners, i)
	{
		g_gameEventManager->AddListener(g_vecEventListeners[i], g_vecEventListeners[i]->GetEventName(), true);
	}
}

void UnregisterEventListeners()
{
	if (!g_gameEventManager)
		return;

	FOR_EACH_VEC(g_vecEventListeners, i)
	{
		g_gameEventManager->RemoveListener(g_vecEventListeners[i]);
	}

	g_vecEventListeners.Purge();
}




bool g_bBlockTeamMessages = true;

CON_COMMAND_F(c_toggle_team_messages, "toggle team messages", FCVAR_SPONLY | FCVAR_LINKED_CONCOMMAND)
{
	g_bBlockTeamMessages = !g_bBlockTeamMessages;
}

GAME_EVENT_F(player_team)
{
	// Remove chat message for team changes
	if (g_bBlockTeamMessages)
		pEvent->SetBool("silent", true);
}

GAME_EVENT_F(player_hurt)
{
	CBasePlayerController *pController = (CBasePlayerController*)pEvent->GetPlayerController("userid");
    ZEPlayer* pPlayer = g_playerManager->GetPlayer(pController->GetPlayerSlot());

	CBasePlayerController* died = (CBasePlayerController*)pEvent->GetPlayerController("userid");
	CBasePlayerController* killer = (CBasePlayerController*)pEvent->GetPlayerController("attacker");
	uint16 health = pEvent->GetInt("dmg_health");

	if (pPlayer->IsAdminFlagSet(ADMFLAG_CONVARS))
	{
		ClientPrint(killer, HUD_PRINTCENTER, "-\4%d ", health);
	}

}

void SetClanTag(CBasePlayerController *player, const char *tag)
{
	addresses::SetClanTag(player, tag);
}

GAME_EVENT_F(player_spawn)
{
	CBasePlayerController *pController = (CBasePlayerController*)pEvent->GetPlayerController("userid");

	if (!pController)
		return;

	CEntityHandle hController = pController->GetHandle();

	// Gotta do this on the next frame...
	new CTimer(0.0f, false, false, [hController]()
	{
		CBasePlayerController *pController = (CBasePlayerController*)Z_CBaseEntity::EntityFromHandle(hController);

		if (!pController)
			return;

		int iPlayer = pController->GetPlayerSlot();
		ZEPlayer* pZEPlayer = g_playerManager->GetPlayer(iPlayer);

		if (pZEPlayer)
		{
			pZEPlayer->SetUsedMedkit(false);
		}

		CBasePlayerPawn *pPawn = pController->GetPawn();

		// Just in case somehow there's health but the player is, say, an observer
		if (!pPawn || pPawn->m_iHealth() <= 0 || !g_pSource2GameClients->IsPlayerAlive(pController->GetPlayerSlot()))
			return;

		pPawn->m_pCollision->m_collisionAttribute().m_nCollisionGroup = COLLISION_GROUP_DEBRIS;
		pPawn->m_pCollision->m_CollisionGroup = COLLISION_GROUP_DEBRIS;
		pPawn->CollisionRulesChanged();

		SetClanTag(player, "[TEST TAG]");

	});

	Message("EVENT FIRED: %s %s\n", pEvent->GetName(), pController->GetPlayerName());
}

GAME_EVENT_F(player_death)
{

	CBasePlayerController *pController = (CBasePlayerController*)pEvent->GetPlayerController("userid");
	CBasePlayerController *pAttacker = (CBasePlayerController*)pEvent->GetPlayerController("attacker");
	float distance = pEvent->GetFloat("distance");

	if (!pController || !pAttacker)
		return;

	ClientPrint(pController, HUD_PRINTTALK, CHAT_PREFIX"You were killed by \4%s \1from \2%.1fm \1away.", pAttacker->GetPlayerName(), distance);
	ClientPrint(pAttacker, HUD_PRINTTALK, CHAT_PREFIX"You killed \4%s \1from \4%.1fm \1away.", pController->GetPlayerName(), distance);
}
