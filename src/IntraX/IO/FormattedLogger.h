#pragma once

#include "IntraX/Utils/Logger.h"
#include "IntraX/IO/FormattedWriter.h"
#include "IntraX/Math/Vector3.h"

namespace Intra { INTRA_BEGIN
class FormattedLogger: public ILogger
{
public:
	INTRA_FORCEINLINE explicit FormattedLogger(FormattedWriter writer=nullptr) noexcept: Writer(Move(writer)) {}
	FormattedLogger(const FormattedLogger&) = delete;
	FormattedLogger(FormattedLogger&&) = default;
	FormattedLogger& operator=(const FormattedLogger&) = delete;
	FormattedLogger& operator=(FormattedLogger&&) = default;

	void Log(LogLevel level, StringView msg, SourceInfo srcInfo = SourceInfo()) override;

	FormattedWriter Writer;
	LogLevel Verbosity = LogLevel::All;
	Vec3 InfoColor = {0.45f, 0.45f, 0.45f};
	Vec3 SuccessColor = {0, 0.6f, 0};
	Vec3 PerfWarningColor = {0.55f, 0.55f, 0.2f};
	Vec3 WarningColor = {0.9f, 0.75f, 0.2f};
	Vec3 ErrorColor = {1, 0.15f, 0.1f};
	Vec3 CriticalErrorColor = {0.8f, 0, 0};
	bool WriteLevelType = true;
};
} INTRA_END