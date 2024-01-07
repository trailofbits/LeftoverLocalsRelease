/*
   Copyright 2023 Trail of Bits

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <jni.h>
#include <android/log.h>
#include "easyvk.h"

#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>

#if !defined(SHARED_MEMORY_SIZE)
#define SHARED_MEMORY_SIZE 4096
#endif

// In case we want to try out of bounds accesses
#if !defined(SHARED_MEMORY_SIZE_TRAVERSED)
#define SHARED_MEMORY_SIZE_TRAVERSED (SHARED_MEMORY_SIZE)
#endif
#include <assert.h>

std::vector<uint32_t> LoadBinaryFileToVector(const char *file_path,
                                            AAssetManager *assetManager) {
    std::vector<uint32_t> file_content;
    assert(assetManager);
    AAsset *file =
            AAssetManager_open(assetManager, file_path, AASSET_MODE_BUFFER);
    size_t file_length = AAsset_getLength(file);

    file_content.resize((file_length+3)/4);

    AAsset_read(file, file_content.data(), file_length);
    AAsset_close(file);
    return file_content;
}

extern "C" JNIEXPORT jintArray JNICALL
Java_com_android_covertVKListener_CovertVKListener_entry(
        JNIEnv* env,
        jobject obj,
        jobject assetManager) {

    int gridSize = 32;
    int workGroupSize = 32;
    int size = SHARED_MEMORY_SIZE_TRAVERSED * gridSize;

    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    auto spvCode = LoadBinaryFileToVector("stripped.spv", mgr);
    jintArray resultArray = env->NewIntArray(size);

    // Initialize instance
    auto instance = easyvk::Instance(false);

    // Get list of available physical devices.
    auto physicalDevices = instance.physicalDevices();

    // Create device from first physical device.
    auto device = easyvk::Device(instance, physicalDevices.at(0));

    //__android_log_print(ANDROID_LOG_INFO, "EasyVK", "%s", device.properties.deviceName);

    auto a = easyvk::Buffer(device, size);
    auto b = easyvk::Buffer(device, size);
    auto c = easyvk::Buffer(device, size);
    auto canary = easyvk::Buffer(device, 1);

    long iterations = 0;

    // Write initial values to the buffers.
    for (int i = 0; i < size; i++) {
        a.store(i, i);
        b.store(i, i);
        c.store(i, i);
    }

    std::vector<easyvk::Buffer> bufs = {a, b, c, canary};

    auto program = easyvk::Program(device, spvCode, bufs);

    // Dispatch 4 work groups of size 1 to carry out the work.
    program.setWorkgroups(gridSize);
    program.setWorkgroupSize(workGroupSize);

    // Run the kernel.
    program.initialize("covertListener");

    program.run();

    jint *elements = env->GetIntArrayElements(resultArray, nullptr);

    //int nonZeros = 0;
    for(int i = 0; i < size; i++) {
        unsigned v = c.load(i);
        elements[i] = v;
    }

    program.teardown();

    // Cleanup.
    a.teardown();
    b.teardown();
    c.teardown();
    device.teardown();
    instance.teardown();

    env->ReleaseIntArrayElements(resultArray, elements, 0);
    return resultArray;
}