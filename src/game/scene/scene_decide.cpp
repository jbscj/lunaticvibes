#include "scene_decide.h"
#include "scene_context.h"

#include "game/sound/sound_mgr.h"
#include "game/sound/sound_sample.h"

SceneDecide::SceneDecide() : vScene(eMode::DECIDE, 1000)
{
    _scene = eScene::DECIDE;

    _inputAvailable = INPUT_MASK_FUNC;
    _inputAvailable |= INPUT_MASK_1P | INPUT_MASK_2P;

    using namespace std::placeholders;
    _input.register_p("SCENE_PRESS", std::bind(&SceneDecide::inputGamePress, this, _1, _2));
    _input.register_h("SCENE_HOLD", std::bind(&SceneDecide::inputGameHold, this, _1, _2));
    _input.register_r("SCENE_RELEASE", std::bind(&SceneDecide::inputGameRelease, this, _1, _2));

    _state = eDecideState::START;
    _updateCallback = std::bind(&SceneDecide::updateStart, this);

    SoundMgr::stopSysSamples();
    SoundMgr::setSysVolume(1.0);
    SoundMgr::playSysSample(SoundChannelType::BGM_SYS, eSoundSample::BGM_DECIDE);
}

SceneDecide::~SceneDecide()
{
    _input.loopEnd();
    loopEnd();
}

////////////////////////////////////////////////////////////////////////////////

void SceneDecide::_updateAsync()
{
    if (gNextScene != eScene::DECIDE) return;

    if (gAppIsExiting)
    {
        gNextScene = eScene::EXIT_TRANS;
    }

    _updateCallback();
}

void SceneDecide::updateStart()
{
    auto t = Time();
    auto rt = t - State::get(IndexTimer::SCENE_START);

    if (rt.norm() >= _skin->info.timeDecideExpiry)
    {
        gNextScene = eScene::PLAY;
    }
}

void SceneDecide::updateSkip()
{
    auto t = Time();
    auto rt = t - State::get(IndexTimer::SCENE_START);
    auto ft = t - State::get(IndexTimer::FADEOUT_BEGIN);

    if (ft.norm() >= _skin->info.timeOutro)
    {
        gNextScene = eScene::PLAY;
    }
}

void SceneDecide::updateCancel()
{
    auto t = Time();
    auto rt = t - State::get(IndexTimer::SCENE_START);
    auto ft = t - State::get(IndexTimer::FADEOUT_BEGIN);

    if (ft.norm() >= _skin->info.timeOutro)
    {
        clearContextPlay();
        gPlayContext.isAuto = false;
        gPlayContext.isReplay = false;
        gNextScene = eScene::SELECT;
    }
}

////////////////////////////////////////////////////////////////////////////////

// CALLBACK
void SceneDecide::inputGamePress(InputMask& m, Time t)
{
    unsigned rt = (t - State::get(IndexTimer::SCENE_START)).norm();
    if (rt < _skin->info.timeIntro) return;

    auto k = _inputAvailable & m;
    if ((k & INPUT_MASK_DECIDE).any() && rt >= _skin->info.timeDecideSkip)
    {
        switch (_state)
        {
        case eDecideState::START:
            State::set(IndexTimer::FADEOUT_BEGIN, t.norm());
            _updateCallback = std::bind(&SceneDecide::updateSkip, this);
            _state = eDecideState::SKIP;
            LOG_DEBUG << "[Decide] State changed to SKIP";
            break;

        default:
            break;
        }
    }

    if (k[Input::ESC])
    {
        switch (_state)
        {
        case eDecideState::START:
            State::set(IndexTimer::FADEOUT_BEGIN, t.norm());
            SoundMgr::stopSysSamples();
            _updateCallback = std::bind(&SceneDecide::updateCancel, this);
            _state = eDecideState::CANCEL;
            LOG_DEBUG << "[Decide] State changed to CANCEL";
            break;

        default:
            break;
        }
    }

    if ((k[Input::K1START] && k[Input::K1SELECT]) || (k[Input::K2START] && k[Input::K2SELECT]))
    {
        // start hold timer
    }
}

// CALLBACK
void SceneDecide::inputGameHold(InputMask& m, Time t)
{
    unsigned rt = (t - State::get(IndexTimer::SCENE_START)).norm();
    if (rt < _skin->info.timeIntro) return;

    auto k = _inputAvailable & m;
    if ((k[Input::K1START] && k[Input::K1SELECT]) || (k[Input::K2START] && k[Input::K2SELECT]))
    {
        // check hold timer

        switch (_state)
        {
        case eDecideState::START:
            State::set(IndexTimer::FADEOUT_BEGIN, t.norm());
            SoundMgr::stopSysSamples();
            _updateCallback = std::bind(&SceneDecide::updateCancel, this);
            _state = eDecideState::CANCEL;
            LOG_DEBUG << "[Decide] State changed to CANCEL";
            break;

        default:
            break;
        }
    }
}

// CALLBACK
void SceneDecide::inputGameRelease(InputMask& m, Time t)
{
    unsigned rt = (t - State::get(IndexTimer::SCENE_START)).norm();
    if (rt < _skin->info.timeIntro) return;
}
