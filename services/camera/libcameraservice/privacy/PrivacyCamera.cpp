#define LOG_TAG "Privacy-Camera"

#include "privacy/PrivacyCamera.h"

namespace android {

PrivacyCamera::PrivacyCamera(CameraMetadata cameraMetadata) {
    // Init Active Array Size
    camera_metadata_entry entry = cameraMetadata.find(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE);
    if (entry.count < 4) {
        ALOGE("PrivacyCamera::PrivacyCamera::Can not get active array size.");
        return;
    }

    mActiveArraySize.left = entry.data.i32[0];
    mActiveArraySize.top = entry.data.i32[1];
    mActiveArraySize.width = entry.data.i32[2];
    mActiveArraySize.height = entry.data.i32[3];
    ALOGI("PrivacyCamera::PrivacyCamera::mActiveArraySize = {%d, %d, %d, %d}",
        mActiveArraySize.left, mActiveArraySize.top, mActiveArraySize.width, mActiveArraySize.height);

    //Get Max Support Face Count
    entry = cameraMetadata.find(ANDROID_STATISTICS_INFO_MAX_FACE_COUNT);
    if (entry.count <= 0) {
        ALOGE("PrivacyCamera::PrivacyCamera::Can not get max face count.");
    }
    mMaxSupportFaces = entry.data.u8[0];
    ALOGI("PrivacyCamera::PrivacyCamera::mMaxSupportFaces = %d", mMaxSupportFaces);

    char value[PROPERTY_VALUE_MAX];
    property_get(PROPERTY_PRIVACY_CAMERA_RADIUS_RATIO, value, "0");
    float radius_ratio = atof(value);
    if (radius_ratio > 0) {
        mRadiusRatio = radius_ratio;
    }

    // Init Face Rect
    mSavedFace.rect[0] = 0;
    mSavedFace.rect[1] = 0;
    mSavedFace.rect[2] = 0;
    mSavedFace.rect[3] = 0;

    // Init OpenCL Program
    cl_int err = CL_SUCCESS;

    // Platform
    err = clGetPlatformIDs(1, &mOpenCLObjects.platform, 0);
    CHECK_OPENCL_ERRORS(err);

    //judge which platform, MTK or QUALCOMM
    bool is_mtk = false;
    size_t platform_name_length = 0;
    err = clGetPlatformInfo(mOpenCLObjects.platform, CL_PLATFORM_VENDOR, 0, 0, &platform_name_length);
    CHECK_OPENCL_ERRORS(err);

    char *platform_name_buffer = (char*) malloc(platform_name_length);
    err = clGetPlatformInfo(mOpenCLObjects.platform, CL_PLATFORM_VENDOR, platform_name_length, platform_name_buffer, 0);
    std::string platform_name = platform_name_buffer;
    delete[] platform_name_buffer;
    ALOGI("PrivacyCamera::PrivacyCamera::platform_name = %s", platform_name.c_str());
    CHECK_OPENCL_ERRORS(err);
    std::string::size_type position = platform_name.find("QUALCOMM");
    if (position == platform_name.npos) {
        is_mtk = true;
    }
    ALOGI("PrivacyCamera::PrivacyCamera::is_mtk = %d", is_mtk);

    // Device
    err = clGetDeviceIDs(mOpenCLObjects.platform, CL_DEVICE_TYPE_GPU, 1, &mOpenCLObjects.device, 0);
    CHECK_OPENCL_ERRORS(err);

    // Context
    cl_context_properties context_props[] = {CL_CONTEXT_PRIORITY_HINT_QCOM, CL_PRIORITY_HINT_HIGH_QCOM, 0};
    if (is_mtk) {
        mOpenCLObjects.context = clCreateContext(0, 1, &mOpenCLObjects.device, 0, 0, &err);
    } else {
        mOpenCLObjects.context = clCreateContext(context_props, 1, &mOpenCLObjects.device, 0, 0, &err);
    }
    CHECK_OPENCL_ERRORS(err);

    // Program
    mOpenCLObjects.program = clCreateProgramWithSource(mOpenCLObjects.context, 1, &FACE_RECOGNITION, 0, &err);
    CHECK_OPENCL_ERRORS(err);

    err = clBuildProgram(mOpenCLObjects.program, 0, 0, 0, 0, 0);
    if(err == CL_BUILD_PROGRAM_FAILURE) {
        size_t log_length = 0;
        err = clGetProgramBuildInfo(mOpenCLObjects.program, mOpenCLObjects.device, CL_PROGRAM_BUILD_LOG, 0, 0, &log_length);
        CHECK_OPENCL_ERRORS(err);

        vector<char> log(log_length);
        err = clGetProgramBuildInfo(mOpenCLObjects.program, mOpenCLObjects.device, CL_PROGRAM_BUILD_LOG, log_length, &log[0], 0);
        CHECK_OPENCL_ERRORS(err);
        ALOGE("Error happened during the build of OpenCL program.\nBuild log:%s", &log[0]);
        return;
    }

    // Kernel
    mOpenCLObjects.kernel = clCreateKernel(mOpenCLObjects.program, "handle_face_regions", &err);
    CHECK_OPENCL_ERRORS(err);

    // Command-Queue
    mOpenCLObjects.queue = clCreateCommandQueue(mOpenCLObjects.context, mOpenCLObjects.device,0, &err);
    CHECK_OPENCL_ERRORS(err);

    ALOGI("PrivacyCamera::PrivacyCamera::Privacy camera init OpenCL finished successfully.");
}

PrivacyCamera::~PrivacyCamera() {
    cl_int err = CL_SUCCESS;
    err = clReleaseKernel(mOpenCLObjects.kernel);
    CHECK_OPENCL_ERRORS(err);

    err = clReleaseProgram(mOpenCLObjects.program);
    CHECK_OPENCL_ERRORS(err);

    err = clReleaseCommandQueue(mOpenCLObjects.queue);
    CHECK_OPENCL_ERRORS(err);

    err = clReleaseContext(mOpenCLObjects.context);
    CHECK_OPENCL_ERRORS(err);
}

bool PrivacyCamera::isSupportPrivacyCamera(CameraMetadata cameraMetadata) {
    char value[PROPERTY_VALUE_MAX];
    property_get(PROPERTY_PRIVACY_CAMERA, value, "");
    if (strcmp(value, "true")) {
        return false;
    }

    camera_metadata_entry facing = cameraMetadata.find(ANDROID_LENS_FACING);
    int facing_count = facing.count;
    if (facing_count <= 0) {
        ALOGE("PrivacyCamera::isSupportPrivacyCamera::Can not get lens facing.");
        return false;
    }

    uint8_t lens_facing = facing.data.u8[0];
    if (lens_facing != ANDROID_LENS_FACING_FRONT) {
        ALOGE("PrivacyCamera::isSupportPrivacyCamera::Is not front camera.");
        return false;
    }

    return true;
}

void PrivacyCamera::saveFacesData(const camera_metadata_t *metadata) {
    if(metadata == nullptr) {
        return;
    }

    char value[PROPERTY_VALUE_MAX];
    property_get(PROPERTY_PRIVACY_CAMERA_SWITCH, value, "");
    if (strcmp(value, "true")) {
        return;
    }

    status_t res;
    camera_metadata_ro_entry_t entry;
    res = find_camera_metadata_ro_entry(metadata, ANDROID_STATISTICS_FACE_DETECT_MODE, &entry);
    if (res != OK ||entry.count == 0) {
        return;
    }
    uint8_t faceDetectMode = entry.data.u8[0];
    if (faceDetectMode == ANDROID_STATISTICS_FACE_DETECT_MODE_OFF) {
        CHECK_FACE_DETECT_ERRORS("Face detect mode is OFF.");
    }

    res = find_camera_metadata_ro_entry(metadata, ANDROID_STATISTICS_FACE_RECTANGLES, &entry);
    if (res != OK ||entry.count < 4) {
        return;
    }
    size_t number_of_faces = entry.count / 4;
    if (number_of_faces > mMaxSupportFaces) {
        CHECK_FACE_DETECT_ERRORS("Number of faces is more than max support faces.");
    }
    const int32_t *faceRects = entry.data.i32;

    res = find_camera_metadata_ro_entry(metadata, ANDROID_STATISTICS_FACE_SCORES, &entry);
    if (res != OK ||entry.count == 0) {
        CHECK_FACE_DETECT_ERRORS("Can not get face score.");
    }
    const uint8_t *faceScores = entry.data.u8;

    for (size_t i = 0; i < number_of_faces; i++) {
        if (faceScores[i] == 0) {
            continue;
        }

        if (faceScores[i] > 100) {
            ALOGW("%s: Face index %zu with out of range score %d", __FUNCTION__, i, faceScores[i]);
        }

        mSavedFace.rect[0] = faceRects[i*4 + 0];
        mSavedFace.rect[1] = faceRects[i*4 + 1];
        mSavedFace.rect[2] = faceRects[i*4 + 2];
        mSavedFace.rect[3] = faceRects[i*4 + 3];
        mSavedFace.score = faceScores[i];
        break;
    }
}

void PrivacyCamera::handleOutputBuffers(buffer_handle_t* outputBuffer) {
    if (outputBuffer == nullptr) {
        return;
    }

    char value[PROPERTY_VALUE_MAX];
    property_get(PROPERTY_PRIVACY_CAMERA_SWITCH, value, "");
    if (strcmp(value, "true")) {
        return;
    }

    struct private_handle_t *ph = reinterpret_cast<struct private_handle_t*>(const_cast<native_handle*>(*outputBuffer));
    char *ptr = (char*)mmap(NULL, ph->size, PROT_READ |PROT_WRITE, MAP_SHARED, ph->fd, 0);

    int face_rect[4] = {0, 0, 0, 0};
    int face_radius = 0;
    int center_width = 0;
    int center_height = 0;
    // Calculate Face Rect
    float scale_width = ph->unaligned_width * 1.0f / mActiveArraySize.width;
    float scale_height = ph->unaligned_height * 1.0f / mActiveArraySize.height;
    int half_face_width = (mSavedFace.rect[2] - mSavedFace.rect[0]) * scale_width / 2;
    int half_face_heigh = (mSavedFace.rect[3] - mSavedFace.rect[1]) * scale_height /2 ;
    center_width = (mSavedFace.rect[0] + PRIVACY_CAMERA_FACE_CENTER_OFFSET)* scale_width + half_face_width;
    center_height = mSavedFace.rect[1] * scale_height + half_face_heigh;
    face_radius = max(half_face_width, half_face_heigh) * (mRadiusRatio +  0.1 / scale_width);

    int width_difference = abs(mLastCenterWidth - center_width);
    int height_difference = abs(mLastCenterHeight - center_height);
    int radius_difference = abs(mLastRadius - face_radius);
    int cache_offset = PRIVACY_CAMERA_FACE_CACHE_OFFSET * scale_width;
    // Use Cache Face Rect Anti Shake
    if (mLastRadius > 0) {
        if (width_difference < cache_offset && height_difference < cache_offset && radius_difference < cache_offset) {
            center_width = mLastCenterWidth;
            center_height = mLastCenterHeight;
            face_radius = mLastRadius;
        } else {
            center_width = mLastCenterWidth + (center_width - mLastCenterWidth) / PRIVACY_CAMERA_FACE_MOVE_TIMES;
            center_height = mLastCenterHeight + (center_height - mLastCenterHeight) / PRIVACY_CAMERA_FACE_MOVE_TIMES;
            face_radius = mLastRadius + (face_radius - mLastRadius) / PRIVACY_CAMERA_FACE_MOVE_TIMES;
        }
    }

    mLastCenterWidth = center_width;
    mLastCenterHeight = center_height;
    mLastRadius = face_radius;

    face_rect[0] = center_width - face_radius;
    face_rect[1] = center_height - face_radius;
    face_rect[2] = center_width + face_radius;
    face_rect[3] = center_height + face_radius;

    cl_int err = CL_SUCCESS;
    cl_int input_buffer_size = 4 * sizeof(int);
    cl_mem input_buffer = clCreateBuffer(mOpenCLObjects.context,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, input_buffer_size, face_rect, &err);
    CHECK_OPENCL_ERRORS(err);
    // Change Y & U if YUV
    cl_int y_size = ph->width * ph->height;
    cl_int output_buffer_size = y_size * 1.5f;
    cl_mem output_buffer = clCreateBuffer(mOpenCLObjects.context,
            CL_MEM_WRITE_ONLY |CL_MEM_USE_HOST_PTR, output_buffer_size, ptr, &err);
    CHECK_OPENCL_ERRORS(err);
    err = clSetKernelArg(mOpenCLObjects.kernel, 0, sizeof(cl_mem), &output_buffer);
    err = clSetKernelArg(mOpenCLObjects.kernel, 1, sizeof(cl_mem), &input_buffer);
    err = clSetKernelArg(mOpenCLObjects.kernel, 2, sizeof(cl_int), &ph->width);
    err = clSetKernelArg(mOpenCLObjects.kernel, 3, sizeof(cl_int), &y_size);
    err = clSetKernelArg(mOpenCLObjects.kernel, 4, sizeof(cl_int), &face_radius);
    err = clSetKernelArg(mOpenCLObjects.kernel, 5, sizeof(cl_int), &center_width);
    err = clSetKernelArg(mOpenCLObjects.kernel, 6, sizeof(cl_int), &center_height);

    CHECK_OPENCL_ERRORS(err);

    size_t globalSize[2] = {static_cast<size_t>(ph->width), static_cast<size_t>(ph->height)};
    err = clEnqueueNDRangeKernel(mOpenCLObjects.queue, mOpenCLObjects.kernel, 2, 0 ,globalSize, 0, 0, 0, 0);
    CHECK_OPENCL_ERRORS(err);

    err = clFinish(mOpenCLObjects.queue);
    CHECK_OPENCL_ERRORS(err);

    clEnqueueMapBuffer(mOpenCLObjects.queue, output_buffer, CL_TRUE, CL_MAP_READ, 0, output_buffer_size, 0, 0, 0, &err);
    CHECK_OPENCL_ERRORS(err);

    err = clEnqueueUnmapMemObject(mOpenCLObjects.queue, output_buffer, ptr, 0, 0, 0);
    CHECK_OPENCL_ERRORS(err);

    err = clReleaseMemObject(input_buffer);
    CHECK_OPENCL_ERRORS(err);

    err = clReleaseMemObject(output_buffer);
    CHECK_OPENCL_ERRORS(err);
    munmap(ptr, ph->size);
}

} // end namespace android
