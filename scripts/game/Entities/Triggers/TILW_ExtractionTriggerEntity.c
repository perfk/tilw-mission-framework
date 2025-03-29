[EntityEditorProps(category: "GameScripted/Triggers", description: "The ExtractionTriggerEntity is a custom trigger designed to be used with the TILW_MissionFrameworkEntity. \nIt compares the number of local faction players to the number of global alive faction players.")]
class TILW_ExtractionTriggerEntityClass : TILW_BaseTriggerEntityClass
{
}

class TILW_ExtractionTriggerEntity : TILW_BaseTriggerEntity
{

	// FILTER SETTINGS

	// CONDITION SETTINGS

	[Attribute("", UIWidgets.Auto, "Which faction is being examined?", category: "Trigger Condition")]
	protected string m_factionKey;

	[Attribute("1", UIWidgets.Auto, "How high/low the ratio of local to global faction players needs to be.\nFor example, if the ratio is 0.9, 90% of all the factions alive players need to be within the radius for the flag to be set.", category: "Trigger Condition", params: "0 1 0.01")]
	protected float m_ratioThreshold;


	// TRIGGER LOGIC

	override void RunQuery()
	{
		m_totalCount = 0;
		m_specialCount = 0;

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		foreach (int playerId : playerIds)
		{
			IEntity controlled = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!controlled)
				continue;
			SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(controlled);
			if (!cc)
				continue;
			if (!SCR_AIDamageHandling.IsAlive(controlled))
				continue;

			Faction f = factionManager.GetPlayerFaction(playerId);
			if (!f)
				continue;
			string fkey = f.GetFactionKey();
			if (fkey != m_factionKey)
				continue;

			if (!IsMatchingPrefab(controlled))
				continue;

			m_totalCount += 1;
			if (vector.Distance(controlled.GetOrigin(), GetOrigin()) <= m_queryRadius)
				m_specialCount += 1;

		}
	}

	override bool EvaluateCondition()
	{
		if (m_totalCount == 0)
			return !m_comparisonMode;
		float ratio = m_specialCount / m_totalCount;
		if (!m_comparisonMode)
			return (ratio <= m_ratioThreshold);
		else
			return (m_comparisonMode && ratio >= m_ratioThreshold);
	}


	// HELPER METHODS

	override string GetStatusMessage(int status)
	{
		string factionName = GetGame().GetFactionManager().GetFactionByKey(m_factionKey).GetFactionName();
		return string.Format(super.GetStatusMessage(status), factionName);
	}
}
