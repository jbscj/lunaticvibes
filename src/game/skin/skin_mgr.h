#pragma once
#include <array>
#include <memory>
#include "skin.h"

typedef std::shared_ptr<vSkin> pSkin;

class SkinMgr
{
private:
    static SkinMgr _inst;
    SkinMgr() = default;
    ~SkinMgr() = default;
public:
    SkinMgr(SkinMgr&) = delete;
    SkinMgr& operator= (SkinMgr&) = delete;

protected:
    std::array<pSkin, static_cast<size_t>(eMode::MODE_COUNT)> c{};

public:
    static void load(eMode);
    static void unload(eMode);
    static pSkin get(eMode);
};