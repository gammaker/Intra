﻿#pragma once

#include "Intra/Range/StringView.h"

namespace Intra { INTRA_BEGIN
void InitSignals();
StringView CrashSignalName(int signum);
StringView CrashSignalDesc(int signum);
extern void(*CrashHandler)(int signum);
} INTRA_END