#ifndef RRL_PARAMS_HH
#define RRL_PARAMS_HH

#include <string>

namespace Rrl{

class Params
{
public:
    static std::string toString(const std::string& arg);
    static bool toBool(const std::string& arg);
    static int toInt(const std::string& arg);
    static double toDouble(const std::string& arg);

    static void registerParams();
};

}

#endif // RRL_PARAMS_HH
