modded class PS_GameModeCoop : SCR_BaseGameMode
{
	override void TryRespawn(RplId playableId, int playerId)
	{
		super.TryRespawn(playableId, playerId);
		TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
		if (mfe) mfe.PlayerUpdate(0, null);
	}
	
	override void OnGameStart()
	{
		super.OnGameStart();
		GetOnPlayerConnected().Insert(TILW_OnPlayerConnected);
	}
	
	protected void TILW_OnPlayerConnected(int playerId)
	{
		GetGame().GetCallqueue().CallLater(ShowJIPInfo, 15 * 1000, false, playerId);
	}
	
	protected void ShowJIPInfo(int playerId)
	{
		if (GetState() != SCR_EGameModeState.GAME || PS_PlayableManager.GetInstance().GetPlayableByPlayer(playerId) != RplId.Invalid()) return;
		
		SCR_PlayerController pc = SCR_PlayerController.TILW_GetPCFromPID(playerId);
		if (!pc) return;
		
		if (m_bTeamSwitch && !m_bRemoveRedundantUnits) pc.TILW_ShowJIPInfo();
		else pc.TILW_SendHintToPlayer("JIP Not Available", "This mission does not support join-in-progress, please wait until the next mission starts.", 10);
	}
}