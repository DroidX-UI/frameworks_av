#ifndef YouMeMagicVoiceEngine_hpp
#define YouMeMagicVoiceEngine_hpp

#include "YouMeMagicVoiceConstDefine.h"
#include <stdio.h>
#include <stdint.h>
#ifdef WIN32
#ifdef YouMeMagicVoice_EXPORTS
#define YouMeMagicVoice_API __declspec(dllexport)
#else
#define YouMeMagicVoice_API __declspec(dllimport)
#endif
#else
#define YouMeMagicVoice_API __attribute ((visibility("default")))
#endif




class YouMeMagicVoice_API YouMeMagicVoiceChanger
{
public:
    virtual ~YouMeMagicVoiceChanger(){};

    /**
     *  功能描述:获取当前变声器状态
     *
     *  @return 见YouMeMagicVoiceState变声器状态
     */
    virtual YouMeMagicVoiceState getState() = 0;

    /**
     *  功能描述:开启变声处理线程
     *
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    virtual YouMeMagicVoiceErrorCode start () = 0;

    /**
     *  功能描述:停止变声处理线程
     *
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    virtual YouMeMagicVoiceErrorCode stop () = 0;

    /**
     *  功能描述:设置音频采样率，必须在start方法之前设置
     *
     *  @param sampleRate:采样率，支持8000/11025/12000/16000/22050/24000/32000/44100/48000Hz
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    virtual YouMeMagicVoiceErrorCode setSampleRate(int sampleRate) = 0;

    /**
     *  功能描述:设置声道数，必须在start方法之前设置
     *
     *  @param channels:声道数，目前仅支持单声道和双声道
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    virtual YouMeMagicVoiceErrorCode setChannels(int channels) = 0;

    /**
     *  功能描述:设置最小处理单元
     *
     *  @param processUnitMS:最小处理单元，应大于100ms，建议160ms
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    virtual YouMeMagicVoiceErrorCode setProcessUnitMS(int processUnitMS) = 0;

    /**
     *  功能描述:设置叠加优化算法因子
     *
     *  @param overlapFactor:叠加优化算法因子，设置为0.2f到0.5f之间
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    virtual YouMeMagicVoiceErrorCode setOverlapFactor(float overlapFactor) = 0;

    /**
     *  功能描述:设置叠加优化算法平滑时间
     *
     *  @param overlapSmoothMs:叠加优化算法平滑时间，设置为10到20之间
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    virtual YouMeMagicVoiceErrorCode setOverlapSmoothMs(int overlapSmoothMs) = 0;

    /**
     *  功能描述:放入待处理的数据采样
     *  将samples数据存入变声器待处理buffer池，采样个数为nSamples，必须在start之后，stop之前调用该方法！
     *  注：所占字节数为nSamples * sizeof(int16_t)
     *
     *  @param samples：待处理buffer
     *         nSamples：待处理buffer的采样个数
     *         bitsPerSample：输入数据的位深
     *  @return 成功放入的采样个数
     */
    virtual int putSamples(void* samples, size_t nSamples, int bitsPerSample) = 0;

    /**
     *  功能描述:查询可取的数据采样
     *  查询可用的数据采样，有足够取出的数据采样，才可以调用取出方法
     *
     *  @return 可取的数据采样个数
     */
    virtual int numSamples() = 0;

    /**
     *  功能描述:取出处理完毕的数据采样
     *  从变声器处理后的buffer池中取出采样个数为nSamples的采样数据，写入到samples中；应查询可用的数据采样，有足够取出的数据采样，才可以调用取出方法。必须在start之后，stop之前调用该方法！
     *  注：所占字节数为nSamples * sizeof(int16_t)
     *
     *  @param samples：待写入的buffer
     *         nSamples：待写入buffer的采样个数
     *  @return 成功取出的采样个数
     */
    virtual int getSamples(void* samples, size_t nSamples) = 0;

    /**
     *  功能描述:清空输入和输出Buffer
     *  必须在start之后，stop之前调用该方法！
     *
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    virtual YouMeMagicVoiceErrorCode flushBuffer() = 0;
};

class YouMeMagicVoice_API YouMeMagicVoiceEngine {
public:
    YouMeMagicVoiceEngine() = delete;
    ~YouMeMagicVoiceEngine() = delete;
    YouMeMagicVoiceEngine(const YouMeMagicVoiceEngine &) = delete;
    YouMeMagicVoiceEngine& operator=(const YouMeMagicVoiceEngine &) = delete;

    /**
     *  功能描述:初始化操作
     *
     */
    static void init();

    /**
     *  功能描述:销毁操作，会释放已创建的全部YouMeMagicVoiceChanger实例
     *
     */
    static void destroy(void);

    /**
     *  功能描述:创建引擎实例
     *
     */
    static YouMeMagicVoiceChanger* createMagicVoiceEngine();

    /**
     *  功能描述:销毁引擎实例，释放内存
     *
     */
    static void releaseMagicVoiceEngine(YouMeMagicVoiceChanger* magicVoiceEngine);

    /**
     *  功能描述:设置是否是测试服，默认是正式服
     *
     */
    static void setTestServer(bool testServer);

    /**
     *  功能描述:设置数据上报缓存目录
     *
     */
    static void setDocumentPath(const char *documentPath);

    /**
     *  功能描述:设置数据上报设备IMEI
     *
     */
    static void setDeviceIMEI(const char *deviceIMEI);

    /**
     *  功能描述:设置数据上报UUID
     *
     */
    static void setUUID(const char *UUID);

    /**
     *  功能描述:设置包名
     *
     */
    static void setPackageName(const char *packageName);

    /**
     *  功能描述:设置变声功能参数
     *
     *  @param effectInfo: 由变声管理模块获取到的YMMagicVoiceEffectInfo（变声音效信息），其中的m_param参数，设置时将变声器置为可用
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    static YouMeMagicVoiceErrorCode setMagicVoiceInfo(const char *effectInfo);

    /**
     *  功能描述:清空变声功能参数
     *
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    static YouMeMagicVoiceErrorCode clearMagicVoiceInfo();

    /**
     *  功能描述:设置变声参数微调接口
     *
     *  @param dFS: 音色的微调值，范围-1.0f~1.0f，在后台下发的基准值的基础上按百分比调节，值减小音色会变厚重，值增大音色会变尖锐
     *         dSemitones: 音调的微调值，范围-1.0f~1.0f，在后台下发的基准值的基础上按百分比调节，值减小音调会低，值增大音调会变高
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    static YouMeMagicVoiceErrorCode setMagicVoiceAdjust(double dFS, double dSemitones);

    /**
     *  功能描述:设置音效混音文件的地址，设置后会解码该地址的音效文件，并与变声效果一同混音输出
     *
     *  @param soundEffectMixInfo: 音效混音文件的地址
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    static YouMeMagicVoiceErrorCode setSoundEffectMixInfo(const char *soundEffectMixInfo);

    /**
     *  功能描述:停止音效文件的混音
     *
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    static YouMeMagicVoiceErrorCode stopSoundEffectMix();

    /**
     *  功能描述:变声器效果是否启用
     *
     *  @return 变声参数是否已生效，为false时表示当前没有变声效果生效，可以不创建变声器
     */
    static bool getMagicVoiceEffectEnabled();

    /**
     *  功能描述:返回变声参数dump数据
     *
     *  @param data：待写入buffer
     *         size：待写入buffer所占字节数
     */
    static void dump(char* data, int size);

    /**
     *  功能描述:设置变声pcm dump目录
     *
     *  @param dir：变声pcm dump目录，null表示不进行dump
     *  @param flush_with_start：是否在每次start时刷新dump
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    static YouMeMagicVoiceErrorCode setPcmDumpDir(const char* dir, bool flush_with_start = true);

    /**
     *  功能描述:使用当前变声效果处理音频数据文件，耗时操作，不建议放到主线程处理
     *
     *  @param inPath：待处理数据的文件路径
     *         outPath：要写入的文件路径
     *         sampleRate：待处理数据的采样率
     *         channels：待处理数据的声道数
     *  @return 见YouMeMagicVoiceErrorCode错误码
     */
    static YouMeMagicVoiceErrorCode processVoiceFile(const char* inPath, const char* outPath, int sampleRate, int channels);

    static YouMeMagicVoiceErrorCode processAudioFile(const char* voiceInputAudioPath, const char* voiceOutputAudioPath, int magicvoiceEffectId);

    static double audioCompareFile(const char* refAudioPath, const char* userAudioPath, int sampleRate, int channels);

    static YouMeMagicVoiceErrorCode voiceFileDetect(const char* voiceFilePath, bool *isMale, double *pitch, double *formant, double *volumeDb);
};

#endif /* YouMeMagicVoiceEngine_hpp */
