#ifndef INCLUDED_QSimpleIni_h
#define INCLUDED_QSimpleIni_h

#include <QString>
#include "SimpleIni.h"

QString __getCSimpleIniValue(CSimpleIniA& ini, 
	const char* a_pSection,
	const char* a_pKey,
	const char* a_pDefault) {
	const char* pv;
	pv = ini.GetValue(a_pSection, a_pKey, a_pDefault);
	return QString::fromUtf8(pv);
}

#endif