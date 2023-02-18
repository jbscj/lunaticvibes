#pragma once

namespace bmson
{
    enum GameMode {
        MODE_5KEYS,
        MODE_7KEYS,
        MODE_9KEYS,
        MODE_10KEYS,
        MODE_14KEYS,
    };
    enum class ErrorCode
    {
        OK = 0,
        FILE_ERROR = 1,
        ALREADY_INITIALIZED,
        VALUE_ERROR,
        TYPE_MISMATCH,
        NOTE_LINE_ERROR,
    };

    struct BarLine
    {
        unsigned long y;
    };
    struct ParseNote
    {
        int x = -1;
        unsigned long y;
        unsigned long l;
        bool c;
    };
    struct SoundChannel
    {
        StringContent name;
        std::list<ParseNote> notes;
    };

    struct BpmEvent
    {
        unsigned long y;
        double bpm;
    };
    struct StopEvent
    {
        unsigned long y;
        unsigned long duration;
    };
}

using namespace bmson;

class ChartFormatBMSONMeta : public ChartFormatBase
{
public:
    StringContent fileVersion;
    // subartists in format [key:value]
    std::list<StringContent> subartists;
    // additional info fields
    StringContent modeHint = "beat-7k";
    double judgeRank = 100;
    double total = 100;
    double bpm;

    // back_img -> stagefile
    // title_img -> backbmp
    // banner_img -> banner
    StringContent eyecatchImg;

    StringContent previewMusic;

    ChartFormatBMSONMeta() { _type = eChartFormat::BMSON; }
    virtual ~ChartFormatBMSONMeta() = default;
};

class ChartFormatBMSON : public ChartFormatBMSONMeta
{
protected:
    unsigned long resolution = 240;
    std::list<unsigned long> lines;
    std::map<unsigned long, BPM> bpmEvents;
    std::map<unsigned long, double> stopEvents;
    std::list<SoundChannel> soundChannels;

public:
    ChartFormatBMSON();
    ChartFormatBMSON(const Path& absolutePath, uint64_t randomSeed = 0);
    virtual ~ChartFormatBMSON() = default;

protected:
    int initWithFile(const Path& file, uint64_t randomSeed = 0);
};