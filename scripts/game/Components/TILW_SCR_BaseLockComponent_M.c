modded class SCR_BaseLockComponent : ScriptComponent
{
	
	// DOES NOT WORK IN MP BECAUSE OF RPL SYSTEM FAILURE WHEN ADDING RPLPROP
	
	[Attribute("0", UIWidgets.Auto, desc: "Lock Driver compartments", category: "Lock")]
	protected bool m_lockPilot;
	[Attribute("0", UIWidgets.Auto, desc: "Lock Gunner compartments\nWarning, this will not affect separate entities with their own lock components, like the BTR turret.", category: "Lock")]
	protected bool m_lockTurret;
	[Attribute("0", UIWidgets.Auto, desc: "Lock Passenger compartments", category: "Lock")]
	protected bool m_lockCargo;
	
	[Attribute("", UIWidgets.ResourceAssignArray, "If defined, only the specified factions are allowed to access the locked compartments.", category: "Lock")]
	protected ref array<string> m_factionLock;
	
	override bool IsLocked(IEntity user, BaseCompartmentSlot compartmentSlot)
	{
		if (m_factionLock && !m_factionLock.IsEmpty())
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(user);
			if (!character || !character.m_pFactionComponent)
				return true;
			if (m_factionLock.Contains(character.m_pFactionComponent.GetAffiliatedFactionKey()))
				return super.IsLocked(user, compartmentSlot);
		}
		return IsCompartmentLocked(compartmentSlot) || super.IsLocked(user, compartmentSlot);
	}
	
	protected bool IsCompartmentLocked(BaseCompartmentSlot compartmentSlot)
	{
		switch(compartmentSlot.GetType())
		{
			case ECompartmentType.PILOT:
				return m_lockPilot;
			case ECompartmentType.TURRET:
				return m_lockTurret;
			case ECompartmentType.CARGO:
				return m_lockCargo;
		}
		return false;
	}
}
