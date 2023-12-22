#include "toolframework/itoolentity.h"
#include "game/client/cdll_client_int.h"

class CClientTools : public IClientTools
{
  public:
};

ON_DLL_LOAD("client.dll", ClientClientTools, (CModule module))
{
	g_pClientTools = Sys_GetFactoryPtr("client.dll", "VCLIENTTOOLS001").RCast<IClientTools*>();
}
