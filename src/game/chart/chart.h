#pragma once
#include <array>
#include <vector>
#include <list>
#include <utility>
#include <chrono>
#include "common/beat.h"
#include "game/runtime/state.h"
#include "game/input/input_mgr.h"

struct HitableNote: Note
{
    bool expired = false;
    bool hit = false;
};

namespace chart
{

enum class NoteLaneCategory : size_t
{
    _ = -1ull, // INVALID

    Note = 0,
    Mine,
    Invs,
    LN,
    EXTRA,
    NOTECATEGORY_COUNT,
};

enum NoteLaneIndex : size_t
{
    _ = -1ull, // INVALID
    Sc1 = 0,
    K1,
    K2,
    K3,
    K4,
    K5,
    K6,
    K7,
    K8,
    K9,
    K10,
    K11,
    K12,
    K13,
    K14,
    K15,
    K16,
    K17,
    K18,
    K19,
    K20,
    K21,
    K22,
    K23,
    K24,
    Sc2,
    NOTELANEINDEX_COUNT,
};

typedef std::pair<NoteLaneCategory, NoteLaneIndex> NoteLane;

enum NoteLaneExtra : size_t
{
    EXTRA_BARLINE_1P,
    EXTRA_BARLINE_2P,

    NOTELANEEXTRA_COUNT = NOTELANEINDEX_COUNT,
};

constexpr size_t LANE_ALL_KEY_COUNT = size_t(NoteLaneCategory::NOTECATEGORY_COUNT) * NOTELANEINDEX_COUNT;
constexpr size_t LANE_BARLINE_1P = LANE_ALL_KEY_COUNT + 0;
constexpr size_t LANE_BARLINE_2P = LANE_ALL_KEY_COUNT + 1;
constexpr size_t LANE_INVALID = LANE_ALL_KEY_COUNT + 2;
constexpr size_t LANE_COUNT = LANE_INVALID + 1;
constexpr size_t channelToIdx(NoteLaneCategory cat, size_t idx)
{
    if (cat == NoteLaneCategory::EXTRA)
    {
        if (idx == NoteLaneExtra::EXTRA_BARLINE_1P)
            return LANE_BARLINE_1P;
        else if (idx == NoteLaneExtra::EXTRA_BARLINE_2P)
            return LANE_BARLINE_2P;
    }

    auto ch = size_t(cat) * NOTELANEINDEX_COUNT + idx;
    return (ch >= LANE_ALL_KEY_COUNT) ? LANE_INVALID : ch;
}
constexpr NoteLane idxToChannel(size_t idx)
{
    if (idx >= size_t(NoteLaneCategory::EXTRA) * NoteLaneIndex::NOTELANEINDEX_COUNT + NoteLaneExtra::NOTELANEEXTRA_COUNT)
        return { NoteLaneCategory::_, NoteLaneIndex::_ };

    return { NoteLaneCategory(idx / NOTELANEINDEX_COUNT), NoteLaneIndex(idx % NOTELANEINDEX_COUNT) };
}

}

class ::ChartFormatBase;

// Chart in-game data representation. Contains following:
//  - Converts plain-beat to real-beat (adds up Stop beat) 
//  - Converts time to beat (if necessary)
//  - Converts (or reads directly) beat to time
//  - Bar Time Indicator
//  - Segmented charting speed (power, 0 at stop)
// [Input]  Current Time(hTime)
// [Output] Current Bar(unsigned) 
//          Metre(double)
//          BPM(BPM)
//          Expired Notes List (including normal, plain, ext, speed)
class ChartObjectBase
{
public:
    static constexpr size_t MAX_MEASURES = 1000;

protected:
    int _playerSlot = -1;
    int _keys = 7;

protected:
	unsigned _noteCount_total;
	unsigned _noteCount_regular;
	unsigned _noteCount_ln;

public:
    // Includes normal notes and LNs; One LN counts as one note
	unsigned constexpr getNoteTotalCount() const { return _noteCount_total; }

    // Normal notes
	unsigned constexpr getNoteRegularCount() const { return _noteCount_regular; }

    // LNs; One LN counts as one note
	unsigned constexpr getNoteLnCount() const { return _noteCount_ln; }

protected:
     // full list of corresponding channel through all measures; only this list is handled by input looper
    std::array<std::list<HitableNote>, chart::LANE_COUNT> _noteLists;
    std::vector<std::list<Note>> _bgmNoteLists;      // BGM notes; handled with timer
    std::vector<std::list<Note>> _specialNoteLists;     // Special definitions for each format. e.g. BGA, Stop
    std::list<Note>              _bpmNoteList;          // BPM change is so common that they are not special

protected:
    std::vector<Metre>   barMetreLength;
    std::vector<Metre>   _barMetrePos;
    std::vector<Time>    _barTimestamp;

    Time   _totalLength;
    Time   _leadInTime = 0;    // when the first sound note locates

    double _averageBPM = 0.;
    double _mainBPM = 0.;

public:
    ChartObjectBase() = delete;
    ChartObjectBase(int slot, size_t plain_n, size_t ext_n);
    static std::shared_ptr<ChartObjectBase> createFromChartFormat(int slot, std::shared_ptr<ChartFormatBase> p);

protected:
    std::array<decltype(_noteLists.front().begin()), chart::LANE_COUNT>    _noteListIterators;
    std::vector<decltype(_bgmNoteLists.front().begin())>            _bgmNoteListIters;
    std::vector<decltype(_specialNoteLists.front().begin())>        _specialNoteListIters;
    decltype(_bpmNoteList.begin())                                  _bpmNoteListIter;

public:
    auto firstNote            (chart::NoteLaneCategory cat, chart::NoteLaneIndex idx) -> decltype(_noteLists.front().begin());

    auto incomingNote         (chart::NoteLaneCategory cat, chart::NoteLaneIndex idx) -> decltype(_noteLists.front().begin());
    auto incomingNoteBgm      (size_t idx) -> decltype(_bgmNoteLists.front().begin());
    auto incomingNoteSpecial  (size_t idx) -> decltype(_specialNoteLists.front().begin());
    auto incomingNoteBpm      () -> decltype(_bpmNoteListIter);

    bool isLastNote           (chart::NoteLaneCategory cat, chart::NoteLaneIndex idx);
    bool isLastNoteBgm        (size_t idx);
    bool isLastNoteSpecial    (size_t idx);
    bool isLastNoteBpm        ();

    bool isLastNote           (chart::NoteLaneCategory cat, chart::NoteLaneIndex idx, decltype(_noteListIterators.front()) it);
    bool isLastNoteBgm        (size_t idx, decltype(_bgmNoteListIters.front()) it);
    bool isLastNoteSpecial    (size_t idx, decltype(_specialNoteListIters.front()) it);
    bool isLastNoteBpm        (decltype(_bpmNoteListIter) it);

protected:
    auto nextNote             (chart::NoteLaneCategory cat, chart::NoteLaneIndex idx) -> decltype(_noteListIterators.front());
    auto nextNoteBgm          (size_t idx) -> decltype(_bgmNoteListIters.front());
    auto nextNoteSpecial      (size_t idx) -> decltype(_specialNoteListIters.front());
    auto nextNoteBpm          () -> decltype(_bpmNoteListIter);

public:
    Time getBarLength(size_t bar);
    Time getCurrentBarLength();
    Metre getBarMetre(size_t bar);
	Metre getCurrentBarMetre();
    Metre getBarMetrePosition(size_t bar);
	Time getBarTimestamp(size_t m) { return m < MAX_MEASURES ? _barTimestamp[m] : LLONG_MAX; }
	Time getCurrentBarTimestamp() { return getBarTimestamp(_currentBar); }
    Time getLeadInTime() const { return _leadInTime.norm() > 200 ? Time(_leadInTime.hres() - 200 * 1000000, true) : Time(0); }

protected:
    unsigned _currentBarTemp       = 0;
    unsigned _currentBar           = 0;
    double   _currentMetreTemp     = 0.;
    double   _currentMetre         = 0.;
    BPM      _currentBPM           = 150.0;
    Time     _currentBeatLength    = Time::singleBeatLengthFromBPM(150.0);
    Time     _lastChangedBPMTime   = 0;
    double   _lastChangedBPMMetre  = 0.;

public:
    void reset();
    void resetNoteListsIterators();            // set after parsing
    /*virtual*/ void update(const Time& rt);    // call with RELATIVE time
    virtual void preUpdate(const Time& rt) = 0;
    virtual void postUpdate(const Time& rt) = 0;
    constexpr auto getCurrentBar() -> decltype(_currentBar) { return _currentBar; }
    constexpr auto getCurrentMetre() -> decltype(_currentMetre) { return _currentMetre; }
    constexpr auto getCurrentBPM() -> decltype(_currentBPM) { return _currentBPM; }

public:
    std::list<HitableNote>  noteExpired;
    std::list<Note>   noteBgmExpired;
    std::list<Note>   noteSpecialExpired;

public:
    virtual chart::NoteLaneIndex getLaneFromKey(chart::NoteLaneCategory cat, Input::Pad input) = 0;
    virtual std::vector<Input::Pad> getInputFromLane(size_t channel) = 0;

    inline const Time& getTotalLength() const { return _totalLength; }
    constexpr auto getAverageBPM() -> decltype(_averageBPM) { return _averageBPM; }
    constexpr auto getMainBPM() -> decltype(_mainBPM) { return _mainBPM; }
};

