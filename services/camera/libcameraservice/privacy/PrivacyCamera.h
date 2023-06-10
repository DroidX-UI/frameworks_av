#include <utils/Log.h>
#include <utils/Errors.h>

#include <sys/mman.h>
#include <system/camera_metadata.h>
#include <hardware/camera.h>
#include <camera/CaptureResult.h>

#include "CL/cl.h"
#include "CL/cl_platform.h"
#include "gr_priv_handle.h"
#include "xm/IPrivacyCamera.h"
#include "cutils/properties.h"

using namespace std;

namespace android {

const static char* FACE_RECOGNITION =
        "#pragma OPENCL FACE RECOGNITION\n"
        "__kernel void handle_face_regions(global char* inputPixels, global int* facesRect, const int width, const int y_size, const int radius\n"
                ",  const int center_width,  const int center_height) {\n"
            "int x = get_global_id(0);\n"
            "int y = get_global_id(1);\n"
            "if(x <= facesRect[0] || x >= facesRect[2] || y <= facesRect[1] || y >= facesRect[3]) {\n"
                "inputPixels[y * width + x] = 0;\n"
                "inputPixels[y / 2 * width + x + y_size] = 125;\n"
                "return;\n"
            "}\n"
            "if((x - center_width) * (x - center_width) + (y - center_height) * (y - center_height) > radius * radius) {\n"
                "inputPixels[y * width + x] = 0;\n"
                "inputPixels[y / 2 * width + x + y_size] = 125;\n"
            "}\n"
        "}\n";

#define PRIVACY_CAMERA_FACE_RADIUS_RATIO 1.4f
#define PRIVACY_CAMERA_FACE_MOVE_TIMES 8
#define PRIVACY_CAMERA_FACE_CACHE_OFFSET 30
#define PRIVACY_CAMERA_FACE_CENTER_OFFSET 50
#define CL_CONTEXT_PRIORITY_HINT_QCOM 0x40C9
#define CL_PRIORITY_HINT_HIGH_QCOM 0x40CA
#define PROPERTY_PRIVACY_CAMERA "persist.sys.privacy_camera"
#define PROPERTY_PRIVACY_CAMERA_SWITCH "persist.sys.privacy_camera_switch"
#define PROPERTY_PRIVACY_CAMERA_RADIUS_RATIO "persist.sys.privacy_camera_radius_ratio"
#define PROPERTY_PRIVACY_CAMERA_CENTER_OFFSET "persist.sys.privacy_camera_center_offset"

#define CHECK_FACE_DETECT_ERRORS(ERR)   \
    ALOGW("Face Detect error: %s in file %s at line %d. Exiting.", ERR, __FILE__, __LINE__);    \
    return;

#define CHECK_OPENCL_ERRORS(ERR)        \
    if(ERR != CL_SUCCESS) {             \
        ALOGW( "OpenCL error with code %d happened in file %s at line %d. Exiting.", ERR, __FILE__, __LINE__ );    \
        return;                         \
    }


class PrivacyCamera : public IPrivacyCamera {
public:
    PrivacyCamera(CameraMetadata cameraMetadata);

    virtual ~PrivacyCamera();

    static bool isSupportPrivacyCamera(CameraMetadata cameraMetadata);

    void saveFacesData(const camera_metadata_t *metadata);

    void handleOutputBuffers(buffer_handle_t* outputBuffer);
private:
    struct OpenCLObjects {
        // Regular OpenCL objects:
        cl_platform_id platform;
        cl_device_id device;
        cl_context context;
        cl_command_queue queue;
        cl_program program;
        cl_kernel kernel;
    };

    struct ActiveArraySize {
        int32_t left;
        int32_t top;
        int32_t width;
        int32_t height;
    };

    uint8_t mMaxSupportFaces = 0;
    camera_face_t mSavedFace;
    OpenCLObjects mOpenCLObjects;
    ActiveArraySize mActiveArraySize;

    int mLastCenterWidth = -1;
    int mLastCenterHeight = -1;
    int mLastRadius = -1;

    float mRadiusRatio = PRIVACY_CAMERA_FACE_RADIUS_RATIO;
};


} // end namespace android
