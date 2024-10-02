#ifndef GaRep_STORAGE_H_
#define GaRep_STORAGE_H_


namespace GaRep
{
struct Settings
{
    float offset, scale;

    bool operator==(const Settings& rhs)
    {
        return offset == rhs.offset && scale == rhs.scale;
    }
    bool operator!=(const Settings& rhs) { return !operator==(rhs); }
};


} // namespace GaRep

#endif // GaRep_STORAGE_H_