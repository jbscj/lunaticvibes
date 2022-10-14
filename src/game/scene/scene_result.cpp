#include "scene_result.h"
#include "scene_context.h"
#include "common/types.h"
#include "game/ruleset/ruleset.h"
#include "game/ruleset/ruleset_bms.h"

#include "game/sound/sound_mgr.h"
#include "game/sound/sound_sample.h"

#include "config/config_mgr.h"
#include "game/generic_info.h"
#include <boost/algorithm/string.hpp>

SceneResult::SceneResult() : vScene(eMode::RESULT, 1000)
{
    _scene = eScene::RESULT;

    _inputAvailable = INPUT_MASK_FUNC;

    if (gPlayContext.chartObj[PLAYER_SLOT_PLAYER] != nullptr)
    {
        _inputAvailable |= INPUT_MASK_1P;
    }
        
    if (gPlayContext.chartObj[PLAYER_SLOT_TARGET] != nullptr)
    {
        _inputAvailable |= INPUT_MASK_2P;
    }

    _state = eResultState::DRAW;

    // set options
    auto d1p = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]->getData();
    State::set(IndexOption::RESULT_RANK_1P, Option::getRankType(d1p.total_acc));
    gPlayContext.ruleset[PLAYER_SLOT_PLAYER]->updateGlobals();

    if (gPlayContext.ruleset[PLAYER_SLOT_TARGET])
    {
        auto d2p = gPlayContext.ruleset[PLAYER_SLOT_TARGET]->getData();
        State::set(IndexOption::RESULT_RANK_2P, Option::getRankType(d2p.total_acc));
        gPlayContext.ruleset[PLAYER_SLOT_TARGET]->updateGlobals();

        State::set(IndexNumber::PLAY_1P_EXSCORE_DIFF, d1p.score2 - d2p.score2);
        State::set(IndexNumber::PLAY_2P_EXSCORE_DIFF, d2p.score2 - d1p.score2);

        if (d1p.score2 > d2p.score2)
            State::set(IndexOption::RESULT_BATTLE_WIN_LOSE, 1);
        else if (d1p.score2 < d2p.score2)
            State::set(IndexOption::RESULT_BATTLE_WIN_LOSE, 2);
        else
            State::set(IndexOption::RESULT_BATTLE_WIN_LOSE, 0);
    }

    // TODO set chart info (total notes, etc.)
    auto chartLength = gPlayContext.chartObj[PLAYER_SLOT_PLAYER]->getTotalLength().norm() / 1000;
    State::set(IndexNumber::PLAY_MIN, int(chartLength / 60));
    State::set(IndexNumber::PLAY_SEC, int(chartLength % 60));
    State::set(IndexNumber::PLAY_REMAIN_MIN, int(chartLength / 60));
    State::set(IndexNumber::PLAY_REMAIN_SEC, int(chartLength % 60));

    // compare to db record
    auto dp = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]->getData();
    Option::e_rank_type nowRank = Option::getRankType(dp.total_acc);
    auto pScore = g_pScoreDB->getChartScoreBMS(gChartContext.hash);
    if (pScore)
    {
        State::set(IndexNumber::RESULT_RECORD_EX_NOW, (int)dp.score2);
        State::set(IndexNumber::RESULT_RECORD_EX_DIFF, (int)dp.score2 - pScore->exscore);
        State::set(IndexNumber::RESULT_RECORD_MAXCOMBO_NOW, (int)dp.maxCombo);
        State::set(IndexNumber::RESULT_RECORD_MAXCOMBO_DIFF, (int)dp.maxCombo - pScore->exscore);
        State::set(IndexNumber::RESULT_RECORD_BP_NOW, (int)dp.miss);
        State::set(IndexNumber::RESULT_RECORD_BP_DIFF, (int)dp.miss - pScore->bp);

        State::set(IndexNumber::RESULT_MYBEST_EX, pScore->exscore);
        State::set(IndexNumber::RESULT_MYBEST_DIFF, dp.score2 - pScore->exscore);
        State::set(IndexNumber::RESULT_MYBEST_RATE, (int)std::floor(pScore->rate));
        State::set(IndexNumber::RESULT_MYBEST_RATE_DECIMAL2, (int)std::floor(pScore->rate * 100.0) % 100);

        State::set(IndexNumber::RESULT_RECORD_EX_BEFORE, pScore->exscore);
        State::set(IndexNumber::RESULT_RECORD_MAXCOMBO_BEFORE, pScore->maxcombo);
        State::set(IndexNumber::RESULT_RECORD_BP_BEFORE, pScore->bp);
        State::set(IndexNumber::RESULT_RECORD_MYBEST_RATE, (int)std::floor(pScore->rate));
        State::set(IndexNumber::RESULT_RECORD_MYBEST_RATE_DECIMAL2, (int)std::floor(pScore->rate * 100.0) % 100);

        Option::e_rank_type recordRank = Option::getRankType(pScore->rate);
        State::set(IndexOption::RESULT_MYBEST_RANK, recordRank);
        //State::set(IndexOption::RESULT_UPDATED_RANK, (pScore->exscore > (int)dp.score2) ? recordRank : nowRank);
        State::set(IndexOption::RESULT_UPDATED_RANK, nowRank);

        if (pScore->exscore < dp.score2)    State::set(IndexSwitch::RESULT_UPDATED_SCORE, true);
        if (pScore->maxcombo < dp.maxCombo) State::set(IndexSwitch::RESULT_UPDATED_MAXCOMBO, true);
        if (pScore->bp > dp.miss)           State::set(IndexSwitch::RESULT_UPDATED_BP, true);
    }
    else
    {
        State::set(IndexNumber::RESULT_RECORD_EX_NOW, (int)dp.score2);
        State::set(IndexNumber::RESULT_RECORD_EX_DIFF, (int)dp.score2);
        State::set(IndexNumber::RESULT_RECORD_MAXCOMBO_NOW, (int)dp.maxCombo);
        State::set(IndexNumber::RESULT_RECORD_MAXCOMBO_DIFF, (int)dp.maxCombo);
        State::set(IndexNumber::RESULT_RECORD_BP_NOW, (int)dp.miss);
        State::set(IndexNumber::RESULT_RECORD_BP_DIFF, (int)dp.miss);

        State::set(IndexOption::RESULT_MYBEST_RANK, Option::RANK_NONE);
        State::set(IndexOption::RESULT_UPDATED_RANK, nowRank);

        State::set(IndexSwitch::RESULT_UPDATED_SCORE, true);
        State::set(IndexSwitch::RESULT_UPDATED_MAXCOMBO, true);
        State::set(IndexSwitch::RESULT_UPDATED_BP, true);
    }

    // TODO compare to target


    bool cleared = State::get(IndexSwitch::RESULT_CLEAR);

    switch (gPlayContext.mode)
    {
    case eMode::PLAY5_2:
    case eMode::PLAY7_2:
    case eMode::PLAY9_2:
    {
        auto d1p = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]->getData();
        auto d2p = gPlayContext.ruleset[PLAYER_SLOT_TARGET]->getData();

        // TODO WIN/LOSE
        /*
        switch (context_play.rulesetType)
        {
        case eRuleset::CLASSIC:
            if (d1p.score2 > d2p.score2)
                // TODO
                break;

        default:
            if (d1p.score > d2p.score)
                break;
        }
        */

        // clear or failed?
        //cleared = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]->isCleared() || gPlayContext.ruleset[PLAYER_SLOT_TARGET]->isCleared();
        break;
    }

    default:
        //cleared = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]->isCleared();
        break;
    }

    // Moved to play
    //State::set(IndexSwitch::RESULT_CLEAR, cleared);

    _pScoreOld = g_pScoreDB->getChartScoreBMS(gChartContext.hash);

    if (!gPlayContext.isReplay)
    {
        auto& [saveScoreTmp, saveLampTmp] = getSaveScoreType();
        saveScore = saveScoreTmp;
        switch (saveLampTmp)
        {
        case Option::e_lamp_type::LAMP_NOPLAY:      saveLamp = ScoreBMS::Lamp::NOPLAY; break;
        case Option::e_lamp_type::LAMP_FAILED:      saveLamp = ScoreBMS::Lamp::FAILED; break;
        case Option::e_lamp_type::LAMP_ASSIST:      saveLamp = ScoreBMS::Lamp::ASSIST; break;
        case Option::e_lamp_type::LAMP_EASY:        saveLamp = ScoreBMS::Lamp::EASY; break;
        case Option::e_lamp_type::LAMP_NORMAL:      saveLamp = ScoreBMS::Lamp::NORMAL; break;
        case Option::e_lamp_type::LAMP_HARD:        saveLamp = ScoreBMS::Lamp::HARD; break;
        case Option::e_lamp_type::LAMP_EXHARD:      saveLamp = ScoreBMS::Lamp::EXHARD; break;
        case Option::e_lamp_type::LAMP_FULLCOMBO:   saveLamp = ScoreBMS::Lamp::FULLCOMBO; break;
        case Option::e_lamp_type::LAMP_PERFECT:     saveLamp = ScoreBMS::Lamp::PERFECT; break;
        case Option::e_lamp_type::LAMP_MAX:         saveLamp = ScoreBMS::Lamp::MAX; break;
        default: assert(false); break;
        }
    }

    using namespace std::placeholders;
    _input.register_p("SCENE_PRESS", std::bind(&SceneResult::inputGamePress, this, _1, _2));
    _input.register_h("SCENE_HOLD", std::bind(&SceneResult::inputGameHold, this, _1, _2));
    _input.register_r("SCENE_RELEASE", std::bind(&SceneResult::inputGameRelease, this, _1, _2));

    Time t;
    State::set(IndexTimer::RESULT_GRAPH_START, t.norm());

    SoundMgr::stopSysSamples();
    if (cleared) 
        SoundMgr::playSysSample(SoundChannelType::BGM_SYS, eSoundSample::SOUND_CLEAR);
    else
        SoundMgr::playSysSample(SoundChannelType::BGM_SYS, eSoundSample::SOUND_FAIL);
}

////////////////////////////////////////////////////////////////////////////////

SceneResult::~SceneResult()
{
    _input.loopEnd();
    loopEnd();
}

void SceneResult::_updateAsync()
{
    if (gNextScene != eScene::RESULT) return;

    if (gAppIsExiting)
    {
        gNextScene = eScene::EXIT_TRANS;
    }

    std::unique_lock<decltype(_mutex)> _lock(_mutex, std::try_to_lock);
    if (!_lock.owns_lock()) return;

    switch (_state)
    {
    case eResultState::DRAW:
        updateDraw();
        break;
    case eResultState::STOP:
        updateStop();
        break;
    case eResultState::RECORD:
        updateRecord();
        break;
    case eResultState::FADEOUT:
        updateFadeout();
        break;
    }
}

void SceneResult::updateDraw()
{
    auto t = Time();
    auto rt = t - State::get(IndexTimer::SCENE_START);

    if (rt.norm() >= _skin->info.timeResultRank)
    {
        State::set(IndexTimer::RESULT_RANK_START, t.norm());
        // TODO play hit sound
        _state = eResultState::STOP;
        LOG_DEBUG << "[Result] State changed to STOP";
    }
}

void SceneResult::updateStop()
{
    auto t = Time();
    auto rt = t - State::get(IndexTimer::SCENE_START);
}

void SceneResult::updateRecord()
{
    auto t = Time();
    auto rt = t - State::get(IndexTimer::SCENE_START);

    // TODO sync score in online mode?
    if (true)
    {
        _scoreSyncFinished = true;
    }
}

void SceneResult::updateFadeout()
{
    auto t = Time();
    auto rt = t - State::get(IndexTimer::SCENE_START);
    auto ft = t - State::get(IndexTimer::FADEOUT_BEGIN);

    if (ft >= _skin->info.timeOutro)
    {
        SoundMgr::stopNoteSamples();

        std::string replayFileName = (boost::format("%04d%02d%02d-%02d%02d%02d.rep")
            % State::get(IndexNumber::DATE_YEAR)
            % State::get(IndexNumber::DATE_MON)
            % State::get(IndexNumber::DATE_DAY)
            % State::get(IndexNumber::DATE_HOUR)
            % State::get(IndexNumber::DATE_MIN)
            % State::get(IndexNumber::DATE_SEC)
            ).str();
        Path replayPath = ConfigMgr::Profile()->getPath() / "replay" / "chart" / gChartContext.hash.hexdigest() / replayFileName;

        // save replay
        if (saveScore)
        {
            gPlayContext.replayNew->saveFile(replayPath);
        }

        // save score
        if (saveScore && !gChartContext.hash.empty())
        {
            assert(gPlayContext.ruleset[PLAYER_SLOT_PLAYER] != nullptr);
            ScoreBMS score;
            auto& format = gChartContext.chartObj;
            auto& chart = gPlayContext.chartObj[PLAYER_SLOT_PLAYER];
            auto& ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER];
            auto& data = ruleset->getData();
            score.notes = chart->getNoteTotalCount();
            score.score = data.score;
            score.rate = data.total_acc;
            score.fast = data.fast;
            score.slow = data.slow;
            score.maxcombo = data.maxCombo;
            score.playcount = _pScoreOld ? _pScoreOld->playcount + 1 : 1;
            auto isclear = ruleset->isCleared() ? 1 : 0;
            score.clearcount = _pScoreOld ? _pScoreOld->clearcount + isclear : isclear;
            score.replayFileName = replayFileName;

            switch (format->type())
            {
            case eChartFormat::BMS:
            case eChartFormat::BMSON:
            {
                auto rBMS = std::reinterpret_pointer_cast<RulesetBMS>(ruleset);
                score.exscore = data.score2;

                score.lamp = ScoreBMS::Lamp::NOPLAY;
                if (rBMS->isCleared())
                {
                    if (rBMS->getMaxCombo() == rBMS->getData().maxCombo)
                    {
                        score.lamp = ScoreBMS::Lamp::FULLCOMBO;
                    }
                    else
                    {
                        if (gPlayContext.isCourse)
                        {
                        }
                        else
                        {
                            switch (rBMS->getGaugeType())
                            {
                            case RulesetBMS::GaugeType::GROOVE:  score.lamp = ScoreBMS::Lamp::NORMAL; break;
                            case RulesetBMS::GaugeType::EASY:    score.lamp = ScoreBMS::Lamp::EASY; break;
                            case RulesetBMS::GaugeType::ASSIST:  score.lamp = ScoreBMS::Lamp::ASSIST; break;
                            case RulesetBMS::GaugeType::HARD:    score.lamp = ScoreBMS::Lamp::HARD; break;
                            case RulesetBMS::GaugeType::EXHARD:  score.lamp = ScoreBMS::Lamp::EXHARD; break;
                            case RulesetBMS::GaugeType::DEATH:   score.lamp = ScoreBMS::Lamp::FULLCOMBO; break;
                            case RulesetBMS::GaugeType::P_ATK:   score.lamp = ScoreBMS::Lamp::EASY; break;
                            case RulesetBMS::GaugeType::G_ATK:   score.lamp = ScoreBMS::Lamp::EASY; break;
                            case RulesetBMS::GaugeType::GRADE:   score.lamp = ScoreBMS::Lamp::NOPLAY; break;
                            case RulesetBMS::GaugeType::EXGRADE: score.lamp = ScoreBMS::Lamp::NOPLAY; break;
                            default: break;
                            }
                        }
                    }
                }
                else
                {
                    score.lamp = ScoreBMS::Lamp::FAILED;
                }
                score.lamp = std::min(score.lamp, saveLamp);

                score.pgreat = rBMS->getJudgeCount(RulesetBMS::JudgeType::PERFECT);
                score.great = rBMS->getJudgeCount(RulesetBMS::JudgeType::GREAT);
                score.good = rBMS->getJudgeCount(RulesetBMS::JudgeType::GOOD);
                score.bad = rBMS->getJudgeCount(RulesetBMS::JudgeType::BAD);
                score.bpoor = rBMS->getJudgeCount(RulesetBMS::JudgeType::BPOOR);
                score.miss = rBMS->getJudgeCount(RulesetBMS::JudgeType::MISS);
                score.bp = score.bad + score.bpoor + score.miss;
                score.combobreak = rBMS->getJudgeCount(RulesetBMS::JudgeType::COMBOBREAK);
                g_pScoreDB->updateChartScoreBMS(gChartContext.hash, score);
                break;
            }
            default:
                break;
            }
        }
        
        // check retry
        if (_retryRequested && gPlayContext.canRetry)
        {
            clearContextPlayForRetry();
            gNextScene = eScene::PLAY;
        }
        else
        {
            clearContextPlay();
            gPlayContext.isAuto = false;
            gPlayContext.isReplay = false;
            gNextScene = gQuitOnFinish ? eScene::EXIT_TRANS : eScene::SELECT;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

// CALLBACK
void SceneResult::inputGamePress(InputMask& m, const Time& t)
{
    if (t - State::get(IndexTimer::SCENE_START) < _skin->info.timeIntro) return;

    if ((_inputAvailable & m & (INPUT_MASK_DECIDE | INPUT_MASK_CANCEL)).any() || m[Input::ESC])
    {
        switch (_state)
        {
        case eResultState::DRAW:
            State::set(IndexTimer::RESULT_RANK_START, t.norm());
            // TODO play hit sound
            _state = eResultState::STOP;
            LOG_DEBUG << "[Result] State changed to STOP";
            break;

        case eResultState::STOP:
            if (saveScore)
            {
                State::set(IndexTimer::RESULT_HIGHSCORE_START, t.norm());
                // TODO stop result sound
                // TODO play record sound
                _state = eResultState::RECORD;
                LOG_DEBUG << "[Result] State changed to RECORD";
            }
            else
            {
                State::set(IndexTimer::FADEOUT_BEGIN, t.norm());
                _state = eResultState::FADEOUT;
                SoundMgr::setSysVolume(0.0, 2000);
                LOG_DEBUG << "[Result] State changed to FADEOUT";
            }
            break;

        case eResultState::RECORD:
            if (_scoreSyncFinished)
            {
                State::set(IndexTimer::FADEOUT_BEGIN, t.norm());
                _state = eResultState::FADEOUT;
                SoundMgr::setSysVolume(0.0, 2000);
                LOG_DEBUG << "[Result] State changed to FADEOUT";
            }
            break;

        case eResultState::FADEOUT:
            break;

        default:
            break;
        }
    }
}

// CALLBACK
void SceneResult::inputGameHold(InputMask& m, const Time& t)
{
    if (t - State::get(IndexTimer::SCENE_START) < _skin->info.timeIntro) return;

    if (_state == eResultState::FADEOUT)
    {
        _retryRequested =
            (_inputAvailable & m & INPUT_MASK_DECIDE).any() && 
            (_inputAvailable & m & INPUT_MASK_CANCEL).any();
    }
}

// CALLBACK
void SceneResult::inputGameRelease(InputMask& m, const Time& t)
{
    if (t - State::get(IndexTimer::SCENE_START) < _skin->info.timeIntro) return;
}
