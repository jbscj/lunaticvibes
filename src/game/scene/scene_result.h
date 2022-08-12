#pragma once
#include <mutex>
#include "scene.h"

class vScore;
enum class eResultState
{
    DRAW,
    STOP,
    RECORD,
    FADEOUT,
};

class SceneResult : public vScene
{
private:
    std::mutex _mutex;
    eResultState _state;
    InputMask _inputAvailable;

protected:
    bool _scoreSyncFinished = false;
    bool _retryRequested = false;
    std::shared_ptr<vScore> _pScoreOld;

    bool saveScore = false;
    ScoreBMS::Lamp saveLamp;

public:
    SceneResult();
    virtual ~SceneResult() = default;

protected:
    // Looper callbacks
    virtual void _updateAsync() override;
    void updateDraw();
    void updateStop();
    void updateRecord();
    void updateFadeout();

protected:
    // Register to InputWrapper: judge / keysound
    void inputGamePress(InputMask&, const Time&);
    void inputGameHold(InputMask&, const Time&);
    void inputGameRelease(InputMask&, const Time&);
};