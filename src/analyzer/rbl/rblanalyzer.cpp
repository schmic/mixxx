#include "analyzer/rbl/rblanalyzer.h"

#ifdef MIXXX_RBL_ANALYSIS
#include <QVector>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

#include "analyzer/rbl/bridge.h"
#include "library/rekordbox/rekordboxconstants.h"
#include "library/dao/analysisdao.h"
#include "proto/keys.pb.h"
#include "preferences/beatdetectionsettings.h"
#include "preferences/keydetectionsettings.h"
#include "preferences/waveformsettings.h"
#include "track/beats.h"
#include "track/keyfactory.h"
#include "track/keyutils.h"
#include "track/track.h"
#include "util/logger.h"
#include "waveform/waveform.h"
#include "waveform/waveformfactory.h"
#endif

namespace mixxx::rbl {
#ifdef MIXXX_RBL_ANALYSIS
namespace {

const Logger kLogger("RblAnalyzer");
const QString kBackendId = QStringLiteral("rbl-analysis");
const QString kBeatsSubVersion = QStringLiteral("rbl-analysis-1");

using ResultPtr = std::unique_ptr<RblResult, decltype(&rbl_result_free)>;

bool shouldReplaceBeats(const TrackPointer& track,
        const BeatDetectionSettings& settings,
        bool force) {
    if ((!force && !settings.getBpmDetectionEnabled()) || track->isBpmLocked()) {
        return false;
    }
    const auto beats = track->getBeats();
    if (!beats) {
        return true;
    }
    if (beats->getSubVersion() == kBeatsSubVersion) {
        return false;
    }
    if (beats->getSubVersion() == rekordboxconstants::beatsSubversion) {
        return settings.getReanalyzeImported();
    }
    return settings.getReanalyzeWhenSettingsChange();
}

bool shouldReplaceKey(const TrackPointer& track,
        const KeyDetectionSettings& settings) {
    if (!settings.getKeyDetectionEnabled()) {
        return false;
    }
    const auto keys = track->getKeys();
    if (keys.getGlobalKey() == track::io::key::INVALID) {
        return true;
    }
    return keys.getSubVersion() != QStringLiteral("analyzer=rbl-analysis-1") &&
            settings.getReanalyzeWhenSettingsChange();
}

WaveformPointer makeWaveform(const RblColumn* columns,
        std::size_t count,
        audio::SampleRate sampleRate,
        SINT frames,
        bool summary) {
    if (!columns || count == 0) {
        return {};
    }
    auto waveform = WaveformPointer(new Waveform(sampleRate, frames,
            summary ? 441 : 150,
            summary ? static_cast<int>(count * ChannelCount) : -1,
            0));
    const int pairs = waveform->getDataSize() / ChannelCount;
    for (int i = 0; i < pairs; ++i) {
        const auto source = std::min<std::size_t>(
                static_cast<std::size_t>(i) * count / std::max(pairs, 1), count - 1);
        for (int channel = 0; channel < ChannelCount; ++channel) {
            auto& filtered = waveform->data()[i * ChannelCount + channel].filtered;
            filtered.low = columns[source].low;
            filtered.mid = columns[source].mid;
            filtered.high = columns[source].high;
            filtered.all = columns[source].peak;
        }
    }
    waveform->setCompletion(waveform->getDataSize());
    waveform->setVersion(summary ? WaveformFactory::currentWaveformSummaryVersion()
                                 : WaveformFactory::currentWaveformVersion());
    waveform->setDescription(summary ? WaveformFactory::currentWaveformSummaryDescription()
                                     : WaveformFactory::currentWaveformDescription());
    waveform->setSaveState(Waveform::SaveState::SavePending);
    return waveform;
}

} // namespace
#endif

bool selected(const UserSettingsPointer& config) {
#ifdef MIXXX_RBL_ANALYSIS
    return BeatDetectionSettings(config).getBeatPluginId() == kBackendId;
#else
    Q_UNUSED(config);
    return false;
#endif
}

Eligibility eligibility(const AnalyzerTrack& analysisTrack,
        const UserSettingsPointer& config,
        const QSqlDatabase& database,
        bool withWaveform,
        bool forceBeats) {
#ifdef MIXXX_RBL_ANALYSIS
    const auto& track = analysisTrack.getTrack();
    bool waveformWanted = withWaveform && database.isOpen() &&
            WaveformSettings(config).waveformGenerationWithAnalysisEnabled();
#ifdef __STEM__
    if (!track->getStemInfo().isEmpty()) {
        waveformWanted = false;
    }
#endif
    if (waveformWanted && track->getId().isValid()) {
        AnalysisDao dao(config);
        dao.initialize(database);
        bool hasDetail = !track->getWaveform().isNull();
        bool hasSummary = !track->getWaveformSummary().isNull();
        for (const auto& analysis : dao.getAnalysesForTrack(track->getId())) {
            if (analysis.type == AnalysisDao::TYPE_WAVEFORM &&
                    WaveformFactory::waveformVersionToVersionClass(analysis.version) ==
                            WaveformFactory::VC_USE) {
                hasDetail = true;
            } else if (analysis.type == AnalysisDao::TYPE_WAVESUMMARY &&
                    WaveformFactory::waveformSummaryVersionToVersionClass(analysis.version) ==
                            WaveformFactory::VC_USE) {
                hasSummary = true;
            }
        }
        waveformWanted = !hasDetail || !hasSummary;
    } else if (waveformWanted) {
        waveformWanted = track->getWaveform().isNull() ||
                track->getWaveformSummary().isNull();
    }
    return {
            shouldReplaceBeats(track, BeatDetectionSettings(config), forceBeats),
            shouldReplaceKey(track, KeyDetectionSettings(config)),
            waveformWanted};
#else
    Q_UNUSED(analysisTrack);
    Q_UNUSED(config);
    Q_UNUSED(database);
    Q_UNUSED(withWaveform);
    Q_UNUSED(forceBeats);
    return {};
#endif
}

void analyzeAndStore(const AnalyzerTrack& analysisTrack,
        const UserSettingsPointer& config,
        const QSqlDatabase& database,
        Eligibility eligible) {
#ifdef MIXXX_RBL_ANALYSIS
    const auto& track = analysisTrack.getTrack();
    if (!eligible.beats && !eligible.key && !eligible.waveform) {
        return;
    }
    const auto path = track->getLocation().toUtf8();
    ResultPtr result(rbl_analyze_path(path.constData()), rbl_result_free);
    if (!result) {
        kLogger.warning() << "Cannot analyze" << track->getLocation()
                          << "with rbl; retaining Mixxx analyzer results";
        return;
    }
    const auto sampleRate = track->getSampleRate();
    if (!sampleRate.isValid() || result->sample_rate == 0 ||
            sampleRate != audio::SampleRate(result->sample_rate)) {
        kLogger.warning() << "Sample-rate mismatch for" << track->getLocation();
        return;
    }
    if (eligible.beats && result->beats_len > 1 && result->beats &&
            result->beats_len <= static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        QVector<audio::FramePos> positions;
        positions.reserve(static_cast<int>(result->beats_len));
        double previous = -1;
        for (std::size_t i = 0; i < result->beats_len; ++i) {
            const double frame = static_cast<double>(result->beats[i].time_ms) *
                    sampleRate / 1000.0;
            if (frame > previous && std::isfinite(frame)) {
                positions.push_back(audio::FramePos(frame));
                previous = frame;
            }
        }
        if (positions.size() > 1) {
            const auto candidate = Beats::fromBeatPositions(
                    sampleRate, positions, kBeatsSubVersion);
            if (candidate) {
                track->trySetBeats(candidate);
            }
        }
    }
    if (eligible.key && result->key) {
        const QString name = QString::fromUtf8(result->key);
        const auto chromatic = KeyUtils::guessKeyFromText(name);
        if (chromatic != track::io::key::INVALID) {
            KeyChangeList changes;
            changes.append(qMakePair(chromatic, 0.0));
            track->setKeys(KeyFactory::makePreferredKeys(changes,
                    {{QStringLiteral("analyzer"), QStringLiteral("rbl-analysis-1")}},
                    sampleRate, static_cast<SINT>(track->getDuration() * sampleRate)));
        }
    }
    if (eligible.waveform && result->columns_len && result->overview_len) {
        auto waveform = makeWaveform(result->columns, result->columns_len,
                sampleRate, static_cast<SINT>(track->getDuration() * sampleRate), false);
        auto summary = makeWaveform(result->overview, result->overview_len,
                sampleRate, static_cast<SINT>(track->getDuration() * sampleRate), true);
        if (waveform && summary && track->getId().isValid() && database.isOpen()) {
            AnalysisDao dao(config);
            dao.initialize(database);
            dao.saveTrackAnalyses(track->getId(), waveform, summary);
            track->setWaveform(waveform);
            track->setWaveformSummary(summary);
        }
    }
#else
    Q_UNUSED(analysisTrack);
    Q_UNUSED(config);
    Q_UNUSED(database);
    Q_UNUSED(eligible);
#endif
}

} // namespace mixxx::rbl
