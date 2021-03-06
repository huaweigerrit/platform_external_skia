/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ok.h"
#include "Test.h"

struct TestStream : Stream {
    const skiatest::TestRegistry* registry = skiatest::TestRegistry::Head();
    bool extended = false, verbose = false;

    static std::unique_ptr<Stream> Create(Options options) {
        TestStream stream;
        if (options("extended") != "") { stream.extended = true; }
        if (options("verbose" ) != "") { stream.verbose  = true; }

        return move_unique(stream);
    }

    struct TestSrc : Src {
        skiatest::Test test {"", false, nullptr};
        bool extended, verbose;

        std::string name() override { return test.name; }
        SkISize     size() override { return {0,0}; }

        bool draw(SkCanvas*) override {
            // TODO(mtklein): GrContext

            struct : public skiatest::Reporter {
                bool ok = true;
                const char* name;
                bool extended, verbose_;

                void reportFailed(const skiatest::Failure& failure) override {
                    SkDebugf("%s: %s\n", name, failure.toString().c_str());
                    ok = false;
                }
                bool allowExtendedTest() const override { return extended; }
                bool           verbose() const override { return verbose_; }
            } reporter;
            reporter.name     = test.name;
            reporter.extended = extended;
            reporter.verbose_ = verbose;

            test.proc(&reporter, nullptr);
            return reporter.ok;
        }
    };

    std::unique_ptr<Src> next() override {
        if (!registry) {
            return nullptr;
        }
        TestSrc src;
        src.test = registry->factory();
        src.extended = extended;
        src.verbose  = verbose;
        registry = registry->next();
        return move_unique(src);
    }
};
static Register test{"test", TestStream::Create};

// Hey, now why were these defined in DM.cpp?  That's kind of weird.
namespace skiatest {
#if SK_SUPPORT_GPU
    bool IsGLContextType(sk_gpu_test::GrContextFactory::ContextType type) {
        return kOpenGL_GrBackend == sk_gpu_test::GrContextFactory::ContextTypeBackend(type);
    }
    bool IsVulkanContextType(sk_gpu_test::GrContextFactory::ContextType type) {
        return kVulkan_GrBackend == sk_gpu_test::GrContextFactory::ContextTypeBackend(type);
    }
    bool IsRenderingGLContextType(sk_gpu_test::GrContextFactory::ContextType type) {
        return IsGLContextType(type) && sk_gpu_test::GrContextFactory::IsRenderingContext(type);
    }
    bool IsNullGLContextType(sk_gpu_test::GrContextFactory::ContextType type) {
        return type == sk_gpu_test::GrContextFactory::kNullGL_ContextType;
    }
#else
    bool IsGLContextType         (int) { return false; }
    bool IsVulkanContextType     (int) { return false; }
    bool IsRenderingGLContextType(int) { return false; }
    bool IsNullGLContextType     (int) { return false; }
#endif

    void RunWithGPUTestContexts(GrContextTestFn* test, GrContextTypeFilterFn* contextTypeFilter,
                                Reporter* reporter, sk_gpu_test::GrContextFactory* factory) {
        // TODO(bsalomon)
    }
}
