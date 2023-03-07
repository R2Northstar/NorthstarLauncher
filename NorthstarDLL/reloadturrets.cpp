ON_DLL_LOAD_CLIENT("client.dll", ClientModReloadTurretSettings, (CModule module))
{
	// always reload turret settings on level load (ai and player settings do this already)
	module.Offset(0x584B85).Patch("EB"); // jle => jmp
}

ON_DLL_LOAD_CLIENT("server.dll", ServerModReloadTurretSettings, (CModule module))
{
	// always reload turret settings on level load (ai and player settings do this already)
	module.Offset(0x667115).Patch("EB"); // jle => jmp
}
