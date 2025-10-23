#include "config.h"

namespace livermore::tx
{

err_t config::load(const char *filepath)
{
    if(!read_file(filepath))
        return error::READ_CONFIG_FAILED;

    return OK;
}

err_t config::check()
{
    // log file size must >= 1 MB && <= 1024 MB
    if(get<int>("log.file_size_mb", 1) < 1)
        return error::LOG_FILE_SIZE_TOO_SMALL;
    if(get<int>("log.file_size_mb", 1) > 1024)
        return error::LOG_FILE_SIZE_TOO_BIG;

    // log file num must >= 1 && <= 100
    if(get<int>("log.file_num", 1) < 1)
        return error::LOG_FILE_NUM_TOO_SMALL;
    if(get<int>("log.file_num", 1) > 100)
        return error::LOG_FILE_NUM_TOO_BIG;

    // log level must >= trace(0) && <= off(6)
    if(get<int>("log.min_lvl", 1) < 0)
        return error::LOG_LEVEL_TOO_SMALL;
    if(get<int>("log.min_lvl", 1) > 6)
        return error::LOG_LEVEL_TOO_BIG;

    return OK;
}

} // namespace livermore::tx