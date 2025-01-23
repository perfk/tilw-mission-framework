[BaseContainerProps()]
class TILW_ManualJIPAvailable : SCR_AvailableActionCondition
{
	override bool IsAvailable(SCR_AvailableActionsConditionData data)
	{		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if(!playerController)
			return false;
		
		if(playerController.m_isJIPAvailable && playerController.CanJIP())
			return true;
		
		return false;
	}
}