class TILW_AOLimitDisplay : SCR_InfoDisplay
{
	protected TextWidget m_timerWidget;
	
	void SetTime(float time)
	{
		if (!m_timerWidget)
			m_timerWidget = TextWidget.Cast(m_wRoot.FindAnyWidget("Timer"));
	
		m_timerWidget.SetText(time.ToString(-1,3));
		
		if (time < 10)
			m_timerWidget.SetColor(Color.Red);
		else
			m_timerWidget.SetColor(Color.White);
	}
}