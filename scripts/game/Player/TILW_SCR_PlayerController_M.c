modded class SCR_PlayerController : PlayerController
{
	
	static SCR_PlayerController TILW_GetPCFromPID(int playerId)
	{
		return SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
	}
	
	void TILW_SendHintToPlayer(string hl, string msg, int dur)
	{
		Rpc(TILW_RpcDo_ShowHint, hl, msg, dur);
	}
	
	void TILW_ShowJIPInfo()
	{
		Rpc(TILW_RpcDo_ShowJIPInfo);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void TILW_RpcDo_ShowJIPInfo()
	{
		if (GetGame().GetInputManager().IsUsingMouseAndKeyboard()) TILW_ShowHint("You can join-in-progress by holding U, choosing a slot and clicking 'Ready' in the bottom right.", "JIP Available", 10);
		else TILW_ShowHint("You can join-in-progress by holding View, choosing a slot and selecting 'Ready' in the bottom right.", "JIP Available", 10);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void TILW_RpcDo_ShowHint(string hl, string msg, int dur)
	{
		TILW_ShowHint(msg, hl, dur);
	}
	
	protected bool TILW_ShowHint(string description, string name = string.Empty, float duration = 0, bool isSilent = false, EHint type = EHint.UNDEFINED, EFieldManualEntryId fieldManualEntry = EFieldManualEntryId.NONE, bool isTimerVisible = false, bool ignoreShown = true)
	{
		SCR_HintUIInfo customHint = SCR_HintUIInfo.CreateInfo(description, name, duration, type, fieldManualEntry, isTimerVisible);
		SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
		if (hintManager) return hintManager.Show(customHint, isSilent, ignoreShown);
		return false;
	}
	
}
