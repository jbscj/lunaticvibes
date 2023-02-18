#include "chartformat_bmson.h"
#include "tao/json.hpp"

ChartFormatBMSON::ChartFormatBMSON(const Path& file, uint64_t randomSeed)
{
    initWithFile(file, randomSeed);
}

int ChartFormatBMSON::initWithFile(const Path& file, uint64_t randomSeed)
{
    if (_loaded)
    {
        return 1;
    }

    filePath = file.filename();
    absolutePath = std::filesystem::absolute(file);
    LOG_DEBUG << "[BMSON] " << absolutePath.u8string();
    std::ifstream ifsFile(absolutePath.c_str());
    if (ifsFile.fail())
    {
        //errorCode = err::FILE_ERROR;
        //errorLine = 0;
        LOG_WARNING << "[BMSON] " << absolutePath.u8string() << " File ERROR";
        return 1;
    }

    const auto fileSize = std::filesystem::file_size(file);
    std::string fileContent(fileSize, '\0');
    ifsFile.read(fileContent.data(), fileSize);
    ifsFile.close();

    std::string_view fcView = fileContent;

    tao::json::value bmsonJson;
    try
    {
        bmsonJson = tao::json::from_file(absolutePath);
    }
    catch (std::exception& e)
    {
        LOG_WARNING << "[BMSON] " << absolutePath.u8string() << " json error: " << e.what();
        throw;
    }

    // parse flags
    bool unspecifiedBarlines = false;
    unsigned long lastNotePulse = 0;

    try
    {
        fileVersion = bmsonJson["version"].get_string();

        tao::json::value info = bmsonJson["info"];
        if (!info.is_object())
        {
            LOG_WARNING << "[BMSON] " << absolutePath.u8string() << " no info field found";
            throw;
        }

        auto fr = info.find("title");
        title = fr && fr->is_string_type() ? fr->get_string() : "";
        fr = info.find("subtitle"); // allows '\n' splitter
        title2 = fr && fr->is_string_type() ? fr->get_string() : "";

        fr = info.find("artist");
        artist = fr && fr->is_string_type() ? fr->get_string() : "";
        fr = info.find("subartist"); // [key:value]
        if (fr && fr->is_array())
        {
            for (auto kv : fr->get_array())
            {
                if (kv.is_string_type())
                {
                    subartists.push_back(kv.get_string());
                }
            }
        }

        fr = info.find("genre");
        genre = fr && fr->is_string_type() ? fr->get_string() : "";
        fr = info.find("mode_hint"); // default "beat-7k"
        modeHint = fr && fr->is_string_type() ? fr->get_string() : modeHint;
        fr = info.find("chart_name");
        version = fr && fr->is_string_type() ? fr->get_string() : "";
        fr = info.find("level");
        playLevel = fr && fr->is_integer() ? fr->as<int>() : playLevel;
        fr = info.find("init_bpm");
        if (fr->is_null())
        {
            // It is a fatal error if init_bpm is unspecified.
            LOG_WARNING << "[BMSON] " << absolutePath.u8string() << " no init_bpm specified";
            throw;
        }
        startBPM = fr && fr->is_double() ? fr->as<double>() : startBPM;
        fr = info.find("judge_rank"); // negative case? The implementation depends of each player.
        judgeRank = fr && fr->is_double() ? fr->as<double>() : judgeRank;
        fr = info.find("total");
        total = fr && fr->is_double() ? std::abs(fr->as<double>()) : total;

        fr = info.find("back_image"); // displayed during gameplay
        stagefile = fr && fr->is_string_type() ? fr->get_string() : "";
        fr = info.find("title_image"); // displayed before song starts
        backbmp = fr && fr->is_string_type() ? fr->get_string() : "";
        fr = info.find("banner_image"); // displayed in song selection or result screen
        banner = fr && fr->is_string_type() ? fr->get_string() : "";
        fr = info.find("eyecatch_image"); // displayed during song loading
        eyecatchImg = fr && fr->is_string_type() ? fr->get_string() : "";
        fr = info.find("preview_music");
        previewMusic = fr && fr->is_string_type() ? fr->get_string() : "";
        fr = info.find("resolution"); // default 240
        if (fr && fr->is_integer() && fr->as<int>() != 0)
            resolution = std::abs(fr->as<int>());

        tao::json::value res = bmsonJson["lines"];
        if (res && res.is_array())
        {
            for (auto e : res.get_array())
            {
                fr = e.find("y");
                if (fr && fr->is_number())
                {
                    lines.push_back(fr->as<unsigned long>());
                }
            }
        }
        else
        {
            unspecifiedBarlines = true;
        }

        res = bmsonJson["bpm_events"];
        if (res && res.is_array())
        {
            for (auto e : res.get_array())
            {
                int itemCount = 0;
                fr = e.find("y");
                unsigned long y;
                if (fr && fr->is_number())
                {
                    y = fr->as<unsigned long>();
                    itemCount++;
                }
                fr = e.find("bpm");
                double bpm;
                if (fr && fr->is_number())
                {
                    bpm = fr->as<double>();
                    itemCount++;
                }
                if (itemCount == 2)
                {
                    // later entries overwrite previous
                    bpmEvents[y] = bpm;
                }
            }
        }

        res = bmsonJson["stop_events"];
        if (res && res.is_array())
        {
            for (auto e : res.get_array())
            {
                int itemCount = 0;
                fr = e.find("y");
                unsigned long y;
                if (fr && fr->is_number())
                {
                    y = fr->as<unsigned long>();
                    itemCount++;
                }
                fr = e.find("duration");
                double dur;
                if (fr && fr->is_number())
                {
                    dur = fr->as<double>();
                    itemCount++;
                }
                if (itemCount == 2)
                {
                    // later entries add to previous
                    if (stopEvents.find(y) != stopEvents.end())
                        stopEvents[y] += dur;
                    else
                        stopEvents[y] = dur;
                }
            }
        }

        res = bmsonJson["sound_channels"];
        if (res && res.is_array())
        {
            for (auto e : res.get_array())
            {
                int itemCount = 0;
                fr = e.find("name");
                std::string name;
                if (fr && fr->is_string_type())
                {
                    name = fr->get_string();
                    itemCount++;
                }
                fr = e.find("notes");
                std::list<bmson::ParseNote> notes(0);
                if (fr && fr->is_array())
                {
                    tao::json::value* notefr;
                    for (auto n : fr->get_array())
                    {
                        int noteItemCount = 0;
                        int x = 0;
                        unsigned long y, l;
                        bool c;
                        notefr = n.find("x");
                        if (!notefr->is_null() && notefr->is_number())
                        {
                            x = notefr->as<unsigned long>();
                        }
                        notefr = n.find("y");
                        if (notefr && notefr->is_number())
                        {
                            y = notefr->as<unsigned long>();
                            noteItemCount++;
                        }
                        notefr = n.find("l");
                        if (notefr && notefr->is_number())
                        {
                            l = notefr->as<unsigned long>();
                            noteItemCount++;
                        }
                        notefr = n.find("c");
                        if (notefr && notefr->is_boolean())
                        {
                            c = notefr->as<bool>();
                            noteItemCount++;
                        }
                        if (noteItemCount == 3)
                        {
                            bmson::ParseNote note = { x, y, l, c };
                            lastNotePulse = y + l;
                            notes.push_back(note);
                        }
                    }
                    if (notes.size() > 0)
                    {
                        itemCount++;
                    }
                }
                if (itemCount == 2)
                {
                    SoundChannel sc = { name, notes };
                    soundChannels.push_back(sc);
                }
            }
        }

        // TODO: bga

        if (unspecifiedBarlines)
        {
            // auto generate barline every 4 beats
            unsigned long curPulse = 0;
            unsigned long step = resolution * 4;
            while (curPulse <= lastNotePulse)
            {
                lines.push_back(curPulse);
                curPulse += step;
            }
        }

        ;
    }
    catch (std::bad_variant_access& e)
    {
        LOG_WARNING << "[BMSON] " << absolutePath.u8string() << " parse error: " << e.what();
        throw;
    }
}