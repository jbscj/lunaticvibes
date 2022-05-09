#include "common/log.h"
#include "input_mgr.h"
#include "input_rawinput.h"
#include "common/sysutil.h"
#include "config/config_mgr.h"

InputMgr InputMgr::_inst;

using namespace Input;

void InputMgr::init()
{
    using namespace std::placeholders;
    addWMEventHandler(&Input::rawinput::RIMgr::WMMsgHandler);
    addWMEventHandler(&WMMouseWheelMsgHandler);
}

void InputMgr::updateDevices()
{
    // Check jostick connection status
    _inst.haveJoystick = false;
    //for (unsigned i = 0; i < sf::Joystick::Count; i++)
    //    if (sf::Joystick::isConnected(i))
    //    {
    //        _inst.joysticksConnected[i] = true;
    //        _inst.haveJoystick = true;
    //    }

    Input::rawinput::RIMgr::inst().refreshDevices();
}

void InputMgr::updateBindings(GameModeKeys keys, Pad K)
{
    // Clear current bindings
    for (auto& k : _inst.padBindings)
        std::for_each(k.begin(), k.end(), [&](KeyMap& m) { m.reset(); });

    switch (keys)
    {
    case 5:
    case 7:
    case 9:
        for (Input::Pad key = Input::S1L; key < Input::ESC; ++(*(int*)&key))
        {
            auto padBindings = ConfigMgr::Input(keys)->getBindings(key);
            std::copy_n(padBindings.begin(), std::min(MAX_BINDINGS_PER_KEY, padBindings.size()), _inst.padBindings[key].begin());
        }
        break;

    default: break;
    }

    LOG_INFO << "Key bindings updated";
}

#ifdef RENDER_SDL2
#include "SDL_mouse.h"
#endif

std::bitset<KEY_COUNT> InputMgr::detect()
{
    std::bitset<KEY_COUNT> res{};

#ifdef RAWINPUT_AVAILABLE
    // rawinput
    rawinput::RIMgr::inst().update();
    {
        std::shared_lock lock(rawinput::RIMgr::inst().mutexInput);
        for (int k = S1L; k < ESC; k++)
        {
            for (const KeyMap& b : _inst.padBindings[k])
            {
                if (b.getType() == KeyMap::DeviceType::RAWINPUT)
                {
                    if (b.isAxis())
                    {
                        if (_inst.axisMode == eAxisMode::AXIS_NORMAL)
                        {
                            double val = rawinput::RIMgr::inst().getAxis(b.getDeviceID(), b.getAxis());
                            if (b.getAxisDir() > 0 && val > 0 || b.getAxisDir() < 0 && val < 0)
                            {
                                int absVal = (int)std::abs(val);
                                if (absVal > _inst.analogDeadZone)
                                    res[k] = true;
                            }
                        }
                        else
                        {
                            // Handled by detectRelativeAxis, do not modify result set here
                        }
                    }
                    else
                    {
                        if (rawinput::RIMgr::inst().isPressed(b.getDeviceID(), b.getCode()))
                            res[k] = true;
                    }
                }
            }
        }
    }
#endif

    // game input
    for (int k = S1L; k < ESC; k++)
    {
        for (const KeyMap& b : _inst.padBindings[k])
        {
			switch (b.getType())
			{
            case KeyMap::DeviceType::KEYBOARD:
                if (isKeyPressed(b.getKeyboard()))
                    res[k] = true;
				break;
			case KeyMap::DeviceType::JOYSTICK:
                // TODO joystick
				break;
			case KeyMap::DeviceType::CONTROLLER:
			case KeyMap::DeviceType::MOUSE:
				break;
			}
			if (res[k]) break;
        }
    }

    // FN input
    res[ESC] = isKeyPressed(Keyboard::K_ESC);
    res[F1] = isKeyPressed(Keyboard::K_F1);
    res[F2] = isKeyPressed(Keyboard::K_F2);
    res[F3] = isKeyPressed(Keyboard::K_F3);
    res[F4] = isKeyPressed(Keyboard::K_F4);
    res[F5] = isKeyPressed(Keyboard::K_F5);
    res[F6] = isKeyPressed(Keyboard::K_F6);
    res[F7] = isKeyPressed(Keyboard::K_F7);
    res[F8] = isKeyPressed(Keyboard::K_F8);
    res[F9] = isKeyPressed(Keyboard::K_F9);
    res[F10] = isKeyPressed(Keyboard::K_F10);
    res[F11] = isKeyPressed(Keyboard::K_F11);
    res[F12] = isKeyPressed(Keyboard::K_F12);
    res[F13] = isKeyPressed(Keyboard::K_F13);
    res[F14] = isKeyPressed(Keyboard::K_F14);
    res[F15] = isKeyPressed(Keyboard::K_F15);
    res[UP] = isKeyPressed(Keyboard::K_UP);
    res[DOWN] = isKeyPressed(Keyboard::K_DOWN);
    res[LEFT] = isKeyPressed(Keyboard::K_LEFT);
    res[RIGHT] = isKeyPressed(Keyboard::K_RIGHT);
    res[INSERT] = isKeyPressed(Keyboard::K_INS);
    res[DEL] = isKeyPressed(Keyboard::K_DEL);
    res[HOME] = isKeyPressed(Keyboard::K_HOME);
    res[END] = isKeyPressed(Keyboard::K_END);
    res[PGUP] = isKeyPressed(Keyboard::K_PGUP);
    res[PGDN] = isKeyPressed(Keyboard::K_PGDN);
    res[RETURN] = isKeyPressed(Keyboard::K_ENTER);
    res[BACKSPACE] = isKeyPressed(Keyboard::K_BKSP);

    res[M1] = isMouseButtonPressed(1);
    res[M2] = isMouseButtonPressed(2);
    res[M3] = isMouseButtonPressed(3);
    res[M4] = isMouseButtonPressed(4);
    res[M5] = isMouseButtonPressed(5);

    auto mouseWheelState = getLastMouseWheelState();
    res[MWHEELUP] = mouseWheelState > 0;
    res[MWHEELDOWN] = mouseWheelState < 0;

    return res;
}

std::map<Input::Pad, std::pair<double, int>> InputMgr::detectRelativeAxis()
{
    std::map<Input::Pad, std::pair<double, int>> result;
    if (_inst.axisMode != eAxisMode::AXIS_RELATIVE) return result;

    for (int i = S1L; i < ESC; i++)
    {
        auto k = static_cast<Input::Pad>(i);
        for (const KeyMap& b : _inst.padBindings[k])
        {
#ifdef RAWINPUT_AVAILABLE
            if (b.getType() == KeyMap::DeviceType::RAWINPUT && b.isAxis())
            {
                auto [speed, time] = rawinput::RIMgr::inst().getAxisSpeed(b.getDeviceID(), b.getAxis());
                //double diff = speed * time;
                if (speed > 0 && b.getAxisDir() > 0)
                {
                    result[k].first += speed;
                    result[k].second = std::max(result[static_cast<Input::Pad>(k)].second, time);
                }
                if (speed < 0 && b.getAxisDir() < 0)
                {
                    result[k].first += -speed;
                    result[k].second = std::max(result[static_cast<Input::Pad>(k)].second, time);
                }
            }
#endif
        }
    }

    return result;
}


bool InputMgr::getMousePos(int& x, int& y)
{
    return getMouseCursorPos(x, y);
}
