#pragma once
#include <vector>
#include "common/types.h"
#include <cereal/access.hpp>
#include <cereal/types/vector.hpp>

template <class Archive, size_t bytes>
void serialize(Archive& ar, Hash<bytes>& hash)
{
    unsigned char* hashBytes = const_cast<unsigned char*>(hash.hex());
    for (size_t i = 0; i < bytes; ++i)
        ar(hashBytes[i]);
}

class ReplayChart
{
public:
    struct Commands
    {
        int64_t ms = -1;
        enum class Type: uint32_t
        {
            UNDEF,
            HISPEED,
            LANECOVER_TOP,
            LANECOVER_BOTTOM,

            S1L_DOWN,
            S1R_DOWN,
            K11_DOWN,
            K12_DOWN,
            K13_DOWN,
            K14_DOWN,
            K15_DOWN,
            K16_DOWN,
            K17_DOWN,
            K18_DOWN,
            K19_DOWN,
            K1START_DOWN,
            K1SELECT_DOWN,
            S2L_DOWN,
            S2R_DOWN,
            K21_DOWN,
            K22_DOWN,
            K23_DOWN,
            K24_DOWN,
            K25_DOWN,
            K26_DOWN,
            K27_DOWN,
            K28_DOWN,
            K29_DOWN,
            K2START_DOWN,
            K2SELECT_DOWN,

            S1L_UP,
            S1R_UP,
            K11_UP,
            K12_UP,
            K13_UP,
            K14_UP,
            K15_UP,
            K16_UP,
            K17_UP,
            K18_UP,
            K19_UP,
            K1START_UP,
            K1SELECT_UP,
            S2L_UP,
            S2R_UP,
            K21_UP,
            K22_UP,
            K23_UP,
            K24_UP,
            K25_UP,
            K26_UP,
            K27_UP,
            K28_UP,
            K29_UP,
            K2START_UP,
            K2SELECT_UP,

            S1A_PLUS,
            S1A_MINUS,
            S1A_STOP,

            S2A_PLUS,
            S2A_MINUS,
            S2A_STOP,

            JUDGE_LEFT_EXACT_0,  // PGREAT
            JUDGE_LEFT_EARLY_0,  // PGREAT
            JUDGE_LEFT_EARLY_1,  // GREAT
            JUDGE_LEFT_EARLY_2,  // GOOD
            JUDGE_LEFT_EARLY_3,  // BAD
            JUDGE_LEFT_EARLY_4,  // POOR
            JUDGE_LEFT_EARLY_5,  // BPOOR
            JUDGE_LEFT_EARLY_6,
            JUDGE_LEFT_EARLY_7,
            JUDGE_LEFT_EARLY_8,
            JUDGE_LEFT_EARLY_9,
            JUDGE_LEFT_LATE_0,   // PGREAT
            JUDGE_LEFT_LATE_1,   // GREAT
            JUDGE_LEFT_LATE_2,   // GOOD
            JUDGE_LEFT_LATE_3,   // BAD
            JUDGE_LEFT_LATE_4,   // POOR
            JUDGE_LEFT_LATE_5,   // BPOOR
            JUDGE_LEFT_LATE_6,
            JUDGE_LEFT_LATE_7,
            JUDGE_LEFT_LATE_8,
            JUDGE_LEFT_LATE_9,

            JUDGE_RIGHT_EXACT_0,  // PGREAT
            JUDGE_RIGHT_EARLY_0,  // PGREAT
            JUDGE_RIGHT_EARLY_1,  // GREAT
            JUDGE_RIGHT_EARLY_2,  // GOOD
            JUDGE_RIGHT_EARLY_3,  // BAD
            JUDGE_RIGHT_EARLY_4,  // POOR
            JUDGE_RIGHT_EARLY_5,  // BPOOR
            JUDGE_RIGHT_EARLY_6,
            JUDGE_RIGHT_EARLY_7,
            JUDGE_RIGHT_EARLY_8,
            JUDGE_RIGHT_EARLY_9,
            JUDGE_RIGHT_LATE_0,   // PGREAT
            JUDGE_RIGHT_LATE_1,   // GREAT
            JUDGE_RIGHT_LATE_2,   // GOOD
            JUDGE_RIGHT_LATE_3,   // BAD
            JUDGE_RIGHT_LATE_4,   // POOR
            JUDGE_RIGHT_LATE_5,   // BPOOR
            JUDGE_RIGHT_LATE_6,
            JUDGE_RIGHT_LATE_7,
            JUDGE_RIGHT_LATE_8,
            JUDGE_RIGHT_LATE_9,

            JUDGE_LEFT_LANDMINE,
            JUDGE_RIGHT_LANDMINE,

        } type = Type::UNDEF;

        template<class Archive>
        void serialize(Archive& ar)
        {
            ar(ms);
            ar(type);
        }
    };

private:
    uint32_t    checksum = 0;           // md5(serirization of this, checksum=0)

public:
    HashMD5     chartHash;
    uint64_t    randomSeed = 0;

    eModGauge   gaugeType = eModGauge::NORMAL;  
    eModRandom  randomTypeLeft = eModRandom::NONE; 
    eModRandom  randomTypeRight = eModRandom::NONE; 
    int8_t      laneEffectType = 0;     // OFF/HID/SUD/SUDHID/LIFT/LIFTSUD
    int8_t      pitchType = 0;          // FREQ/PITCH/SPEED
    int8_t      pitchValue = 0;         // -12 ~ +12 (value below 0 may invalid)
    uint8_t     assistMask = 0;
    eModHs      hispeedFix = eModHs::NONE;

    bool        DPFlip = false;
    bool        DPBattle = false;

    std::vector<Commands> commands;

public:
    ReplayChart() = default;
    virtual ~ReplayChart() = default;

private:
    friend class cereal::access;

    template<class Archive>
    void serialize(Archive& ar, const std::uint32_t version)
    {
        ar(checksum);
        ar(chartHash);
        ar(randomSeed);
        ar(gaugeType);
        ar(randomTypeLeft);
        ar(randomTypeRight);
        ar(laneEffectType);
        ar(pitchType);
        ar(pitchValue);
        ar(assistMask);
        ar(hispeedFix);
        ar(DPFlip);
        ar(DPBattle);
        ar(commands);

        // more parameters goes below
        if (version >= 2)
        {

        }
    }
    void generateChecksum();


public:
    bool loadFile(const Path& path);
    bool saveFile(const Path& path);
    bool validate();

    static Path getReplayPath(const HashMD5& chartMD5);
    Path getReplayPath();
};

CEREAL_CLASS_VERSION(ReplayChart, 1);
