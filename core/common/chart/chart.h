#pragma once
#include <regex>
#include <vector>
#include <numeric>
#include <memory>
#include "types.h"
#include "beat.h"

// Subset of chart formats.
// Currently including BMS

enum class eChartType
{
    UNKNOWN,

    BMS,
    BMSON,
};
inline const std::regex regexFileExtBMS{ R"(\.(bms|bme|bml|pms)$)", std::regex::icase };
inline const std::regex regexFileExtBMSON{ R"(\.(bmson)$)", std::regex::icase };
eChartType matchChartType(const Path& p);

class vChart
{
protected:
    eChartType _type = eChartType::UNKNOWN;
public:
    constexpr eChartType type() { return _type; }

public:
    vChart() = default;
    virtual ~vChart() = default;
    static std::shared_ptr<vChart> getFromFile(const Path& path);

protected:
    Path _absolutePath;
    bool _loaded = false;
public:
    constexpr bool isLoaded() { return _loaded; }

// following fields are generic info, which are stored in db
public:
    Path _filePath;
    HashMD5 _fileHash;
    HashMD5 _folderHash;

    StringContent _title;
    StringContent _title2;
    StringContent _artist;
    StringContent _artist2;
    StringContent _genre;
    StringContent _version;     // mostly known as difficulty name
    double _level;

    int _totalLength_sec;
    int _totalnotes;

    StringContent _BG;
    StringContent _banner;

    StringContent _text1;
    StringContent _text2;
    StringContent _text3;

    BPM _minBPM = INFINITY;
    BPM _maxBPM = 0.0;
    BPM _itlBPM = 130.0;

// following fields are filled during loading
public:
    std::vector<StringContent> _wavFiles;
    std::vector<StringContent> _bgaFiles;

    std::vector<Note> _BPMs;
    std::vector<Beat> _measureLength;
    std::vector<Note> _notes;
    //std::vector<_Inherit_SpriteStatic_with_playbegin_timer_> _BGAsprites;

public:
    BPM getAvgBPM() const;
    BPM getNormBPM() const;
    Path getDirectory() const;

    virtual int getExtendedProperty(const std::string& key, void* ret) = 0;
};
