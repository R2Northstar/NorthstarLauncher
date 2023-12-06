#include <string.h>
#include "interface.h"

InterfaceReg* s_pInterfaceRegs;

InterfaceReg::InterfaceReg(InstantiateInterfaceFn fn, const char* pName) : m_pName(pName)
{
	m_CreateFn = fn;
	m_pNext = s_pInterfaceRegs;
	s_pInterfaceRegs = this;
}

void* CreateInterface(const char* pName, InterfaceStatus* pReturnCode)
{
	InterfaceReg *pCur;

	NS::log::PLUGINSYS->info("looking up interface {}", pName);

	for(pCur = s_pInterfaceRegs; pCur; pCur = pCur->m_pNext)
	{
		NS::log::PLUGINSYS->info("comparing {} and {} = {}", pCur->m_pName, pName, strcmp(pCur->m_pName, pName));
		if(strcmp(pCur->m_pName, pName) == 0)
		{
			if(pReturnCode)
			{
				*pReturnCode = InterfaceStatus::IFACE_OK;
			}

			NS::log::PLUGINSYS->info("creating interface");
			return pCur->m_CreateFn();
		}
	}

	if(pReturnCode)
	{
		*pReturnCode = InterfaceStatus::IFACE_FAILED;
	}

	NS::log::PLUGINSYS->error("could not find interface {}", pName);
	return NULL;
}

