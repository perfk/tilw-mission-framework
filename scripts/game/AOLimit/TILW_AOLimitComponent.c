[ComponentEditorProps(category: "GameScripted/", description: "AO Limit component")]
class TILW_AOLimitComponentClass : ScriptComponentClass
{
}

class TILW_AOLimitComponent : ScriptComponent
{
	[Attribute("20", UIWidgets.Auto, "After how many seconds outside of AO players are killed", params: "0 inf 0")]
	protected float m_killTimer;
	
	[RplProp(), Attribute("", UIWidgets.Auto, desc: "Factions affected by the AO limit (if empty, all factions)")]
	protected ref array<string> m_factionKeys;
	
	[Attribute("", UIWidgets.Auto, desc: "Passengers of these vehicle prefabs (or inheriting) are NEVER affected by the AO limit")]
	protected ref array<ResourceName> m_ignoredVehicles;
	
	[Attribute("1", UIWidgets.Auto, "How many seconds pass between checking if players are still in AO", params: "0.25 inf 0.25")]
	protected float m_checkFrequency;
	
	[Attribute("", UIWidgets.ColorPicker, "The color of the AO.")]
	protected ref Color m_AOLineColor;
	
	[Attribute("0", UIWidgets.Auto, "The drawn line width. 0 = not drawn", params: "0 inf 0")]
	protected int m_AOLineWidth;
	
	[RplProp(onRplName: "OnPoints3DChange")]
	protected ref array<vector> m_points3D = new array<vector>();

	protected ref array<float> m_points2D = new array<float>();
	
	protected ref array<MapItem> m_markers = new array<MapItem>();
	
	protected float m_timeLeft = 0;
	protected float m_checkDelta = 0;
	
	protected bool m_wasOutsideAO = false;
	
	protected TILW_AOLimitDisplay m_aoLimitDisplay;
	
	void TILW_AOLimitComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.INIT);
	}
	
	protected override protected void EOnInit(IEntity owner)
	{
		PolylineShapeEntity pse = PolylineShapeEntity.Cast(GetOwner());
		if (!pse) {
			Print("TILW_AOLimitComponent | Owner entity (" + GetOwner() + ") is not a polyline!", LogLevel.WARNING);
			return;
		}
		if (pse.GetPointCount() < 3) {
			Print("TILW_AOLimitComponent | Owner entity (" + GetOwner() + ") does not have enough points!", LogLevel.WARNING);
			return;
		}

		pse.GetPointsPositions(m_points3D);
		for (int i = 0; i < m_points3D.Count(); i++) m_points3D[i] = pse.CoordToParent(m_points3D[i]);
		
		SCR_Math2D.Get2DPolygon(m_points3D, m_points2D);
		
		if (RplSession.Mode() == RplMode.Dedicated) return;

		SetEventMask(GetOwner(), EntityEvent.FIXEDFRAME);
		DrawAO();
	}
	
	protected override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		if (m_wasOutsideAO)
			UpdateTimer(timeSlice);
		
		m_checkDelta -= timeSlice;
		if (m_checkDelta > 0)
			return;
		m_checkDelta = m_checkFrequency;

		bool outsideAO = IsOutsideAO();
		
		if (m_wasOutsideAO && !outsideAO) // re-enters ao
			PlayerEntersAO();
		
		else if (!m_wasOutsideAO && outsideAO) // leaves ao
			PlayerLeavesAO();
		
		m_wasOutsideAO = outsideAO;
	}
	
	protected bool IsOutsideAO()
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gamemode.GetState() != SCR_EGameModeState.GAME)
			return false;

		IEntity ce = GetGame().GetPlayerController().GetControlledEntity();
		if (!ce)
			return false;
		
		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(ce);
		if (!player)
			return false;

		if (!SCR_AIDamageHandling.IsAlive(player))
			return false;
		
		if (!player.m_pFactionComponent)
			return false;
		
		Faction faction = player.m_pFactionComponent.GetAffiliatedFaction();
		if (!faction)
			return false;
		
		if (!m_factionKeys.IsEmpty() && !m_factionKeys.Contains(faction.GetFactionKey()))
			return false;
		
		vector playerPos = player.GetOrigin();
		bool inPolygon = Math2D.IsPointInPolygon(m_points2D, playerPos[0], playerPos[2]);
		if (inPolygon)
			return false;
		
		IEntity ve = CompartmentAccessComponent.GetVehicleIn(player);
		if (ve && IsVehicleIgnored(ve))
			return false;
		
		return true;
	}

	protected bool IsVehicleIgnored(IEntity e)
	{
		// Possible optimization: Save result for this vehicle
		
		if (m_ignoredVehicles.IsEmpty()) return false;
		
		EntityPrefabData epd = e.GetPrefabData();
		if (!epd) return false;
		BaseContainer bc = epd.GetPrefab();
		if (!bc) return false;
		foreach (ResourceName rn : m_ignoredVehicles) if (SCR_BaseContainerTools.IsKindOf(bc, rn)) return true;
			
		return false;
	}
	
	protected void UpdateTimer(float timeSlice)
	{
		m_timeLeft -= timeSlice;
		m_aoLimitDisplay.SetTime(m_timeLeft);
	
		if (m_timeLeft < 0)
		{
			IEntity player = GetGame().GetPlayerController().GetControlledEntity();
			CharacterControllerComponent characterController = CharacterControllerComponent.Cast(player.FindComponent(CharacterControllerComponent));
			if (characterController)
				characterController.ForceDeath();
			
			PlayerEntersAO();
		}
	}
	
	protected void PlayerLeavesAO()
	{
		m_timeLeft = m_killTimer;
		
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.HINT);
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		SCR_HUDManagerComponent hud = SCR_HUDManagerComponent.Cast(playerController.GetHUDManagerComponent());
		m_aoLimitDisplay = TILW_AOLimitDisplay.Cast(hud.FindInfoDisplay(TILW_AOLimitDisplay));
		m_aoLimitDisplay.SetTime(m_timeLeft);
		m_aoLimitDisplay.Show(true, UIConstants.FADE_RATE_INSTANT);
	}
	
	protected void PlayerEntersAO()
	{
		m_timeLeft = m_killTimer;
		m_aoLimitDisplay.Show(false, UIConstants.FADE_RATE_INSTANT);
	}
	
	void SetFactions(array<string> factions)
	{
		m_factionKeys = factions;
		Replication.BumpMe();
	}

	void SetPoints(array<vector> points)
	{
		if (points.Count() < 3) {
			Print("TILW_AOLimitComponent | not enough points!", LogLevel.ERROR);
			return;
		}
		
		m_points3D = points;
		Replication.BumpMe();
		
		if(RplSession.Mode() != RplMode.Dedicated)
			OnPoints3DChange();
	}
	
	protected void OnPoints3DChange()
	{
		SCR_Math2D.Get2DPolygon(m_points3D, m_points2D);
		
		EntityEvent mask = GetEventMask();
		if(mask != EntityEvent.FIXEDFRAME)
			SetEventMask(GetOwner(), EntityEvent.FIXEDFRAME);
		
		DrawAO();
	}
	
	protected void DrawAO()
	{
		if(!m_AOLineWidth)
			return;
		
		foreach(MapItem marker : m_markers)
		{
			marker.SetVisible(false);
			marker.Recycle();
		}
		m_markers.Clear();
		
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		MapItem lastItem;
		MapItem firstItem;
		
		foreach(vector point : m_points3D)
		{
			MapItem item =  mapEntity.CreateCustomMapItem();
			item.SetPos(point[0],point[2]);
			item.SetVisible(true);
			m_markers.Insert(item);

			MapDescriptorProps props = item.GetProps();
			props.SetFrontColor(Color.FromRGBA(0,0,0,0));
			props.Activate(true);
			item.SetProps(props);
			
			if(lastItem)
			{
				MapLink link = item.LinkTo(lastItem);
				MapLinkProps linkProps = link.GetMapLinkProps();
				linkProps.SetLineColor(m_AOLineColor);
				linkProps.SetLineWidth(m_AOLineWidth);
			}
			
			if(!firstItem)
				firstItem = item;
			lastItem = item;
		}
		
		MapLink link = lastItem.LinkTo(firstItem);
		MapLinkProps linkProps = link.GetMapLinkProps();
		
		linkProps.SetLineColor(m_AOLineColor);
		linkProps.SetLineWidth(m_AOLineWidth);
	}
}
