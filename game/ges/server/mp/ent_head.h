///////////// Copyright 2019 Team GoldenEye:Source. All rights reserved. /////////////

#ifndef MC_ENT_HEAD_H
#define MC_ENT_HEAD_H
#ifdef _WIN32
#pragma once
#endif
#include "cbase.h"

CBaseEntity *CreateNewHead( const Vector &position, const QAngle &angles, const char* hatModel);

#endif //MC_ENT_HAT_H
