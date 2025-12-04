#include "Skybound.h"

sAsset::~sAsset()
{
    RefsDel();
    SKY_ASSERT(_Reference == 0);
}

void sAsset::RefsDel()
{
    SKY_ASSERT(_Reference != 0);
    _Reference--;
};

sAssetManager::~sAssetManager()
{
    std::map<sID, sFont*>::iterator fi;
    for (fi = _Fonts.begin(); fi != _Fonts.end(); ++fi)
    {
        if (fi->second != nullptr)
        {
            delete fi->second;
            fi->second = nullptr;
        }
    }
    _Fonts.clear();

    for (auto &pi : _Pictures)
    {
        if (pi.second != nullptr)
        {
            delete pi.second;
            pi.second = nullptr;
        }
    }
    _Fonts.clear();
}

sFont* sAssetManager::getFont(sID id)
{
    auto r = _Fonts.find(id);
    if (r == _Fonts.end())
    {
        return nullptr;
    }
    return r->second;
}

sPicture* sAssetManager::getPicture(sID id)
{
    auto r = _Pictures.find(id);
    if (r == _Pictures.end())
    {
        return nullptr;
    }
    return r->second;
}

sFont* sAssetManager::addFont(sID id, const char* p_fileName, float size)
{
    sFont *p_font = sFont::LoadFromFile(p_fileName, size);
    if (p_font == nullptr)
    {
        return nullptr;
    }

    auto r = _Fonts.find(id);
    if (r != _Fonts.end())
    {
        delete r->second;
        r->second = nullptr;
        _Fonts.erase(r);
    }
    _Fonts[id] = p_font;
    return p_font;
}

sPicture* sAssetManager::addPicture(sID id, const char* p_path)
{
    sPicture* p_pic = new sPicture();
    
    if (!p_pic->LoadPNG(p_path))
    {
        delete p_pic;
        return nullptr;
    }

    auto r = _Pictures.find(id);
    if (r != _Pictures.end())
    {
        delete r->second;
        r->second = nullptr;
        _Pictures.erase(r);
    }
    _Pictures[id] = p_pic;
    return p_pic;
}




