[EntityEditorProps(category: "game/", description: "Regenerates navmesh around entity + children after initialization.")]
class TILW_NavmeshRegenComponentClass: ScriptComponentClass
{
}

class TILW_NavmeshRegenComponent: ScriptComponent
{
	void TILW_NavmeshRegenComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.INIT);
	}

	protected override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		GetGame().GetCallqueue().Call(RegenEntity, owner);
	}
	
	protected static void RegenEntity(IEntity owner)
	{
		AIWorld aiw = GetGame().GetAIWorld();
		if (!aiw)
			return;
		SCR_AIWorld scr_aiw = SCR_AIWorld.Cast(aiw);
		if (!scr_aiw)
			return;
		scr_aiw.RequestNavmeshRebuildEntity(owner);
	}
	
	protected static void RegenVolume(VolumeEntity ve)
	{	
		array<vector> points3D = new array<vector>();
		array<float> points2D = new array<float>();
		
		AIWorld aiw = GetGame().GetAIWorld();
		if (!aiw)
			return;
		SCR_AIWorld scr_aiw = SCR_AIWorld.Cast(aiw);
		if (!scr_aiw)
			return;
	}
}