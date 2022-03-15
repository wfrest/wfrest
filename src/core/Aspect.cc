#include "Aspect.h"

using namespace wfrest;

GlobalAspect *GlobalAspect::get_instance()
{
    static GlobalAspect kInstance;
    return &kInstance;
}

GlobalAspect::~GlobalAspect()
{
    for(auto asp : aspect_list)
    {
        delete asp;
    }
}