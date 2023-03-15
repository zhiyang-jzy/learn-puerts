// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <Log.h>
#include <map>
#include <string>
#include <limits>

#include "Binding.hpp"
#include "CppObjectMapper.h"
#include "V8Object.hpp"

#include "libplatform/libplatform.h"
#include "v8.h"


#if defined(PLATFORM_WINDOWS)

#if _WIN64

#include "Blob/Win64/SnapshotBlob.h"

#else
#include "Blob/Win32/SnapshotBlob.h"
#endif

#elif defined(PLATFORM_ANDROID_ARM)
#include "Blob/Android/armv7a/SnapshotBlob.h"
#elif defined(PLATFORM_ANDROID_ARM64)
#include "Blob/Android/arm64/SnapshotBlob.h"
#elif defined(PLATFORM_MAC)
#include "Blob/macOS/SnapshotBlob.h"
#elif defined(PLATFORM_IOS)
#include "Blob/iOS/arm64/SnapshotBlob.h"
#elif defined(PLATFORM_LINUX)
#include "Blob/Linux/SnapshotBlob.h"
#endif

class TestClass {
public:
    TestClass(int p) {
        X = p;
    }

    static void Print(std::string msg) {
        std::cout << msg << std::endl;
    }

    int Add(int a, int b) {
//        std::cout << "Add(" << a << "," << b << ")" << std::endl;
        return a + b;
    }

    void Test(puerts::Object Obj) {

        auto P = Obj.Get<int>("p");
        std::cout << " jsobj " << P << std::endl;
        Obj.Set<std::string>("q", "john");
    }

    int X;
};

UsingCppType(TestClass);
using namespace v8;

void xx(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto pom = static_cast<puerts::FCppObjectMapper *>((v8::Local<v8::External>::Cast(
            info.Data()))->Value());
    pom->LoadCppType(info);
}

const char *ToCString(const v8::String::Utf8Value &value) {
    return *value ? *value : "<string conversion failed>";
}


void Print(const v8::FunctionCallbackInfo<v8::Value> &args) {
    bool first = true;
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope(args.GetIsolate());
        if (first) {
            first = false;
        } else {
            printf(" ");
        }
        v8::String::Utf8Value str(args.GetIsolate(), args[i]);
        const char *cstr = ToCString(str);
        printf("%s", cstr);
    }
    printf("\n");
    fflush(stdout);
}


int main() {
    // Initialize V8.
    v8::StartupData SnapshotBlob;
    SnapshotBlob.data = (const char *) SnapshotBlobCode;
    SnapshotBlob.raw_size = sizeof(SnapshotBlobCode);
    v8::V8::SetSnapshotDataBlob(&SnapshotBlob);

    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();

    // Create a new Isolate and make it the current one.
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator =
            v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate *isolate = v8::Isolate::New(create_params);

    puerts::FCppObjectMapper cppObjectMapper;
    {
        v8::Isolate::Scope isolate_scope(isolate);

        // Create a stack-allocated handle scope.
        v8::HandleScope handle_scope(isolate);

        // Create a new context.
        v8::Local<v8::Context> context = v8::Context::New(isolate);

        // Enter the context for compiling and running the hello world script.
        v8::Context::Scope context_scope(context);

        cppObjectMapper.Initialize(isolate, context);
        isolate->SetData(MAPPER_ISOLATE_DATA_POS, static_cast<puerts::ICppObjectMapper *>(&cppObjectMapper));

        context->Global()->Set(context, v8::String::NewFromUtf8(isolate, "loadCppType").ToLocalChecked(),
                               v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                                   auto pom = static_cast<puerts::FCppObjectMapper *>((v8::Local<v8::External>::Cast(
                                           info.Data()))->Value());
                                   pom->LoadCppType(info);
                               }, v8::External::New(isolate, &cppObjectMapper))->GetFunction(
                                       context).ToLocalChecked()).Check();

        puerts::PLog(puerts::LogLevel::Log,"test");

        //注册


        context->Global()->Set(context, v8::String::NewFromUtf8(isolate, "print").ToLocalChecked(),
                               v8::FunctionTemplate::New(isolate, Print)->GetFunction(context).ToLocalChecked());

        puerts::DefineClass<TestClass>()
                .Constructor<int>()
                .Function("Print", MakeFunction(&TestClass::Print))
                .Property("X", MakeProperty(&TestClass::X))
                .Method("Add", MakeFunction(&TestClass::Add))
                .Method("Test", MakeFunction(&TestClass::Test))
                .Register();

        puerts::JSClassDefinition *info = const_cast<puerts::JSClassDefinition *>(puerts::FindCppTypeClassByName(
                "TestClass"));
//        std::cout << "ababa==" << info->ScriptName << std::endl;

        TestClass tt(996);
        std::cout <<"in c++ "<< tt.X << std::endl;

        auto name = v8::String::NewFromUtf8(isolate, "testobj")
                .ToLocalChecked();
        auto obj = cppObjectMapper.FindOrAddCppObject(isolate, context, info->TypeId, &tt, true);
        context->Global()->Set(context, name, obj);


        {
            const char *csource = R"(
                    print('in js : ',testobj.X);
                    testobj.X = 332;
                    delete testobj.X;
                    print(testobj.X);
                    print('in js changed : ',testobj.X);
              )";

            // Create a string containing the JavaScript source code.
            v8::Local<v8::String> source =
                    v8::String::NewFromUtf8(isolate, csource)
                            .ToLocalChecked();

            // Compile the source code.
            v8::Local<v8::Script> script =
                    v8::Script::Compile(context, source).ToLocalChecked();

            // Run the script
            auto _unused = script->Run(context).ToLocalChecked();
            std::cout <<"after js in c++: " << tt.X << std::endl;
//
//            v8::Local<v8::Value> add_value;
//            context->Global()->Get(context, v8::String::NewFromUtf8(isolate, "add").ToLocalChecked()).ToLocal(
//                    &add_value);
//            v8::Local<v8::Function> add = v8::Local<v8::Function>::Cast(add_value);
//
//// 构造一个调用参数列表
//            v8::Local<v8::Value> argv[2] = {v8::Number::New(isolate, 2), v8::Number::New(isolate, 3)};
//
//// 调用 JavaScript 函数
//            v8::Local<v8::Value> result = add->Call(context, context->Global(), 2, argv).ToLocalChecked();
//
//// 将返回值转换为 double 类型并打印出来
//            double num = result->NumberValue(context).ToChecked();
//            printf("result: %f\n", num);
//
//
//            Local<Value> function_value = context->Global()->Get(context, String::NewFromUtf8(isolate,
//                                                                                              "my_function").ToLocalChecked()).ToLocalChecked();
//            Local<Function> function = Local<Function>::Cast(function_value);
//            const int argc = 0;
//            v8::Local<v8::Value> lvalue = function->Call(context, context->Global(), argc,
//                                                         argv).ToLocalChecked()->ToObject(context).ToLocalChecked();
//
//            // 获取局部变量的值
//            int32_t value = lvalue->Int32Value(context).FromMaybe(-1);
//
//            std::cout << value << " --- " << std::endl;

        }

        cppObjectMapper.UnInitialize(isolate);

    }

    // Dispose the isolate and tear down V8.
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
    return 0;
}
