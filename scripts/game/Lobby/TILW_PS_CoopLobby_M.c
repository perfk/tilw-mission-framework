modded class PS_CoopLobby
{
	protected TextWidget m_wRatioCounter;
	
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		SetRatio();
	}
	
	override void OnPlayerDisconnected(int playerId)
	{
		super.OnPlayerDisconnected(playerId);
		SetRatio();
	}
	
	protected void SetRatio()
	{
		int totalPlayers = m_PlayerManager.GetPlayerCount();
		array<int> playersPerRatio();
	
		if(m_mFactions.Count() < 2)
		{
			TextWidget ratioHeader = TextWidget.Cast(m_wRoot.FindAnyWidget("RatioHeader"));
			ratioHeader.SetVisible(false);
			return;
		}
		
		int totalSlots = 0;
		foreach (SCR_Faction faction, PS_FactionSelector selector : m_mFactions)
		{
			totalSlots += selector.GetMaxCount();
		}
		
		array<float> ratios = {};
		foreach (SCR_Faction faction, PS_FactionSelector selector : m_mFactions)
		{
			float ratio = selector.GetMaxCount() / totalSlots;
			ratios.Insert(ratio);
		}
		ratios.Debug();
		int remainingPlayers = totalPlayers;
		for (int i = 0; i < ratios.Count(); i++)
		{
			int playersForThisRatio = Math.Round((float)ratios[i] * totalPlayers);
			playersPerRatio.Insert(playersForThisRatio);
			
			remainingPlayers -= playersForThisRatio;
		}
		
		if (remainingPlayers > 0)
		{
			int maxIndex = 0;
			for (int i = 1; i < playersPerRatio.Count(); i++)
			{
				if (ratios[i] > ratios[maxIndex])
					maxIndex = i;
			}
			playersPerRatio[maxIndex] = playersPerRatio[maxIndex] + remainingPlayers;
		}
		
		string formattedRatio = "";
		for (int i = 0; i < playersPerRatio.Count(); i++)
		{
			formattedRatio += playersPerRatio[i].ToString();

			if (i < playersPerRatio.Count() - 1)
				formattedRatio += " : ";
		}
		
		m_wRatioCounter.SetText(formattedRatio);
	}
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		
		m_wRatioCounter = TextWidget.Cast(m_wRoot.FindAnyWidget("RatioCounter"));
		SetRatio();
	}
}