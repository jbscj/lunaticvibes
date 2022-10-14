#include "scene_mgr.h"
#include "scene_context.h"
#include "scene_pre_select.h"
#include "scene_select.h"
#include "scene_decide.h"
#include "scene_play.h"
#include "scene_result.h"
#include "scene_play_retry_trans.h"
#include "scene_play_course_trans.h"
#include "scene_keyconfig.h"
#include "scene_customize.h"
#include "scene_exit_trans.h"
#include "game/skin/skin_mgr.h"
#include "game/ruleset/ruleset.h"

SceneMgr SceneMgr::_inst;

pScene SceneMgr::get(eScene e)
{
    State::set(IndexTimer::SCENE_START, TIMER_NEVER);
    State::set(IndexTimer::START_INPUT, TIMER_NEVER);
    State::set(IndexTimer::_LOAD_START, TIMER_NEVER);
    State::set(IndexTimer::PLAY_READY, TIMER_NEVER);
    State::set(IndexTimer::PLAY_START, TIMER_NEVER);

	pScene ps = nullptr;
    switch (e)
    {
    case eScene::EXIT:
    case eScene::NOT_INIT:
        break;

    case eScene::PRE_SELECT:
        ps = std::make_shared<ScenePreSelect>();
        break;

    case eScene::SELECT:
        ps = std::make_shared<SceneSelect>();
        break;

    case eScene::DECIDE:
        ps = std::make_shared<SceneDecide>();
        break;

    case eScene::PLAY:
        switch (gPlayContext.mode)
        {
        case eMode::PLAY5:
        case eMode::PLAY7:
        case eMode::PLAY9:
        case eMode::PLAY10:
        case eMode::PLAY14:
        case eMode::PLAY5_2:
        case eMode::PLAY7_2:
        case eMode::PLAY9_2:
            ps = std::make_shared<ScenePlay>();
            break;

        default:
            LOG_ERROR << "[Scene] Invalid mode: " << int(gPlayContext.mode);
            return nullptr;
        }
        break;

    case eScene::RETRY_TRANS:
        ps = std::make_shared<ScenePlayRetryTrans>();
        break;

    case eScene::RESULT:
        switch (gPlayContext.mode)
        {
        case eMode::PLAY5:
        case eMode::PLAY7:
        case eMode::PLAY9:
        case eMode::PLAY10:
        case eMode::PLAY14:
        case eMode::PLAY5_2:
        case eMode::PLAY7_2:
        case eMode::PLAY9_2:
            ps = std::make_shared<SceneResult>();
            break;

        default:
            LOG_ERROR << "[Scene] Invalid mode: " << int(gPlayContext.mode);
            return nullptr;
        }
        break;

    case eScene::KEYCONFIG:
        ps = std::make_shared<SceneKeyConfig>();
        break;

    case eScene::CUSTOMIZE:
        ps = std::make_shared<SceneCustomize>();
        break;

    case eScene::EXIT_TRANS:
        ps = std::make_shared<SceneExitTrans>();
        break;

	default:
		return nullptr;
    }

    Time t;
    State::set(IndexTimer::SCENE_START, t.norm());
    State::set(IndexTimer::START_INPUT, t.norm() + (ps ? ps->getSkinInfo().timeIntro : 0));

	return ps;
}

void SceneMgr::clean()
{
	SkinMgr::clean();
}