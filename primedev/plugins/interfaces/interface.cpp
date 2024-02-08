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
	for (InterfaceReg* pCur = s_pInterfaceRegs; pCur; pCur = pCur->m_pNext)
	{
		if (strcmp(pCur->m_pName, pName) == 0)
		{
			if (pReturnCode)
			{
				*pReturnCode = InterfaceStatus::IFACE_OK;
			}

			DevMsg(eLog::PLUGSYS, "creating interface %s\n", pName);
			return pCur->m_CreateFn();
		}
	}

	if (pReturnCode)
	{
		*pReturnCode = InterfaceStatus::IFACE_FAILED;
	}

	Error(eLog::PLUGSYS, NO_ERROR, "could not find interface %s\n", pName);
	return NULL;
}
