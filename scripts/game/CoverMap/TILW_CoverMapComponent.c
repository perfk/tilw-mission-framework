[ComponentEditorProps(category: "GameScripted/", description: "Cover Map component")]
class TILW_CoverMapComponentClass : ScriptComponentClass
{
}

class TILW_CoverMapComponent : ScriptComponent
{
	[Attribute("1.778", UIWidgets.Auto, "Ratio for width to height. 1 = square", params: "1 inf 1")]
	protected float m_ratioWidth;
	
	protected vector m_center = vector.Zero;
	protected float m_radius = 0;
	
	void TILW_CoverMapComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.INIT);
	}
	
	protected override void EOnInit(IEntity owner)
	{
		PolylineShapeEntity pse = PolylineShapeEntity.Cast(owner);
		if (!pse) {
			Print("TILW_CoverMapComponent | Owner entity (" + owner + ") is not a polyline!", LogLevel.ERROR);
			return;
		}
		if (pse.GetPointCount() < 3) {
			Print("TILW_CoverMapComponent | Owner entity (" + owner + ") does not have enough points!", LogLevel.ERROR);
			return;
		}
		
		ref array<vector> points = {};
		pse.GetPointsPositions(points);
		for (int i = 0; i < points.Count(); i++) points[i] = pse.CoordToParent(points[i]);
		
		SetupCoverMap(points);
		GetGame().GetCallqueue().CallLater(CenterMap, 2500, false, m_center, m_radius * 2);
	}
	
	protected void CenterMap(vector position, float mapHeight)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if(!mapEntity || !mapEntity.IsOpen())
			return;
		
		float screenWidth, screenHeight;
		CanvasWidget widget = mapEntity.GetMapWidget();
		widget.GetScreenSize(screenWidth, screenHeight);

		mapEntity.ZoomPanSmooth(screenHeight / mapHeight, position[0], position[2], 0);
	}
	
	protected void SetupCoverMap(array<vector> points)
	{
		foreach (vector point : points)
		{
			m_center += point;
		}
	
		m_center /= points.Count();
		
		foreach (vector point : points)
		{
			float distance = vector.Distance(m_center, point);
	        	if (distance > m_radius)
	        	m_radius = distance;
		}
		
		array<vector> directions = {
		    Vector(-1, 0, 0), // Left
		    Vector(0, 0, -1), // Up
		    Vector(1, 0, 0),  // Right
		    Vector(0, 0, 1)   // Down
		};
		
		foreach (vector dir : directions)
		{
			float ratio = 1;
			if(dir[0] != 0)
				ratio = m_ratioWidth;
			
			vector postion = m_center + (dir * m_radius * ratio);
			SpawnMarker(postion, dir);
		}
	}

	protected void SpawnMarker(vector positon, vector direction)
	{
		float radius = 100000 / 2;
		
		positon = positon + (direction * radius);

		Resource resource = Resource.Load("{2B1919D7EE577D77}Prefabs/Logic/CoverMap/AO_Cover_Map.et");
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = positon;

		IEntity marker = GetGame().SpawnEntityPrefab(resource, null, spawnParams);
	}
}