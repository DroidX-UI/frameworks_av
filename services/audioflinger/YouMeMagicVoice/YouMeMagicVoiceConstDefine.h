#ifndef YouMeMagicVoiceConstDefine_hpp
#define YouMeMagicVoiceConstDefine_hpp

// 版本号定义，前三个版本号根据需要手动更新，最后一个版本号 BUILD_NUMBER 由编译脚本在每次编译开始前自动更新
// 主版本号和次版本号相同的SDK，具有完全的兼容性，可直接替换升级
#define YMMV_EG_MAIN_VER       1  //主版本号，当有大的功能提升时，该版本号加1
#define YMMV_EG_MINOR_VER      0  //次版本号，当API有改动时，该版本号加1
#define YMMV_EG_RELEASE_NUMBER 5  //发布版本号，每次正式提测准备对外发布的版本时，该版本号加1为奇数。测试过程发现严重问题，
//修正后回归测试时，该版本号不变。测试过程没发现问题或只发现小问题，该版本号加1为偶数。

#define YMMV_EG_BUILD_NUMBER 213

#define YMMV_EG_SDK_VERSION "%d.%d.%d.%d"
#define YMMV_EG_SDK_NUMBER  ( (((YMMV_EG_MAIN_VER) << 28) & 0xF0000000) | (((YMMV_EG_MINOR_VER) << 22) & 0x0FC00000) | (((YMMV_EG_RELEASE_NUMBER) << 14) & 0x003FC000) | ((YMMV_EG_BUILD_NUMBER) & 0x00003FFF) )

#include <string>


typedef enum YouMeMagicVoiceErrorCode{
    YOUME_MAGICVOICE_SUCCESS                        = 0, // 成功
    YOUME_MAGICVOICE_ERROR_ILLEGAL_SDK              =-1, // 非法使用SDK
    YOUME_MAGICVOICE_ERROR_TOKEN_ERROR              =-2, // TOKEN错误
    YOUME_MAGICVOICE_ERROR_INVALID_PARAM            =-3, // 参数错误
    YOUME_MAGICVOICE_ERROR_WRONG_STATE              =-4, // 状态错误
    YOUME_MAGICVOICE_ERROR_API_NOT_SUPPORTED        =-5, // API不支持
    YOUME_MAGICVOICE_ERROR_EQ_BAND_NOT_FIND         =-6, // 找不到对应的eqband
    YOUME_MAGICVOICE_ERROR_EXPIRED                  =-7, // 变声音效已过期
    YOUME_MAGICVOICE_ERROR_WRONG_FILE_PATH          =-8, // 文件路径有误
    YOUME_MAGICVOICE_ERROR_OUT_OF_MEMORY            =-9, // 内存不足
    YOUME_MAGICVOICE_ERROR_NETWORK_ERROR            =-10,// 网络错误
    YOUME_MAGICVOICE_ERROR_SERVER_INTER_ERROR       =-11,// 服务器内部错误
    YOUME_MAGICVOICE_ERROR_SOUND_TOO_SHORT          =-12,// 音频文件过短
    YOUME_MAGICVOICE_ERROR_SOUND_NO_VOICED_FRAMES   =-13,// 音频无语音内容

    YOUME_MAGICVOICE_ERROR_UNKNOWN              =-100// 未知错误

}YouMeMagicVoiceErrorCode;

typedef enum YouMeMagicVoiceState{
    YOUME_MAGICVOICE_STATE_INITED              =1,   // 已初始化（构造即是初始化）
    YOUME_MAGICVOICE_STATE_STARTED             =1<<1,// 已启动
    YOUME_MAGICVOICE_STATE_STOPPED             =1<<2 // 已停止
}YouMeMagicVoiceState;

#endif /* YouMeMagicVoiceConstDefine_hpp */
