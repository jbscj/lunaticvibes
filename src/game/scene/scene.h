#pragma once
#include <bitset>
#include <string>
#include <memory>
#include <array>
#include "game/skin/skin.h"
#include "game/data/data.h"
#include "game/input/input_wrapper.h"
#include "common/types.h"

// Parent class of scenes, defines how an object being stored and drawn.
// Every classes of scenes should inherit this class.
class vScene: public AsyncLooper
{
protected:
    std::shared_ptr<vSkin> _skin;
    InputWrapper _input;


public:
	bool sceneEnding = false;

public:
    vScene(eMode mode, unsigned rate = 240, bool backgroundInput = false);
    virtual ~vScene();

public:
    vScene() = delete;
    virtual void update();      // skin update
    void update_mouse(InputMask& m, Time t);
    virtual void draw() const;

protected:
    virtual void _updateAsync() = 0;
};
