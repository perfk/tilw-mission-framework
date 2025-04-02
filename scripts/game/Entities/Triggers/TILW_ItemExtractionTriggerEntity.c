[EntityEditorProps(category: "GameScripted/Triggers", description: "The ExtractionTriggerEntity is a custom trigger designed to be used with the TILW_MissionFrameworkEntity. \nIt Checks if an item with the specifide entity name is in a valid players inventory within the trigger radius")]
class TILW_ItemExtractionTriggerEntityClass : TILW_BaseTriggerEntityClass
{
}

class TILW_ItemNamePredicate : InventorySearchPredicate
{
	private string m_name;
					
	void TILW_ItemNamePredicate(string s_name)					
	{						
		m_name = s_name;				
	}

					
	override protected bool IsMatch(BaseInventoryStorageComponent storage, IEntity item, array<GenericComponent> queriedComponents, array<BaseItemAttributeData> queriedAttributes)				
	{					
		return m_name == item.GetName();				
	}
	
}

class TILW_ItemExtractionTriggerEntity : TILW_BaseTriggerEntity
{

	// FILTER SETTINGS

	// CONDITION SETTINGS
	
	[Attribute("", UIWidgets.Auto, "Which faction is being examined?", category: "Trigger Condition")]
	protected string m_factionKey;

	[Attribute("", UIWidgets.Auto, "Which Item is being examined?", category: "Trigger Condition")]
	protected string m_item;

	//[Attribute("", UIWidgets.Auto, "Item Target Entity", category: "Trigger Condition")]
	//protected string m_itemTargetEntity;


	//Predicate for finding a named item in inventory
	protected bool b_itemPresent = false;


	// TRIGGER LOGIC

	override void RunQuery()
	{

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
			
			Print(vector.Distance(controlled.GetOrigin(), this.GetOrigin()));
			if (vector.Distance(controlled.GetOrigin(), this.GetOrigin()) > m_queryRadius)
				continue;
			

			InventoryStorageManagerComponent invManager = InventoryStorageManagerComponent.Cast(controlled.FindComponent(InventoryStorageManagerComponent));;
			IEntity item = invManager.FindItem(new TILW_ItemNamePredicate(m_item), EStoragePurpose.PURPOSE_DEPOSIT);
			
			if(item != null)
				b_itemPresent = true;
			else
				b_itemPresent = false;

		}
	}

	override bool EvaluateCondition()
	{
			return b_itemPresent;
	}


	// HELPER METHODS

	override string GetStatusMessage(int status)
	{
		string factionName = GetGame().GetFactionManager().GetFactionByKey(m_factionKey).GetFactionName();
		return string.Format(super.GetStatusMessage(status), factionName);
	}
}
