modded class SCR_AICombatComponent : ScriptComponent
{
	bool m_neverDismountTurret = false;

	override bool DismountTurretCondition(inout vector targetPos, bool targetPosProvided)
	{
		if (m_neverDismountTurret)
			return false;
		return super.DismountTurretCondition(targetPos, targetPosProvided);
	}
}
