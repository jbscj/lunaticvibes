#pragma once
#include "scene.h"

class SceneExitTrans : public SceneBase
{
public:
    SceneExitTrans();
    virtual ~SceneExitTrans() = default;

protected:
    // Looper callbacks
    virtual void _updateAsync() override {}
};