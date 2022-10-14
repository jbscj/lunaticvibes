#include "sprite_graph.h"
#include "game/scene/scene_context.h"

const size_t RT_GRAPH_THRESHOLD = 50;

SpriteLine::SpriteLine(int player, LineType ltype, int field_w, int field_h, int start, int end, int width, Color color) :
    SpriteStatic(nullptr), _player(player), _ltype(ltype), _field_w(field_w), _field_h(field_h), _start(start), _end(end), _line(width), _color(color)
{
    _type = SpriteTypes::LINE;
}

void SpriteLine::appendPoint(const ColorPoint& c) { _points.push_back(c); }

void SpriteLine::draw() const
{
    if (!_hide)
    {
        for (auto& [p1, p2] : _rects)
        {
            _line.draw(p1, p2, _color * _current.color);
        }
    }
}

void SpriteLine::updateProgress(const Time& t)
{
    int duration = _end - _start;
    if (duration > 0)
    {
        long long rt = t.norm() - State::get(_triggerTimer);
        if (rt >= _start)
        {
            _progress = (double)(rt - _start) / duration;
            _progress = std::clamp(_progress, 0.0, 1.0);
        }
        else
        {
            _progress = 0.0;
        }
    }
    else
    {
        _progress = 1.0;
    }
}

void SpriteLine::updateRects()
{
    /*
    for (size_t i = 0; i < _rects.size(); ++i)
    {
        const auto& r = _current.rect;
        _rects[i].x = r.x + r.w * _points[i].xf;
        _rects[i].y = r.y - r.h * _points[i].yf;
    }
    */

    if (!gPlayContext.ruleset[_player]) return;

    auto pushRects = [this](int size, const std::vector<int>& points, unsigned maxh, std::function<bool(int val1, int val2)> cond = [](int, int) { return true; })
    {
        std::vector<std::pair<Point, Point>> tmp;
        const auto& r = _current.rect;
        size_t region = static_cast<size_t>(std::floor(size * _progress));
        if (region == 0) return;
        region--;

        if (size > RT_GRAPH_THRESHOLD)
        {
            for (size_t i = 0; i < region; ++i)
            {
                if (cond(points[i], points[i + 1]))
                {
                    tmp.push_back({
                        {
                            r.x + _field_w * (double(i) / (size - 1)),
                            r.y - _field_h * (double(points[i]) / maxh)
                        },
                        {
                            r.x + _field_w * (double(i + 1) / (size - 1)),
                            r.y - _field_h * (double(points[i + 1]) / maxh)
                        }
                        });
                }
            }
        }
        else
        {
            for (size_t i = 0; i < region; ++i)
            {
                if (cond(points[i], points[i + 1]))
                {
                    tmp.push_back({
                        {
                            r.x + _field_w * (double(i) / (size - 1)),
                            r.y - _field_h * (double(points[i]) / maxh)
                        },
                        {
                            r.x + _field_w * (double(i) / (size - 1)),
                            r.y - _field_h * (double(points[i + 1]) / maxh)
                        }
                        });
                    tmp.push_back({
                        {
                            r.x + _field_w * (double(i) / (size - 1)),
                            r.y - _field_h * (double(points[i + 1]) / maxh)
                        },
                        {
                            r.x + _field_w * (double(i + 1) / (size - 1)),
                            r.y - _field_h * (double(points[i + 1]) / maxh)
                        }
                        });
                }
            }
        }

        _rects.clear();
        for (auto& [p1, p2] : tmp)
        {
            if (int(p1.x) != int(p2.x) || int(p1.y) != int(p2.y))
                _rects.push_back({ p1, p2 });
        }
    };


    int h = gPlayContext.ruleset[_player]->getClearHealth() * 100;
    switch (_ltype)
    {
    case LineType::GAUGE_F:
    {
        auto p = gPlayContext.graphGauge[_player];
        size_t s = p.size();
        pushRects(s, p, 100.0, [h](int val1, int val2) {return (val1 <= h && val2 <= h); });
        break;
    }

    case LineType::GAUGE_C:
    {
        auto p = gPlayContext.graphGauge[_player];
        size_t s = p.size();
        pushRects(s, p, 100.0, [h](int val1, int val2) {return (val1 >= h && val2 >= h); });
        break;
    }

    case LineType::SCORE:
    {
        auto p = gPlayContext.graphScore[_player];
        size_t s = p.size();
        pushRects(s, p, gPlayContext.ruleset[_player]->getMaxScore());
        break;
    }

    case LineType::SCORE_MYBEST:
    {
        if (gPlayContext.ruleset[PLAYER_SLOT_MYBEST])
        {
            auto p = gPlayContext.graphScore[PLAYER_SLOT_MYBEST];
            size_t s = p.size();
            pushRects(s, p, gPlayContext.ruleset[PLAYER_SLOT_MYBEST]->getMaxScore());
        }
        break;
    }

    case LineType::SCORE_TARGET:
    {
        auto pt = gPlayContext.graphScore[PLAYER_SLOT_TARGET];
        size_t s = pt.size();
        pushRects(s, pt, gPlayContext.ruleset[PLAYER_SLOT_PLAYER]->getMaxScore());
        break;
    }

    default:
        break;
    }
}

bool SpriteLine::update(const Time& t)
{
    if (SpriteStatic::update(t))
    {
        if (_current.blend == BlendMode::NONE)
            _current.color.a = 255;

        _line._width = _current.rect.w;

        updateProgress(t);
        updateRects();
        return true;
    }
    return false;
}
