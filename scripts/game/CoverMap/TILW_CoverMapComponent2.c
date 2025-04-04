[ComponentEditorProps(category: "GameScripted/", description: "Cover Map component")]
class TILW_CoverMapComponent2Class : ScriptComponentClass
{
}

class TILW_CoverMapComponent2 : ScriptComponent
{
	
	[Attribute("0 0 0 0.25", UIWidgets.ColorPicker, desc: "Color of the shape, you can also set transparency.", category: "Visualization")]
	protected ref Color m_color;
	
	SCR_MapEntity m_MapEntity;
	
	[RplProp(onRplName: "OnPoints3DChange")]
	protected ref array<vector> m_points3D = new array<vector>();
	
	protected ref array<float> m_points2D_1 = new array<float>();
	protected ref array<float> m_points2D_2 = new array<float>();
	
	
	[Attribute("0", UIWidgets.Auto, "Invert the shape so it surrounds the polyline area.", category: "Visualization")]
	protected bool m_invert;
	
	[Attribute("", UIWidgets.Auto, "If defined, only display for the given factions.", category: "Visualization")]
	protected ref array<FactionKey> m_factionKeys;
	
	[Attribute("0", UIWidgets.Auto, "Is it visible before slotting or for spectators?", category: "Visualization")]
	protected bool m_visibleForEmptyFaction;
	
	
	
	
	protected bool IsVisible()
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return true;
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return true;
		
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliationComponent = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!playerFactionAffiliationComponent)
			return true;
		
		Faction faction = playerFactionAffiliationComponent.GetAffiliatedFaction();
		FactionKey fkey = "";
		if (faction)
			fkey = faction.GetFactionKey();
		
		if (fkey == "")
			return m_visibleForEmptyFaction;
		
		return (m_factionKeys.IsEmpty() || m_factionKeys.Contains(fkey));
	}
	
	override void OnPostInit(IEntity owner)
	{
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		ScriptInvokerBase<MapConfigurationInvoker> onMapOpen = m_MapEntity.GetOnMapOpen();
		ScriptInvokerBase<MapConfigurationInvoker> onMapClose = m_MapEntity.GetOnMapClose();
		
		onMapOpen.Insert(CreateMapWidget);
		onMapClose.Insert(DeleteMapWidget);
		
		GetGame().GetCallqueue().Call(InitPoints);
	}
	
	void SetPoints3D(array<vector> points3D)
	{
		if (points3D.Count() < 3)
		{
			Print("TILW_ | Not enough points!", LogLevel.ERROR);
			return;
		}

		m_points3D = points3D;
		Replication.BumpMe();

		OnPoints3DChange();
	}
	
	protected void InitPoints()
	{
		m_points3D = {};
		
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
		
		for (int i = 0; i < m_points3D.Count(); i++)
			m_points3D[i] = pse.CoordToParent(m_points3D[i]);
		
		OnPoints3DChange();
	}
		
		
	protected void OnPoints3DChange()
	{
		m_points2D_1 = {};
		m_points2D_2 = {};
		
		if (m_invert)
			InvertPolygon();
		else
			SCR_Math2D.Get2DPolygon(m_points3D, m_points2D_1);
		
	}
	
	protected void InvertPolygon()
	{
		// don't ask what the fuck this is
		
		vector v_bl = Vector(0, 0, 0);
		vector v_tr = Vector(4096, 0, 4096);
		
		vector v_tl = Vector(0, 0, 4096);
		vector v_br = Vector(4096, 0, 0);
		
		float min_bl, min_tr;
		int i_bl = -1;
		int i_tr = -1;
		
		for (int i = 0; i < m_points3D.Count(); i++)
		{
			vector v_cur = Vector(m_points3D[i][0], 0, m_points3D[i][2]);
			float cur_bl = vector.Distance(v_cur, v_bl);
			if (i_bl == -1 || cur_bl < min_bl)
			{
				i_bl = i;
				min_bl = cur_bl;
			}
			float cur_tr = vector.Distance(v_cur, v_tr);
			if (i_tr == -1 || cur_tr < min_tr)
			{
				i_tr = i;
				min_tr = cur_tr;
			}
		}
		
		array<vector> pg_1 = {};
		array<vector> pg_2 = {};
		
		bool inserting = false;
		for (int i = 0; i < m_points3D.Count(); i++) // bl to tr
		{
			if (i == i_bl)
				inserting = true;
			if (inserting)
				pg_1.Insert(m_points3D[i]);
			if (inserting && i == i_tr)
				break;
			if (i == m_points3D.Count() - 1)
				i = -1;
		}
		inserting = false;
		for (int i = 0; i < m_points3D.Count(); i++) // tr to bl
		{
			if (i == i_tr)
				inserting = true;
			if (inserting)
				pg_2.Insert(m_points3D[i]);
			if (inserting && i == i_bl)
				break;
			if (i == m_points3D.Count() - 1)
				i = -1;
		}
		
		bool cw = IsClockwise();
		
		pg_1.Insert(v_tr);
		if (cw)
			pg_1.Insert(v_tl);
		else
			pg_1.Insert(v_br);
		pg_1.Insert(v_bl);
		
		pg_2.Insert(v_bl);
		if (cw)
			pg_2.Insert(v_br);
		else
			pg_2.Insert(v_tl);
		pg_2.Insert(v_tr);
	
		SCR_Math2D.Get2DPolygon(pg_1, m_points2D_1);
		SCR_Math2D.Get2DPolygon(pg_2, m_points2D_2);
		
	}
	
	protected bool IsClockwise()
	{
		float sum = 0;
		for (int i = 0; i < m_points3D.Count(); i++)
		{
			vector p1 = m_points3D[i];
			vector p2 = m_points3D[(i + 1) % m_points3D.Count()];
			sum += (p2[0] - p1[0]) * (p2[2] + p1[2]);
		}
		return (sum > 0);
	}
	
	CanvasWidget m_wCanvasWidget;

	protected ref PolygonDrawCommand m_drawPolygon1 = new PolygonDrawCommand();
	protected ref PolygonDrawCommand m_drawPolygon2 = new PolygonDrawCommand();
	protected ref array<ref CanvasWidgetCommand> m_drawCommands = null;
	
	void CreateMapWidget(MapConfiguration mapConfig)
	{
		if (!IsVisible())
			return;
		
		Widget mapFrame = m_MapEntity.GetMapMenuRoot().FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!mapFrame)
			mapFrame = m_MapEntity.GetMapMenuRoot();
		if (!mapFrame)
			 return;
		
		m_wCanvasWidget = CanvasWidget.Cast(GetGame().GetWorkspace().CreateWidgets("{B8793707B56B2F9F}UI/Map/PolyMapMarkerBase.layout", mapFrame));
		
		m_drawPolygon1.m_iColor = m_color.PackToInt();
		m_drawPolygon2.m_iColor = m_color.PackToInt();
		
		if (!m_drawCommands)
		{
			if (m_invert)
				m_drawCommands = { m_drawPolygon1, m_drawPolygon2 };
			else
				m_drawCommands = { m_drawPolygon1 };
		}
		
		m_wCanvasWidget.SetDrawCommands(m_drawCommands);
		
		SetEventMask(GetOwner(), EntityEvent.POSTFRAME);
	}
	
	void DeleteMapWidget(MapConfiguration mapConfig)
	{
		ClearEventMask(GetOwner(), EntityEvent.POSTFRAME);
	}
	
	override void EOnPostFrame(IEntity owner, float timeSlice)
	{
		m_drawPolygon1.m_Vertices = new array<float>();
		for (int i = 0; i < m_points2D_1.Count(); i += 2)
		{
			float screenX, screenY;
			m_MapEntity.WorldToScreen(m_points2D_1[i], m_points2D_1[i+1], screenX, screenY, true);
			
			m_drawPolygon1.m_Vertices.Insert(screenX);
			m_drawPolygon1.m_Vertices.Insert(screenY);
		}
		
		if (m_invert)
		{
			m_drawPolygon2.m_Vertices = new array<float>();
			for (int i = 0; i < m_points2D_2.Count(); i += 2)
			{
				float screenX, screenY;
				m_MapEntity.WorldToScreen(m_points2D_2[i], m_points2D_2[i+1], screenX, screenY, true);
				
				m_drawPolygon2.m_Vertices.Insert(screenX);
				m_drawPolygon2.m_Vertices.Insert(screenY);
			}
		}
	}
}