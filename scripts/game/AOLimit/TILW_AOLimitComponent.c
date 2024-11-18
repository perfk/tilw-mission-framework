[ComponentEditorProps(category: "GameScripted/", description: "AO Limit component")]
class TILW_AOLimitComponentClass: ScriptComponentClass
{
}


class TILW_AOLimitComponent : ScriptComponent
{
	
	[Attribute("5", UIWidgets.Auto, "How many seconds pass between checking if players are still in AO", params: "0.25 inf 0.25")]
	protected float m_checkFrequency;
	
	[Attribute("", UIWidgets.Auto, desc: "Factions affected by the AO limit (if empty, all factions)")]
	protected ref array<string> m_factionKeys;
	
	//[Attribute("0", UIWidgets.Auto, desc: "If greater than 0, the AO limit instead becomes a setup zone, which  expires after the given amount of seconds.")]
	//protected int m_setupTimer;
	
	protected ref array<float> m_points2D = new array<float>();
	protected ref array<vector> m_points3D = new array<vector>();
	
	override void OnPostInit(IEntity owner)
	{
		if (Replication.IsServer()) GetGame().GetCallqueue().Call(InitLoop, owner);
		
	}
	
	// QUERY
	
	protected void InitLoop(IEntity owner)
	{
		PolylineShapeEntity pse = PolylineShapeEntity.Cast(owner);
		if (!pse) {
			Print("TILW_AOLimitComponent | Owner entity (" + owner + ") is not a polyline!", LogLevel.ERROR);
			return;
		}
		if (pse.GetPointCount() < 3) {
			Print("TILW_AOLimitComponent | Owner entity (" + owner + ") does not have enough points!", LogLevel.ERROR);
			return;
		}
		pse.GetPointsPositions(m_points3D);
		for (int i = 0; i < m_points3D.Count(); i++) m_points3D[i] = pse.CoordToParent(m_points3D[i]);
		SCR_Math2D.Get2DPolygon(m_points3D, m_points2D);
		
		QueryLoop();
	}
	
	protected void QueryLoop()
	{
		RunQuery();
		GetGame().GetCallqueue().CallLater(QueryLoop, m_checkFrequency * 1000, false);
	}
	
	protected void RunQuery()
	{
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gm.GetState() != SCR_EGameModeState.GAME) return;
		
		array<int> players = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			IEntity pce = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!pce) continue;
			if (!SCR_AIDamageHandling.IsAlive(pce)) continue;
			SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(pce);
			if (!cc) continue; // not going to work since RL has an "initial entity"
			if (!cc.m_pFactionComponent) continue;
			if (!m_factionKeys.IsEmpty()) {
				Faction f = cc.m_pFactionComponent.GetAffiliatedFaction();
				if (!f) continue;
				if (!m_factionKeys.Contains(f.GetFactionKey())) continue;
			}
			bool inPolygon = Math2D.IsPointInPolygon(m_points2D, pce.GetOrigin()[0], pce.GetOrigin()[2]);
			if (!inPolygon) PlayerOutsidePolygon(playerId);
		}
	}
	
	protected void PlayerOutsidePolygon(int playerId)
	{
		SCR_PlayerController pc = SCR_PlayerController.TILW_GetPCFromPID(playerId);
		if (!pc) return;
		pc.TILW_SendHintToPlayer("AO Limit reached", "You have passed your AO limit, please return.", 5);
	}
	
	
	/*
	
	// DRAWING
	
	[Attribute("1", UIWidgets.Auto, "Draw AO limit on map")]
	protected bool m_drawLimit;
	
	[Attribute("1", UIWidgets.Auto, "Draw AO limit for everyone, instead of just for the affected factions")]
	protected bool m_showToEveryone;
	
	// color
	
	protected CanvasWidget m_wCanvasWidget;
	protected ref array<ref CanvasWidgetCommand> m_aCanvasCommands;
	
	protected void InitDrawing()
	{
		// make sure it's not on dedicated server
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity) return;
		mapEntity.GetOnMapOpen().Insert(CreateWidget);
		mapEntity.GetOnMapClose().Insert(DeleteWidget);
	}
	
	protected void CreateWidget(MapConfiguration config)
	{
		SetEventMask(GetOwner(), EntityEvent.POSTFRAME);
	}
	protected void DeleteWidget(MapConfiguration config)
	{
		ClearEventMask(GetOwner(), EntityEvent.POSTFRAME);
	}
	
	protected CanvasWidgetCommand CreateLineDrawCommand()
	{
		LineDrawCommand ldc = new LineDrawCommand();
		return ldc;
	}
	
	protected void DrawVertices()
	{
		m_aCanvasCommands = {};
		
		m_aCanvasCommands.Insert(CreateLineDrawCommand());
		
		m_wCanvasWidget.SetDrawCommands(m_aCanvasCommands);
	}
	
	protected bool IsVisible()
	{
		if (!m_drawLimit) return false;
		if (!m_showToEveryone) {
			Faction f = SCR_PlayerController.GetLocalControlledEntityFaction();
			if (!f) return false;
			if (!m_factionKeys.Contains(f.GetFactionKey())) return false;
		}
		return true;
	}
	
	**/
	
}