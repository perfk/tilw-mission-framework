modded class Vehicle 
{
	override void RegisterToMissionDate()
	{
		// Patch to prevent VME in non RL scenarios, delete when RL 1.0.51 is no longer the newest version
		PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gm)
			super.RegisterToMissionDate();
	}
}