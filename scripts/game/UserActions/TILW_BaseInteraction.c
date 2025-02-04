class TILW_BaseInteraction : ScriptedUserAction
{
	[Attribute( uiwidget: UIWidgets.Auto, defvalue: "Interact: Entity", desc: "Action Name")]
	protected string m_actionName;
	
	[Attribute( uiwidget: UIWidgets.Auto, desc: "Allowed user faction keys. If empty, any.")]
	protected ref array<string> m_factionKeys;
	
	[Attribute("1", uiwidget: UIWidgets.Auto, desc: "Should the interaction only work once, and disappear afterwards?")]
	protected bool m_singleUse;
	
	[Attribute("0", uiwidget: UIWidgets.Auto, desc: "Should the interaction remove this entities parent entity (together with any children)?")]
	protected bool m_deleteParent;
	
	[Attribute("", uiwidget: UIWidgets.Auto, desc: "If defined, name of mission flag to set after interaction")]
	protected string m_flagName;
	
	protected bool m_completed = false;
	
	//------------------------------------------------------------------------------------------------
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
		if (m_singleUse)
			m_completed = true;
		
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		if (fw)
			fw.AdjustMissionFlag(m_flagName, true);
		
		if (m_deleteParent && pOwnerEntity.GetParent())
			SCR_EntityHelper.DeleteEntityAndChildren(pOwnerEntity.GetParent());
    }
	//------------------------------------------------------------------------------------------------
    override bool GetActionNameScript(out string outName)
    {
        outName = (m_actionName);
        return true;
    }
    //------------------------------------------------------------------------------------------------
    override bool CanBePerformedScript(IEntity user)
    {
		return !m_completed;
    }
    //------------------------------------------------------------------------------------------------
    override bool CanBeShownScript(IEntity user)
    {
		if (!m_factionKeys.IsEmpty()) {
			SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(user);
			if (!cc || !m_factionKeys.Contains(cc.GetFactionKey()))
				return false;
		}
		return !m_completed;
    }
	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
	    return false;
	}
	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
	    return true;
	}
	
};