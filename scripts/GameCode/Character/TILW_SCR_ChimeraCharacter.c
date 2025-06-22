modded class SCR_ChimeraCharacter : ChimeraCharacter
{
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!GetGame().InPlayMode() && !Replication.IsServer())
			return;
		TILW_TriggerSystem ts = TILW_TriggerSystem.GetInstance();
		if (ts)
			ts.InsertCharacter(this);
	}

	void ~SCR_ChimeraCharacter()
	{
		if (!GetGame().InPlayMode() && !Replication.IsServer())
			return;
		TILW_TriggerSystem ts = TILW_TriggerSystem.GetInstance();
		if (ts)
			ts.RemoveCharacter(this);
	}
}