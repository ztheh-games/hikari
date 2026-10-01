#ifndef HIKARI_CLIENT_SCRIPTING_SQUIRRELSERVICE
#define HIKARI_CLIENT_SCRIPTING_SQUIRRELSERVICE


#include <squirrel.h>
#include <sqstdmath.h>

#include <string>
#include <memory>

namespace hikari {

    class SquirrelService {
    private:
        static void squirrelPrintFunction(HSQUIRRELVM vm, const SQChar *s, ...);
        static void squirrelErrorFunction(HSQUIRRELVM vm, const SQChar *s, ...);
        static void squirrelLoggingProxyFunction(const std::string & message);

        static const SQInteger DEFAULT_STACK_SIZE;

        SQInteger initialStackSize;
        struct VmDeleter {
            void operator()(HSQUIRRELVM vm) const;
        };
        std::unique_ptr<SQVM, VmDeleter> vmOwner;
        HSQUIRRELVM vm;

        void initVirtualMachine();
        void initStandardLibraries();
        void initBindings();

        void deinitVirtualMachine();

    public:
        explicit SquirrelService(SQInteger initialStackSize = DEFAULT_STACK_SIZE);
        virtual ~SquirrelService();

        const HSQUIRRELVM getVmInstance();

        void runScriptFile(const std::string & fileName);
        void runScriptString(const std::string & scriptString);
        void collectGarbage();
    };

} // hikari

#endif // HIKARI_CLIENT_SCRIPTING_SQUIRRELSERVICE
