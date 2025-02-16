#pragma once
#include "macros.h"
#include "pybind11.h"
#include "pybind11/embed.h"
#include <stdio.h>
#include <iostream>
#undef _DEBUG
#undef ERROR
#include <cocos2d.h>
#include "MinHook.h"
#include <tuple>
#include "globals.h"

namespace py = pybind11;

USING_NS_CC;

namespace utilities {

    // anonymous namespace
    namespace {
        // thank you hjfod

        class CallPyOnMainNode : public CCNode {
        protected:
            py::object m_function;

        public:
            static CallPyOnMainNode* create(py::object func) {
                auto ret = new CallPyOnMainNode;
                if (ret) {
                    ret->m_function = func;
                    ret->autorelease();
                    return ret;
                }
                CC_SAFE_DELETE(ret);
                return nullptr;
            }

            void onInvoke() {
                py::scoped_interpreter guard{};

                PyObject_CallNoArgs(m_function.ptr());
            }
        };

        class CallOnMainNode : public CCNode {
        protected:
            std::function<void()> m_function;

        public:
            static CallOnMainNode* create(std::function<void()> func) {
                auto ret = new CallOnMainNode;
                if (ret) {
                    ret->m_function = func;
                    ret->autorelease();
                    return ret;
                }
                CC_SAFE_DELETE(ret);
                return nullptr;
            }

            void onInvoke() {
                m_function();
            }
        };

        class CallPythonFileNode : public CCNode {
        protected:
            py::str m_arg;

        public:
            static CallPythonFileNode* create(py::str arg) {
                auto ret = new CallPythonFileNode;
                if (ret) {
                    ret->m_arg = arg;
                    ret->autorelease();
                    return ret;
                }
                CC_SAFE_DELETE(ret);
                return nullptr;
            }

            void onInvoke() {
                py::scoped_interpreter guard{};
                py::eval_file(m_arg);
            }
        };
    }
    // end anonymous namespace

    void runPyOnMain(py::object func) {
        auto node = CallPyOnMainNode::create(func);
        CCDirector::sharedDirector()->getRunningScene()->runAction(CCSequence::create(
            CCDelayTime::create(0),
            CCCallFunc::create(node, callfunc_selector(CallPyOnMainNode::onInvoke)),
            nullptr
        ));
    }

    void runOnMain(std::function<void()> func) {
        auto node = CallOnMainNode::create(func);
        CCDirector::sharedDirector()->getRunningScene()->runAction(CCSequence::create(
            CCDelayTime::create(0),
            CCCallFunc::create(node, callfunc_selector(CallOnMainNode::onInvoke)),
            nullptr
        ));
    }

    void runOnMainDelay(std::function<void()> func, float delay) {
        auto node = CallOnMainNode::create(func);
        CCDirector::sharedDirector()->getRunningScene()->runAction(CCSequence::create(
            CCDelayTime::create(delay),
            CCCallFunc::create(node, callfunc_selector(CallOnMainNode::onInvoke)),
            nullptr
        ));
    }

    void runPythonFile(py::str filename) {
        auto node = CallPythonFileNode::create(filename);
        CCDirector::sharedDirector()->getRunningScene()->runAction(CCSequence::create(
            CCDelayTime::create(0),
            CCCallFunc::create(node, callfunc_selector(CallPythonFileNode::onInvoke)),
            nullptr
        ));
    }

    void initialize() {
        MH_Initialize();
    }

    uintptr_t getBase() {
        return reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    }

    bool hasEnding(std::string const& fullString, std::string const& ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
        }
        else {
            return false;
        }
    }

    std::string getGDPath() {
        char path_c[MAX_PATH + 1];
        GetModuleFileNameA(NULL, path_c, sizeof(path_c));
        size_t pos = std::string::npos;
        std::string path = std::string(path_c);
        while ((pos = path.find("GeometryDash.exe")) != std::string::npos)
        {
            path.erase(pos, std::string("GeometryDash.exe").length());
        }
        return path;
    }

    void log(std::string message, LoggingLevel levelInt) {
        std::string stringmap[] = {
            "DEBUG",
            "INFO", 
            "WARNING",
            "ERROR",
            "CRITICAL"
        };

        std::string level = stringmap[(int)levelInt];

        std::cout << "CINNAMON: " << level << " >> " << message << std::endl;
    }

    void log(std::string message, std::string level="INFO") {
        int logLevel = loggingLevelStringToInt(level);

        if ((int)logLevel >= (int)globals::loggingLevel)
            std::cout << "CINNAMON: " << level << " >> " << message << std::endl;
    }

    void enableDebugMode() {
        globals::debugMode = true;
        globals::loggingLevel = LoggingLevel::DEBUG;

        AllocConsole();
        FILE* fDummy;
        freopen_s(&fDummy, "CONIN$", "r", stdin);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        freopen_s(&fDummy, "CONOUT$", "w", stdout);

        log("Debug mode enabled", "DEBUG");
    }

    MH_STATUS hookCinnamon(PVOID address, PVOID hook, LPVOID* original) {
        std::stringstream addr_stream;
        addr_stream << std::hex << address;
        std::string addr( addr_stream.str() );

        log("Hooking " + addr + ": " + std::to_string((unsigned int)(address)), "DEBUG");
        MH_STATUS status = MH_CreateHook(address, hook, original);
        log(std::string("Hooked ") + addr + std::string(" with status ") + std::string(MH_StatusToString(status)), "DEBUG");
        return status;
    }

    void setLoggingLevel(LoggingLevel level) {
        globals::loggingLevel = level;
    }

    void runWithExceptionHandler(std::function<void()> func) {
        try {
            func();
        }
        catch (py::error_already_set& e) {
            log(std::string("Exception: ") + e.what(), "ERROR");
        }
        catch (...) {
            log("Unknown exception", "ERROR");
        }
    }
}