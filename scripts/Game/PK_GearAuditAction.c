// Custom User Action to trigger the Gear Audit from within the game.
// Attach this to any object (like a crate or sign) in the world editor.

class PK_GearAuditAction : ScriptedUserAction
{
	// This text appears in the interaction menu
	override bool GetActionNameScript(out string outName)
	{
		outName = "Run Gear Audit (All Characters)";
		return true;
	}

	// Tell the engine this action only has a local effect (diagnostic)
	// Note: This often helps with performance but still requires RplComponent on the object.
	override bool HasLocalEffectOnly()
	{
		return true;
	}

	// This is what happens when you press 'F' on the object
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		// Trigger our global scanning logic
		PK_GearCheck_InGame.ScanAllCharacters();
		
		// Provide immediate feedback to the player
		SCR_HintManagerComponent.ShowCustomHint("Gear Audit started! Check the console (F8) for results.", "GEAR AUDIT", 5.0);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return true;
	}
}