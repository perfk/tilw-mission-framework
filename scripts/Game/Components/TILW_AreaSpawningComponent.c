[ComponentEditorProps(category: "GameScripted/", description: "Area Spawning component")]
class TILW_AreaSpawningComponentClass: ScriptComponentClass
{
}

class TILW_AreaSpawningComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Auto, "Prefabs to be spawned at random locations within the area \nThe area is defined by the components owner entity, which must be a polyline", "et")]
	protected ref array<ResourceName> m_prefabs;
	
	[Attribute("1", UIWidgets.Auto, "How many of each prefab to spawn")]
	protected int m_iCount;
	
	[Attribute("1", UIWidgets.Auto, "Avoid spawning on water")]
	protected bool m_bWaterCheck;
	
	[Attribute("1", UIWidgets.Auto, "Makes rotation(Yaw) random")]
	protected bool m_bRandomYaw;
	
	[Attribute("1", UIWidgets.CheckBox, "Align spawned prefab to terrain normal")]
	protected bool m_bAlignToGround;
	
	[Attribute("0", UIWidgets.Auto, "Don't spawn inside other objects. Keep off when possible.")]
	protected bool m_bContactCheck;
	
	protected ref array<vector> m_aPoints = {};
	protected ref array<float> m_aPolygon = {};
	
	void TILW_AreaSpawningComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.INIT);
	}

	protected override void EOnInit(IEntity owner)
	{
		if (!Replication.IsServer() || !GetGame().InPlayMode())
			return;
		
		PolylineShapeEntity pse = PolylineShapeEntity.Cast(owner);
		
		if (!pse)
		{
			Print("TILW_AreaSpawningComponent | Owner entity (" + owner + ") is not a polyline!", LogLevel.ERROR);
			return;
		}
		
		if (pse.GetPointCount() < 3)
		{
			Print("TILW_AreaSpawningComponent | Owner entity (" + owner + ") does not have enough points!", LogLevel.ERROR);
			return;
		}
		
		pse.GetPointsPositions(m_aPoints);
		
		for (int i = 0; i < m_aPoints.Count(); i++)
			m_aPoints[i] = pse.CoordToParent(m_aPoints[i]);
		
		SCR_Math2D.Get2DPolygon(m_aPoints, m_aPolygon);
		
		SpawnPrefabs();
	}
	
	protected bool SpawnPrefabs()
	{
		if (m_prefabs.IsEmpty())
		{
			Print("TILW_AreaSpawningComponent | No prefabs configured!", LogLevel.ERROR);
			return false;
		}
	
		foreach (ResourceName prefab : m_prefabs)
		{
			float size;
			if (!GetPrefabSize(prefab, size))
			{
				PrintFormat("TILW_AreaSpawningComponent | Failed to get bounds for prefab '%1'!", prefab, LogLevel.ERROR);
				return false;
			}
	
			int spawnedCount = 0;
			int attempts = 0;
			int maxAttempts = 1000;
	
			while (spawnedCount < m_iCount && attempts < maxAttempts)
			{
				attempts++;
	
				float x, y;
				SCR_Math2D.GetRandomPointInPolygon(m_aPolygon, x, y);
				
				vector position = Vector(x, SCR_TerrainHelper.GetTerrainY(Vector(x, 0, y), null, true), y);
				
				int yaw = 0;
				if (m_bRandomYaw)
					yaw = Math.RandomIntInclusive(0, 359);
	
				if (m_bContactCheck)
				{
					if (!IsPositionEmpty(position, size))
						continue;
				}
	
				vector transform[4];
				Math3D.AnglesToMatrix(Vector(yaw, 0, 0), transform);
				transform[3] = position;
	
				if (m_bAlignToGround)
				{
					if (!SCR_TerrainHelper.SnapAndOrientToTerrain(transform, GetGame().GetWorld()))
						continue;
				}
	
				IEntity entity = SpawnPrefab(prefab, transform[3], yaw);
				if (!entity)
					continue;
	
				if (m_bAlignToGround)
					entity.SetWorldTransform(transform);
				
				spawnedCount++;
			}
	
			if (spawnedCount < m_iCount)
			{
				PrintFormat("TILW_AreaSpawningComponent | Failed to spawn prefab '%1'. Spawned %2 of %3 after %4 attempts.", prefab, spawnedCount, m_iCount, attempts, LogLevel.ERROR);
				return false;
			}
		}
	
		return true;
	}

	protected bool GetPrefabSize(ResourceName prefab, out float size)
	{
		size = 0;
	
		IEntity testPrefab = SpawnPrefab(prefab);
		if (!testPrefab)
			return false;
	
		vector min, max;
		testPrefab.GetBounds(min, max);
	
		float sizeX = max[0] - min[0];
		float sizeY = max[1] - min[1];
		float sizeZ = max[2] - min[2];
	
		size = Math.Max(sizeX, Math.Max(sizeY, sizeZ)) / 2;
	
		SCR_EntityHelper.DeleteEntityAndChildren(testPrefab);
		return true;
	}

	protected bool IsPositionEmpty(vector position, float radius)
	{
		TILW_PhysQuery query = new TILW_PhysQuery();

		GetGame().GetWorld().QueryEntitiesBySphere(position, radius, query.QueryCallback, null, EQueryEntitiesFlags.ALL);
		if(!query.m_bFound)
			return true;

		return false;
	}
	
	protected bool IsSurfaceWater(vector position)
	{
	    return ChimeraWorldUtils.TryGetWaterSurfaceSimple(GetGame().GetWorld(), position);
	}
	
	protected IEntity SpawnPrefab(ResourceName prefab, vector position = vector.Zero, float yaw = 0)
	{
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.Transform[3] = position;
		
		IEntity entity = GetGame().SpawnEntityPrefab(Resource.Load(prefab), null, spawnParams);

		entity.SetYawPitchRoll(Vector(yaw,0,0));
		
		return entity;
	}
}

class TILW_PhysQuery : TILW_BaseQuery
{
	ref array<IEntity> m_aIgnoreEntities;
	bool m_bFound;
	
	override bool QueryCallback(IEntity entity)
	{
		if(entity != entity.GetRootParent())
			return true;
		
		if(m_aIgnoreEntities && m_aIgnoreEntities.Contains(entity))
			return true;
		
		if(!entity.GetPhysics())
			return true;
		
		m_bFound = true;
		
		return false;
	}
}

class TILW_BaseQuery
{
	protected ref array<IEntity> m_aEntities = {};
	
	bool QueryCallback(IEntity entity)
	{
		m_aEntities.Insert(entity);
		
		return true;
	}

	void Clear()
	{
		m_aEntities.Clear();
	}
	
	array<IEntity> GetResults()
	{
		return m_aEntities;
	}
	
	bool ArrayContainsKind(IEntity entity, array<ResourceName> list)
	{
		EntityPrefabData epd = entity.GetPrefabData();
		if (!epd)
			return false;

		BaseContainer bc = epd.GetPrefab();
		if (!bc)
			return false;
		
		foreach (ResourceName rn : list)
			if (SCR_BaseContainerTools.IsKindOf(bc, rn))
				return true;

		return false;
	}
}
