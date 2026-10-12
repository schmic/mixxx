#pragma once

#include <QSqlDatabase>

#include "analyzer/analyzertrack.h"
#include "preferences/usersettings.h"

namespace mixxx::rbl {

struct Eligibility {
    bool beats = false;
    bool key = false;
    bool waveform = false;
};

bool selected(const UserSettingsPointer& config);
Eligibility eligibility(const AnalyzerTrack& track,
        const UserSettingsPointer& config,
        const QSqlDatabase& database,
        bool withWaveform,
        bool forceBeats);
void analyzeAndStore(const AnalyzerTrack& track,
        const UserSettingsPointer& config,
        const QSqlDatabase& database,
        Eligibility eligible);

} // namespace mixxx::rbl
