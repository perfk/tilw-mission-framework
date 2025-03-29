[EntityEditorProps(category: "GameScripted/Triggers", description: "The TILW_PresenceTriggerEntity is a custom trigger designed to be used with the TILW_MissionFrameworkEntity. \nIt checks if there are more/less then m_countThreshold matching entities in the trigger.")]
class TILW_PresenceTriggerEntityClass : TILW_BaseTriggerEntityClass
{
}

class TILW_PresenceTriggerEntity : TILW_BaseTriggerEntity
{

	// FILTER SETTINGS

	[Attribute("", UIWidgets.Auto, "If defined, only entities which have one of these names are counted.", category: "Trigger Filter")]
	protected ref array<string> m_nameFilter;

	[Attribute("0", UIWidgets.Auto, "If false, prevent dead or destroyed entities from being counted.", category: "Trigger Filter")]
	protected bool m_allowDestroyed;

	// CONDITION SETTINGS

	[Attribute("1", UIWidgets.Auto, "How high/low the number of present entities has to be", category: "Trigger Condition", params: "0 inf 1")]
	protected int m_countThreshold;


	// TRIGGER LOGIC

	override bool EvaluateCondition()
	{
		if (m_comparisonMode == TILW_EComparisonMode.EQUAL_OR_LESS)
			return (m_totalCount <= m_countThreshold);
		else
			return (m_totalCount >= m_countThreshold);
	}


	// HELPER METHODS

	override bool AddEntity(IEntity e)
	{
		m_totalCount += 1;
		return true;
	}

	override bool FilterEntity(IEntity e)
	{
		if (!super.FilterEntity(e))
			return false;

		if (!m_nameFilter.IsEmpty() && !m_nameFilter.Contains(GetName()))
			return false;

		if (!m_allowDestroyed && IsEntityDestroyed(e))
			return false;

		return true;
	}

	override bool IsMatchingClass(IEntity e)
	{
		return true;
	}

	// Maybe get entity name for display

}
