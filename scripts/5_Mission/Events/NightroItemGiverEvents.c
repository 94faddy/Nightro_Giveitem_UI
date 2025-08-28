class NightroItemGiverNotificationHelper
{
    static void SendNotification(PlayerBase player, string message, string color = "ColorWhite")
    {
        if (player)
        {
            GetGame().ChatMP(player, message, color);
        }
    }
}

modded class MissionGameplay
{
    void SendNightroNotification(string message, string color = "ColorWhite")
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player)
        {
            GetGame().ChatMP(player, message, color);
        }
    }
}