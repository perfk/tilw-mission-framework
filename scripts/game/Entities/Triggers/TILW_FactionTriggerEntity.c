[EntityEditorProps(category: "GameScripted/Triggers", description: "The TILW_FactionTriggerEntity is a custom trigger designed to be used with the TILW_MissionFrameworkEntity. \nIt compares the number of a factions characters to the total amount of characters.")]
class TILW_FactionTriggerEntityClass: TILW_BaseTriggerEntityClass
{
}

class TILW_FactionTriggerEntity : TILW_BaseTriggerEntity
{
	
	// FILTER SETTINGS
	
	[Attribute("0", UIWidgets.Auto, "If true, only the ratio between players should matter. AI characters are ignored completely.", category: "Trigger Filter")]
	protected bool m_playersOnly;
	
	[Attribute("1", UIWidgets.Auto, "If true, exclude playable characters that are not currently slotted.", category: "Trigger Filter")]
	protected bool m_excludeUnusedPlayables;
	
	[Attribute("", UIWidgets.ResourceAssignArray, "Any of these faction keys will be completely ignored by the trigger.", category: "Trigger Filter")]
	protected ref array<string> m_ignoredFactions;
	
	// CONDITION SETTINGS
	
	[Attribute("", UIWidgets.Auto, "Key of the owner faction", category: "Trigger Condition")]
	protected string m_ownerFactionKey;
	
	[Attribute("1", UIWidgets.Auto, "How high/low the factions character share has to be", category: "Trigger Condition", params: "0 1 0.01")]
	protected float m_ratioThreshold;
	
	
	// TRIGGER LOGIC
	
	override bool EvaluateCondition()
	{
		if (m_totalCount == 0) return !m_comparisonMode;
		float ratio = m_specialCount / m_totalCount;
		if (!m_comparisonMode) return (ratio <= m_ratioThreshold);
		else return (m_comparisonMode && ratio >= m_ratioThreshold);
	}
	
	
	// HELPER METHODS
	
	override bool AddEntity(IEntity e)
	{
		SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(e);
		Faction f = cc.m_pFactionComponent.GetAffiliatedFaction();
		if (!f) return true;
		string fKey = f.GetFactionKey();
		if (m_ignoredFactions.Contains(fKey)) return true;
		m_totalCount += 1;
		if (fKey == m_ownerFactionKey) m_specialCount += 1;
		return true;
	}
	
	override bool FilterEntity(IEntity e)
	{
		if (!super.FilterEntity(e)) return false;
		
		if (IsEntityDestroyed(e)) return false;
		
		if (!EntityUtils.IsPlayer(e)) {
			if (m_playersOnly) return false; // Only players allowed
			Managed m = e.FindComponent(PS_PlayableComponent);
			if (m_excludeUnusedPlayables && m) {
				PS_PlayableComponent pc = PS_PlayableComponent.Cast(m);
				if (pc.GetPlayable()) return false; // Not slotted
				
			}
		}
		
		return true;
	}
	
	override bool IsMatchingClass(IEntity e)
	{
		SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(e);
		return (cc);
	}
	
	override string GetStatusMessage(int status)
	{
		string factionName = GetGame().GetFactionManager().GetFactionByKey(m_ownerFactionKey).GetFactionName();
		return string.Format(super.GetStatusMessage(status), factionName);
	}
}