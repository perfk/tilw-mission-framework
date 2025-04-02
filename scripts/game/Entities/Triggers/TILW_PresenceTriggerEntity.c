[EntityEditorProps(category: "GameScripted/Triggers", description: "The TILW_PresenceTriggerEntity is a custom trigger designed to be used with the TILW_MissionFrameworkEntity. \nIt checks if there are more/less then m_countThreshold matching entities in the trigger.")]
class TILW_PresenceTriggerEntityClass : TILW_BaseTriggerEntityClass
{
}

class TILW_PresenceTriggerEntity : TILW_BaseTriggerEntity
{

	// FILTER SETTINGS

	[Attribute("", UIWidgets.Auto, "Names of the entities to look for.", category: "Trigger Filter")]
	protected ref array<string> m_entityNames;

	[Attribute("0", UIWidgets.Auto, "If false, prevent dead or destroyed entities from being counted.", category: "Trigger Filter")]
	protected bool m_allowDestroyed;

	// CONDITION SETTINGS

	[Attribute("1", UIWidgets.Auto, "What percentage of the specified items has to be present.", category: "Trigger Condition", params: "0 inf 1")]
	protected float m_ratioThreshold;
	
	[Attribute("0", UIWidgets.Auto, "Detete (collect) entities after they have contributed to the condition being met.", category: "Trigger Filter")]
	protected bool m_deleteEntities;


	// TRIGGER LOGIC

	override bool EvaluateCondition()
	{
		if (m_totalCount == 0)
			return !m_comparisonMode;
		
		float ratio = m_specialCount / m_totalCount;
		
		if (!m_comparisonMode)
			return (ratio <= m_ratioThreshold);
		else
			return (ratio >= m_ratioThreshold);
	}
	
	override void RunQuery()
	{
		if (m_totalCount == 0)
			m_totalCount = m_entityNames.Count();
		
		if (!m_deleteEntities)
			m_specialCount = 0;
		
		foreach (string name : m_entityNames)
		{
			IEntity e = GetGame().GetWorld().FindEntityByName(name);
			if (!e)
				continue;
			float distance = vector.Distance(GetOrigin(), e.GetOrigin());
			if (distance > m_queryRadius)
				continue;
			if (!m_allowDestroyed && IsEntityDestroyed(e))
				continue;
			
			m_specialCount += 1;
			
			if (m_deleteEntities)
			{
				SCR_EntityHelper.DeleteEntityAndChildren(e);
				m_entityNames.RemoveItem(name);
			}
		}
	}

	// Maybe get entity name for display

}
